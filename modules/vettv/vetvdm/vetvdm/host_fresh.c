/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * PROJECT:  VetTV Retro OS
 * FILE:     modules/vettv/vetvdm/vetvdm/host_fresh.c
 * PURPOSE:  PR #28 stopgap: fresh, non-shared WineVDM process per launch.
 *
 * Copyright (C) 2026 VetTV
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * Flag: VETVDM_FRESH_WINEVDM=1 in the environment, or
 *       [vetvdm] FreshWineVDM=1 in vetvdm.ini next to vetvdm.exe.
 *       The environment wins when set. Default off.
 *
 * vetvdm already starts a new otvdmw.exe per launch and waits on it, and
 * otvdm v0.9.0 only reuses a running process when otvdm.ini sets
 * SeparateWOWVDM=0 (default 1). With the flag on, vetvdm also:
 *   - counts otvdm*.exe already running (route line otvdm_pre=) so QA can
 *     see whether launch 1's process survived into launch 2;
 *   - reads SeparateWOWVDM from otvdm.ini next to otvdmw.exe (wow_ini=) and
 *     warns when it is 0 (shared server);
 *   - after its otvdmw.exe exits, ends any process still in its OWN job,
 *     or (nested in vettvshell's job) only otvdm*.exe descendants of its
 *     own child created after that child. Nothing else is touched.
 */

#include "host.h"
#include "route.h"

#include "win32.h"

#include <tlhelp32.h>
#include <stdio.h>

int
fresh_wine_enabled(void)
{
    WCHAR value[16];
    WCHAR ini[MAX_PATH];
    WCHAR *slash;
    DWORD n;

    n = GetEnvironmentVariableW(L"VETVDM_FRESH_WINEVDM", value, 16);
    if (n > 0 && n < 16)
        return vetvdm_flag_on(value);
    n = GetModuleFileNameW(NULL, ini, MAX_PATH);
    if (n == 0 || n >= MAX_PATH)
        return 0;
    slash = wcsrchr(ini, L'\\');
    if (!slash || (DWORD)(slash - ini) + 12 >= MAX_PATH)
        return 0;
    lstrcpyW(slash + 1, L"vetvdm.ini");
    if (!file_exists(ini))
        return 0;
    GetPrivateProfileStringW(L"vetvdm", L"FreshWineVDM", L"", value, 16, ini);
    return vetvdm_flag_on(value);
}

static int
is_otvdm(const WCHAR *exe)
{
    return exe && CompareStringW(LOCALE_INVARIANT, NORM_IGNORECASE, exe, 5,
                                 L"otvdm", 5) == CSTR_EQUAL;
}

void
fresh_wine_prepare(host *st)
{
    WCHAR ini[MAX_PATH];
    HANDLE snap;
    PROCESSENTRY32W pe;
    long count = 0;

    st->otvdm_pre = -1;
    st->wow_ini = 1;
    if (st->wine[0] && replace_file(st->wine, L"otvdm.ini", ini, MAX_PATH) &&
        file_exists(ini))
        st->wow_ini = GetPrivateProfileIntW(L"otvdm", L"SeparateWOWVDM", 1, ini) ? 1 : 0;
    if (!st->fresh_wine)
        return;
    if (st->wow_ini == 0)
        warn_add(st->warn, sizeof(st->warn), "otvdm-ini-shared-wow");
    snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return;
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe))
    {
        do
        {
            if (is_otvdm(pe.szExeFile))
                count++;
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    st->otvdm_pre = count;
}

#define FRESH_MAX 32

/* otvdm*.exe descendants of root created no earlier than root. */
static DWORD
own_otvdm_tree(DWORD root, const FILETIME *born, DWORD *pids, DWORD cap)
{
    HANDLE snap;
    PROCESSENTRY32W pe;
    DWORD parents[FRESH_MAX + 1];
    DWORD np = 1, n = 0, i;
    int grew = 1;

    parents[0] = root;
    snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE)
        return 0;
    while (grew && np <= FRESH_MAX)
    {
        grew = 0;
        pe.dwSize = sizeof(pe);
        if (!Process32FirstW(snap, &pe))
            break;
        do
        {
            int is_parent = 0, seen = 0;
            for (i = 0; i < np; i++)
            {
                if (parents[i] == pe.th32ParentProcessID)
                    is_parent = 1;
                if (parents[i] == pe.th32ProcessID)
                    seen = 1;
            }
            if (!is_parent || seen || pe.th32ProcessID == root ||
                np > FRESH_MAX)
                continue;
            parents[np++] = pe.th32ProcessID;
            grew = 1;
            if (is_otvdm(pe.szExeFile) && n < cap)
            {
                /* Guard against a reused pid: must be born after our child. */
                HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe.th32ProcessID);
                FILETIME c, e, k, u;
                if (h && GetProcessTimes(h, &c, &e, &k, &u) &&
                    CompareFileTime(&c, born) >= 0)
                    pids[n++] = pe.th32ProcessID;
                if (h)
                    CloseHandle(h);
            }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return n;
}

void
fresh_wine_cleanup(host *st, struct vcore_job *job, DWORD child_pid)
{
    DWORD reaped = 0;
    char note[40];

    if (!job)
        return;
    if (job->job)
    {
        JOBOBJECT_BASIC_ACCOUNTING_INFORMATION acct;
        memset(&acct, 0, sizeof(acct));
        if (QueryInformationJobObject(job->job, JobObjectBasicAccountingInformation,
                                      &acct, sizeof(acct), NULL) &&
            acct.ActiveProcesses > 0)
        {
            reaped = acct.ActiveProcesses;
            TerminateJobObject(job->job, 0);
        }
    }
    else if (job->process && child_pid)
    {
        FILETIME born, e, k, u;
        DWORD pids[FRESH_MAX];
        DWORD n, i;
        if (GetProcessTimes(job->process, &born, &e, &k, &u))
        {
            n = own_otvdm_tree(child_pid, &born, pids, FRESH_MAX);
            for (i = 0; i < n; i++)
            {
                HANDLE h = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pids[i]);
                if (!h)
                    continue;
                if (TerminateProcess(h, 0))
                {
                    WaitForSingleObject(h, 5000);
                    reaped++;
                }
                CloseHandle(h);
            }
        }
    }
    st->otvdm_reaped = reaped;
    _snprintf(note, sizeof(note), "fresh-reaped:%lu", (unsigned long)reaped);
    note[sizeof(note) - 1] = 0;
    warn_add(st->warn, sizeof(st->warn), note);
}
