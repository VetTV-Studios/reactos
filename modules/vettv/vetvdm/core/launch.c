/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * PROJECT:  VetTV Retro OS
 * FILE:     modules/vettv/vetvdm/core/launch.c
 * PURPOSE:  Start a layer process inside a kill-on-close job object.
 *
 * Copyright (C) 2026 VetTV
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * CreateProcessW is CREATE_SUSPENDED so AssignProcessToJobObject runs
 * before the child can spawn. Closing the job kills DOSBox-X and its
 * children (JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE).
 *
 * The child inherits this process environment unchanged. NTVDM is
 * CreateProcessW(image, tail) from vetvdm itself. The launch hook skips
 * any caller whose image name is vetvdm.exe, so no bypass variable is set.
 */

#include "win32.h"

#include <string.h>

DWORD
vcore_launch_in_job(const WCHAR *app, WCHAR *cmdline, const WCHAR *cwd,
                   HANDLE std_out, vcore_job *out)
{
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    DWORD flags;
    BOOL inherit = FALSE;

    if (!out || !app || !app[0] || !cmdline)
        return VETVDM_E_INTERNAL;
    out->job = NULL;
    out->process = NULL;
    out->thread = NULL;
    out->pid = 0;
    out->nested = 0;
    out->os_error = 0;

    out->job = CreateJobObjectW(NULL, NULL);
    if (!out->job)
        return VETVDM_E_START;
    memset(&info, 0, sizeof(info));
    info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(out->job, JobObjectExtendedLimitInformation,
                                &info, sizeof(info)))
    {
        CloseHandle(out->job);
        out->job = NULL;
        return VETVDM_E_START;
    }

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    /*
     * Bug #2: DOSBox-X's [log] logfile= never wrote anything on ReactOS.
     * Give the child a real file as stdout and stderr instead.
     */
    if (std_out && std_out != INVALID_HANDLE_VALUE)
    {
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = std_out;
        si.hStdError = std_out;
        inherit = TRUE;
    }
    memset(&pi, 0, sizeof(pi));
    /*
     * vettvshell runs vetvdm inside its own job, and XP-era kernels and
     * ReactOS cannot nest jobs. Try to break away first. If the outer job
     * forbids that, start the child without it and stay in the outer job.
     */
    flags = CREATE_SUSPENDED | CREATE_BREAKAWAY_FROM_JOB;
    if (!CreateProcessW(app, cmdline, NULL, NULL, inherit, flags, NULL,
                       (cwd && cwd[0]) ? cwd : NULL, &si, &pi))
    {
        flags = CREATE_SUSPENDED;
        if (!CreateProcessW(app, cmdline, NULL, NULL, inherit, flags, NULL,
                           (cwd && cwd[0]) ? cwd : NULL, &si, &pi))
        {
            out->os_error = GetLastError();
            pi.hProcess = NULL;
        }
    }
    if (!pi.hProcess)
    {
        CloseHandle(out->job);
        out->job = NULL;
        return VETVDM_E_START;
    }
    if (!AssignProcessToJobObject(out->job, pi.hProcess))
    {
        /* Still in the outer job; that job owns cleanup. */
        CloseHandle(out->job);
        out->job = NULL;
        out->nested = 1;
    }
    if (ResumeThread(pi.hThread) == (DWORD)-1)
    {
        TerminateProcess(pi.hProcess, VETVDM_E_START);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        CloseHandle(out->job);
        out->job = NULL;
        return VETVDM_E_START;
    }
    out->process = pi.hProcess;
    out->thread = pi.hThread;
    out->pid = pi.dwProcessId;
    /* Pass on the foreground right vettvshell gave us (ASFW_ANY). */
    AllowSetForegroundWindow(pi.dwProcessId);
    return VETVDM_OK;
}

typedef struct
{
    DWORD pid;
    HWND hwnd;
} vcore_find_window;

static BOOL CALLBACK
vcore_find_window_cb(HWND hwnd, LPARAM lp)
{
    vcore_find_window *f = (vcore_find_window *)lp;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == f->pid && IsWindowVisible(hwnd) && !GetWindow(hwnd, GW_OWNER))
    {
        f->hwnd = hwnd;
        return FALSE;
    }
    return TRUE;
}

/*
 * Wait up to 10 s for the layer's first visible top-level window and
 * bring it to the front, so the game has keyboard focus without a click
 * and the first keystroke is not lost to the launcher (QA qa3d, Lead 3e).
 * ReactOS may still refuse; AttachThreadInput is the XP-era fallback.
 */
static void
vcore_bring_to_front(vcore_job *job)
{
    vcore_find_window f;
    int i;

    f.pid = job->pid;
    f.hwnd = NULL;
    for (i = 0; i < 100 && !f.hwnd; i++)
    {
        if (WaitForSingleObject(job->process, 100) != WAIT_TIMEOUT)
            return;
        EnumWindows(vcore_find_window_cb, (LPARAM)&f);
    }
    if (!f.hwnd)
        return;
    if (!SetForegroundWindow(f.hwnd))
    {
        HWND fg = GetForegroundWindow();
        DWORD me = GetCurrentThreadId();
        DWORD them = fg ? GetWindowThreadProcessId(fg, NULL) : 0;
        if (them && them != me && AttachThreadInput(me, them, TRUE))
        {
            SetForegroundWindow(f.hwnd);
            AttachThreadInput(me, them, FALSE);
        }
    }
    BringWindowToTop(f.hwnd);
    SetFocus(f.hwnd);
}

DWORD
vcore_job_wait(vcore_job *job, DWORD *exit_code)
{
    DWORD code = 1;
    if (!job || !job->process)
        return VETVDM_E_START;
    vcore_bring_to_front(job);
    if (WaitForSingleObject(job->process, INFINITE) != WAIT_OBJECT_0)
        return VETVDM_E_START;
    if (!GetExitCodeProcess(job->process, &code))
        return VETVDM_E_START;
    if (exit_code)
        *exit_code = code;
    return VETVDM_OK;
}

void
vcore_job_close(vcore_job *job)
{
    if (!job)
        return;
    if (job->thread)
        CloseHandle(job->thread);
    if (job->process)
        CloseHandle(job->process);
    if (job->job)
        CloseHandle(job->job);
    job->thread = NULL;
    job->process = NULL;
    job->job = NULL;
}
