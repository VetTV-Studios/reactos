/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Launching (SHELL.md section 2 and "Launching titles vs the launch hook").
   DOS/Win16 go straight to vetvdm.exe (no --from-hook); Win32 is a plain
   CreateProcessW. Everything runs in a kill-on-close job so the quit combo
   can end the whole tree. Called on a worker thread; it blocks. */
#include "vtshell.h"

static HANDLE g_proc;
static HANDLE g_job;            /* job of the running game, NULL when idle */
static CRITICAL_SECTION g_lock;

void VtLaunchInit(void)
{
    InitializeCriticalSection(&g_lock);
}

static void Lock(void)
{
    EnterCriticalSection(&g_lock);
}

static void GameDir(const VT_GAME *g, LPWSTR out)
{
    WCHAR *slash;
    lstrcpynW(out, g->exe, MAX_PATH);
    if ((slash = wcsrchr(out, L'\\')) != NULL)
        *slash = 0;
    else
        lstrcpynW(out, g->folder, MAX_PATH);
}

void VtLaunch(const VT_GAME *g, VT_LAUNCH_RESULT *r)
{
    static WCHAR cmd[4096];
    WCHAR vdm[MAX_PATH], cwd[MAX_PATH], profile[MAX_PATH + 16] = L"";
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION lim;
    DWORD flags = CREATE_SUSPENDED, start;
    HANDLE job;
    BOOL viaVdm = (g->kind == VT_KIND_DOS || g->kind == VT_KIND_WIN16);

    ZeroMemory(r, sizeof(*r));
    cmd[4095] = 0;
    GameDir(g, cwd);

    if (viaVdm) {
        if (!VtVetvdmPath(vdm, MAX_PATH)) {
            r->createError = ERROR_PATH_NOT_FOUND;
            return;
        }
        if (g->json[0])
            wsprintfW(profile, L" --profile=\"%s\"", g->json);
        /* --kind=auto: vetvdm reads the header itself, so its rules (e.g.
           DOS-extender NE -> DOS) win over the library tag.
           --caller=shell needs vetvdm rev 3e or later (3d rejects it). */
        _snwprintf(cmd, 4095, L"\"%s\" --caller=shell --kind=auto --image=\"%s\"%s -- \"%s\"%s%s",
                  vdm, g->exe, profile, g->exe,
                  g->args[0] ? L" " : L"", g->args);
        /* vetvdm is a console program; don't let a console window flash up
           over the library. QA check H17. */
        flags |= CREATE_NO_WINDOW;
    } else {
        _snwprintf(cmd, 4095, L"\"%s\"%s%s", g->exe, g->args[0] ? L" " : L"", g->args);
    }

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    VtLog(L"launch kind=%s cmd=%s", VtKindName(g->kind), cmd);
    /* The launcher is foreground now; let the game (or DOSBox-X, a
       grandchild via vetvdm) take the foreground when its window opens. */
    AllowSetForegroundWindow(ASFW_ANY);
    if (!CreateProcessW(viaVdm ? vdm : g->exe, cmd, NULL, NULL, FALSE, flags,
                        NULL, cwd, &si, &pi)) {
        r->createError = GetLastError();
        VtLog(L"launch failed error=%lu", r->createError);
        return;
    }

    /* vetvdm puts itself and DOSBox-X/WineVDM in its own kill-on-close job.
       ReactOS (NT 5.2) has no nested jobs, so a shell job here made vetvdm's
       AssignProcessToJobObject fail (0xE0000005). For vetvdm launches the
       shell tracks the process instead; killing vetvdm closes its job. */
    job = viaVdm ? NULL : CreateJobObjectW(NULL, NULL);
    if (job) {
        ZeroMemory(&lim, sizeof(lim));
        lim.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_BREAKAWAY_OK; /* a Win32 game may start vetvdm, which needs its own job (no nesting on NT 5.2) */
        if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &lim, sizeof(lim)) ||
            !AssignProcessToJobObject(job, pi.hProcess)) {
            VtLog(L"job setup failed error=%lu, running without job", GetLastError());
            CloseHandle(job);
            job = NULL;
        }
    }
    Lock();
    g_job = job;
    g_proc = pi.hProcess;
    LeaveCriticalSection(&g_lock);

    r->started = TRUE;
    start = GetTickCount();
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &r->exitCode);
    Lock();
    g_proc = NULL;
    LeaveCriticalSection(&g_lock);
    CloseHandle(pi.hProcess);
    r->seconds = (GetTickCount() - start) / 1000;

    Lock();
    g_job = NULL;
    LeaveCriticalSection(&g_lock);
    if (job)
        CloseHandle(job); /* kill-on-close also ends anything left behind */
    VtLog(L"exit code=0x%08lX seconds=%lu", r->exitCode, r->seconds);
}

void VtLaunchAbort(void)
{
    Lock();
    if (g_job) {
        TerminateJobObject(g_job, VETVDM_E_CANCEL); /* quit combo: quiet return */
        VtLog(L"quit combo: job terminated");
    } else if (g_proc) {
        TerminateProcess(g_proc, VETVDM_E_CANCEL); /* vetvdm's own job ends its children */
        VtLog(L"quit combo: vetvdm terminated");
    }
    LeaveCriticalSection(&g_lock);
}

/* SHELL.md section 5 plus VETVDM_SPEC section 6. FALSE = no message. */
BOOL VtErrorText(const VT_LAUNCH_RESULT *r, LPWSTR out, DWORD cch)
{
    WCHAR drive[4] = L"C:";
    LPCWSTR text = NULL;
    (void)cch;

    if (!r->started) {
        wsprintfW(out, L"Couldn't start the game (error %lu).", r->createError);
        return TRUE;
    }
    if ((r->exitCode & 0xFFFFFF00u) != 0xE0000000u)
        return FALSE; /* the program's own exit code */
    switch (r->exitCode) {
    case VETVDM_E_NORESULT: text = L"The game didn't finish cleanly. See vetvdm.log for details."; break;
    case VETVDM_E_INTERNAL: text = L"The DOS/16-bit player didn't start."; break;
    case VETVDM_E_CMDLINE: text = L"The game's start settings are wrong. Check its Options."; break;
    case VETVDM_E_NOPROG: text = L"Can't find the game's program file. It may have been moved."; break;
    case VETVDM_E_NOTDOS: text = L"This file isn't a DOS or 16-bit program."; break;
    case VETVDM_E_NOLAYER: text = L"The DOS/16-bit player isn't installed on this system."; break; /* VETVDM_E_NOLAYER */
    case VETVDM_E_START: text = L"The DOS/16-bit player didn't start."; break;
    case VETVDM_E_WIN31: text = L"This game is set to use your own Windows 3.1 install, but none is set up."; break;
    case VETVDM_E_PROFILE: text = L"This game's settings file is damaged. Reset it in Options."; break;
    case VETVDM_E_CANCEL: return FALSE; /* the user cancelled vetvdm's chooser */
    default: break;
    }
    VtSystemDrive(drive, 4);
    if (text)
        wsprintfW(out, L"%s Details: %s\\VetTV\\logs\\vetvdm.log", text, drive);
    else
        wsprintfW(out, L"Something went wrong starting the game (code 0x%08lX). Details: %s\\VetTV\\logs\\vetvdm.log",
                  r->exitCode, drive);
    return TRUE;
}
