/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Add-game classification, SHELL.md section 6. Reads headers only. */
#include "vtshell.h"
#include <stdio.h>

#define VT_MAX_HEADER_FILE (64u * 1024u * 1024u)

static BOOL EndsWithI(LPCWSTR s, LPCWSTR suffix)
{
    size_t a = lstrlenW(s), b = lstrlenW(suffix);
    return a >= b && lstrcmpiW(s + a - b, suffix) == 0;
}

static BOOL ReadAt(HANDLE h, DWORD off, void *buf, DWORD len)
{
    DWORD got = 0;
    if (SetFilePointer(h, (LONG)off, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER)
        return FALSE;
    return ReadFile(h, buf, len, &got, NULL) && got == len;
}

VT_KIND VtClassifyFile(LPCWSTR path)
{
    HANDLE h;
    DWORD size, hi = 0, lfanew = 0;
    BYTE mz[0x40], sig[6];
    VT_KIND kind = VT_KIND_SKIP;

    if (EndsWithI(path, L".com") || EndsWithI(path, L".bat"))
        return VT_KIND_DOS;
    if (!EndsWithI(path, L".exe"))
        return VT_KIND_SKIP;

    h = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return VT_KIND_SKIP;
    size = GetFileSize(h, &hi);
    if (hi || size > VT_MAX_HEADER_FILE || size < 2 || !ReadAt(h, 0, mz, 2) ||
        mz[0] != 'M' || mz[1] != 'Z')
        goto done;

    kind = VT_KIND_DOS; /* plain MZ unless a new-style header says otherwise */
    if (size >= sizeof(mz) && ReadAt(h, 0, mz, sizeof(mz)) &&
        (mz[0x18] | (mz[0x19] << 8)) >= 0x40)
    {
        lfanew = mz[0x3C] | (mz[0x3D] << 8) | (mz[0x3E] << 16) | ((DWORD)mz[0x3F] << 24);
        if (lfanew >= 0x40 && lfanew <= size - sizeof(sig) && ReadAt(h, lfanew, sig, sizeof(sig)))
        {
            if (sig[0] == 'N' && sig[1] == 'E')
                kind = VT_KIND_WIN16; /* confirmed by kernel32 below */
            else if (sig[0] == 'P' && sig[1] == 'E' && !sig[2] && !sig[3])
                kind = ((sig[4] | (sig[5] << 8)) == 0x014C) ? VT_KIND_WIN32 : VT_KIND_UNSUPPORTED;
            /* LE / LX (DOS extenders) and anything else stay DOS */
        }
    }
done:
    CloseHandle(h);
    if (kind == VT_KIND_WIN16) {
        /* Same rule as launch hook rev 6b: only kernel32's own classifier
           decides Win16. NE files it doesn't call SCS_WOW_BINARY (Borland
           16-bit DOS extender, ne_exetyp 5) stay on the DOS path. */
        DWORD type;
        if (!GetBinaryTypeW(path, &type) || type != SCS_WOW_BINARY)
            kind = VT_KIND_DOS;
    }
    return kind;
}

LPCWSTR VtKindName(VT_KIND kind)
{
    switch (kind) {
    case VT_KIND_DOS: return L"dos";
    case VT_KIND_WIN16: return L"win16";
    case VT_KIND_WIN32: return L"win32";
    case VT_KIND_UNSUPPORTED: return L"unsupported";
    default: return L"skip";
    }
}

static BOOL IsExcludedName(LPCWSTR name)
{
    static const WCHAR *prefixes[] = { L"SETUP", L"INSTALL", L"UNINST", L"CONFIG",
                                       L"SETSOUND", L"README" };
    WCHAR up[MAX_PATH];
    int i;
    lstrcpynW(up, name, MAX_PATH);
    CharUpperW(up);
    for (i = 0; i < (int)(sizeof(prefixes) / sizeof(prefixes[0])); i++)
        if (wcsncmp(up, prefixes[i], lstrlenW(prefixes[i])) == 0)
            return TRUE;
    return FALSE;
}

/* Main-EXE heuristic for one folder (non-recursive): name matching the folder,
   else the largest non-excluded program. .BAT files are only a last resort. */
BOOL VtPickMainExe(LPCWSTR folder, LPWSTR exeOut, VT_KIND *kindOut)
{
    WCHAR pattern[MAX_PATH], base[MAX_PATH], stem[MAX_PATH], full[MAX_PATH];
    WIN32_FIND_DATAW fd;
    HANDLE f;
    LPCWSTR leaf;
    DWORD bestSize = 0;
    BOOL found = FALSE, nameHit = FALSE, bestIsBat = TRUE;

    leaf = wcsrchr(folder, L'\\');
    lstrcpynW(base, leaf ? leaf + 1 : folder, MAX_PATH);
    wsprintfW(pattern, L"%s\\*", folder);
    f = FindFirstFileW(pattern, &fd);
    if (f == INVALID_HANDLE_VALUE)
        return FALSE;
    do {
        VT_KIND k;
        WCHAR *dot;
        BOOL isBat;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        if (IsExcludedName(fd.cFileName))
            continue;
        wsprintfW(full, L"%s\\%s", folder, fd.cFileName);
        k = VtClassifyFile(full);
        if (k == VT_KIND_SKIP || k == VT_KIND_UNSUPPORTED)
            continue;
        isBat = EndsWithI(fd.cFileName, L".bat");
        lstrcpynW(stem, fd.cFileName, MAX_PATH);
        if ((dot = wcsrchr(stem, L'.')) != NULL)
            *dot = 0;
        if (!nameHit && lstrcmpiW(stem, base) == 0 && !isBat) {
            nameHit = TRUE;
            found = TRUE;
            lstrcpynW(exeOut, full, MAX_PATH);
            *kindOut = k;
            continue;
        }
        if (nameHit)
            continue;
        if (!found || (bestIsBat && !isBat) ||
            (bestIsBat == isBat && fd.nFileSizeLow > bestSize)) {
            found = TRUE;
            bestIsBat = isBat;
            bestSize = fd.nFileSizeLow;
            lstrcpynW(exeOut, full, MAX_PATH);
            *kindOut = k;
        }
    } while (FindNextFileW(f, &fd));
    FindClose(f);
    return found;
}

static void ClassifyWalk(LPCWSTR folder, int depth)
{
    WCHAR pattern[MAX_PATH], full[MAX_PATH];
    WIN32_FIND_DATAW fd;
    HANDLE f;

    wsprintfW(pattern, L"%s\\*", folder);
    f = FindFirstFileW(pattern, &fd);
    if (f == INVALID_HANDLE_VALUE)
        return;
    do {
        if (!lstrcmpW(fd.cFileName, L".") || !lstrcmpW(fd.cFileName, L".."))
            continue;
        wsprintfW(full, L"%s\\%s", folder, fd.cFileName);
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (depth < 3)
                ClassifyWalk(full, depth + 1);
        } else {
            VT_KIND k = VtClassifyFile(full);
            if (k != VT_KIND_SKIP)
                wprintf(L"\"%s\",%s\n", full, VtKindName(k));
        }
    } while (FindNextFileW(f, &fd));
    FindClose(f);
}

int VtClassifyCli(LPCWSTR folder)
{
    wprintf(L"path,kind\n");
    ClassifyWalk(folder, 1);
    return 0;
}
