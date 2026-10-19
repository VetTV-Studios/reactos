/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Path rules (SHELL.md "Profile paths"): system calls only, no profile env
   vars, no literal profile folder names. */
#include "vtshell.h"
#include <shlobj.h>
#include <stdarg.h>

BOOL VtSystemDrive(LPWSTR out, DWORD cch)
{
    WCHAR win[MAX_PATH];
    UINT n = GetWindowsDirectoryW(win, MAX_PATH);
    if (n < 2 || n >= MAX_PATH || win[1] != L':' || cch < 3)
        return FALSE;
    out[0] = win[0];
    out[1] = L':';
    out[2] = 0;
    return TRUE;
}

BOOL VtVetvdmPath(LPWSTR out, DWORD cch)
{
    WCHAR sys[MAX_PATH];
    UINT n = GetSystemDirectoryW(sys, MAX_PATH);
    if (!n || n >= MAX_PATH || (DWORD)n + 12 >= cch)
        return FALSE;
    wsprintfW(out, L"%s\\vetvdm.exe", sys);
    return TRUE;
}

/* Per-user shell data: <Local AppData>\VetTV\Shell. Falls back to
   <exe folder>\data if the lookup fails; the caller shows a notice. */
BOOL VtDataDir(LPWSTR out, DWORD cch, BOOL *usedFallback)
{
    WCHAR base[MAX_PATH];
    *usedFallback = FALSE;
    if (cch < MAX_PATH)
        return FALSE;
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL,
                                   SHGFP_TYPE_CURRENT, base)) && base[0]) {
        wsprintfW(out, L"%s\\VetTV", base);
        CreateDirectoryW(out, NULL);
        wsprintfW(out, L"%s\\VetTV\\Shell", base);
    } else {
        WCHAR *slash;
        *usedFallback = TRUE;
        if (!GetModuleFileNameW(NULL, base, MAX_PATH))
            return FALSE;
        if ((slash = wcsrchr(base, L'\\')) != NULL)
            *slash = 0;
        wsprintfW(out, L"%s\\data", base);
    }
    CreateDirectoryW(out, NULL);
    return TRUE;
}

void VtLog(LPCWSTR fmt, ...)
{
    WCHAR drive[4], path[MAX_PATH], line[1024], msg[900];
    char utf8[3072];
    SYSTEMTIME st;
    HANDLE h;
    DWORD wrote;
    int n;
    va_list ap;

    if (!VtSystemDrive(drive, 4))
        return;
    wsprintfW(path, L"%s\\VetTV", drive);
    CreateDirectoryW(path, NULL);
    wsprintfW(path, L"%s\\VetTV\\logs", drive);
    CreateDirectoryW(path, NULL);
    wsprintfW(path, L"%s\\VetTV\\logs\\shell.log", drive);

    va_start(ap, fmt);
    wvsprintfW(msg, fmt, ap);
    va_end(ap);
    GetLocalTime(&st);
    wsprintfW(line, L"%04u-%02u-%02u %02u:%02u:%02u tick=%lu %s\r\n",
              st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond,
              GetTickCount(), msg);
    n = WideCharToMultiByte(CP_UTF8, 0, line, -1, utf8, sizeof(utf8), NULL, NULL);
    if (n <= 1)
        return;
    h = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return;
    WriteFile(h, utf8, (DWORD)(n - 1), &wrote, NULL);
    CloseHandle(h);
}
