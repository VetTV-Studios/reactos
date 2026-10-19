/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Library: one game per folder under each library root. A vettv.game.json
   (SHELL.md section 7) wins; otherwise the folder is classified on the fly. */
#include "vtshell.h"
#define JSMN_STATIC
#include "jsmn.h"

#define VT_JSON_MAX (64 * 1024)

static BOOL JsonEq(const char *js, const jsmntok_t *t, const char *s)
{
    int len = lstrlenA(s);
    return t->type == JSMN_STRING && t->end - t->start == len &&
           memcmp(js + t->start, s, len) == 0;
}

static void JsonCopy(const char *js, const jsmntok_t *t, LPWSTR out, int cch)
{
    int n = MultiByteToWideChar(CP_UTF8, 0, js + t->start, t->end - t->start,
                                out, cch - 1);
    out[n > 0 ? n : 0] = 0;
}

/* Index of the token after tokens[i] and all of its children. */
static int JsonSkip(const jsmntok_t *t, int i, int count)
{
    int end = t[i].end;
    i++;
    while (i < count && t[i].start < end)
        i++;
    return i;
}

static BOOL LoadGameJson(VT_GAME *g, LPCWSTR folder, LPCWSTR jsonPath)
{
    static char buf[VT_JSON_MAX + 1];
    static jsmntok_t tok[512];
    jsmn_parser p;
    HANDLE h;
    DWORD got = 0;
    int n, i;
    WCHAR exeRel[MAX_PATH] = L"", platform[16] = L"";

    h = CreateFileW(jsonPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return FALSE;
    ReadFile(h, buf, VT_JSON_MAX, &got, NULL);
    CloseHandle(h);
    buf[got] = 0;

    jsmn_init(&p);
    n = jsmn_parse(&p, buf, got, tok, sizeof(tok) / sizeof(tok[0]));
    if (n < 1 || tok[0].type != JSMN_OBJECT)
        return FALSE;
    for (i = 1; i + 1 < n; ) {
        const jsmntok_t *k = &tok[i], *v = &tok[i + 1];
        if (v->type == JSMN_STRING) {
            if (JsonEq(buf, k, "title")) JsonCopy(buf, v, g->title, VT_TITLE_LEN);
            else if (JsonEq(buf, k, "exe")) JsonCopy(buf, v, exeRel, MAX_PATH);
            else if (JsonEq(buf, k, "args")) JsonCopy(buf, v, g->args, 256);
            else if (JsonEq(buf, k, "platform")) JsonCopy(buf, v, platform, 16);
        }
        i = JsonSkip(tok, i + 1, n);
    }
    if (!exeRel[0])
        return FALSE;
    wsprintfW(g->exe, L"%s\\%s", folder, exeRel);
    if (!lstrcmpiW(platform, L"dos")) g->kind = VT_KIND_DOS;
    else if (!lstrcmpiW(platform, L"win16")) g->kind = VT_KIND_WIN16;
    else if (!lstrcmpiW(platform, L"win32")) g->kind = VT_KIND_WIN32;
    else g->kind = VtClassifyFile(g->exe);
    lstrcpynW(g->json, jsonPath, MAX_PATH);
    return TRUE;
}

static void ScanRoot(VT_LIBRARY *lib, LPCWSTR root)
{
    WCHAR pattern[MAX_PATH], folder[MAX_PATH], json[MAX_PATH];
    WIN32_FIND_DATAW fd;
    HANDLE f;

    wsprintfW(pattern, L"%s\\*", root);
    f = FindFirstFileW(pattern, &fd);
    if (f == INVALID_HANDLE_VALUE)
        return;
    do {
        VT_GAME *g;
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || fd.cFileName[0] == L'.')
            continue;
        if (lib->count >= VT_MAX_GAMES)
            break;
        g = &lib->games[lib->count];
        ZeroMemory(g, sizeof(*g));
        wsprintfW(folder, L"%s\\%s", root, fd.cFileName);
        lstrcpynW(g->folder, folder, MAX_PATH);
        lstrcpynW(g->title, fd.cFileName, VT_TITLE_LEN);
        wsprintfW(json, L"%s\\vettv.game.json", folder);
        if (!LoadGameJson(g, folder, json) && !VtPickMainExe(folder, g->exe, &g->kind))
            continue;
        if (g->kind == VT_KIND_SKIP)
            continue;
        lib->count++;
    } while (FindNextFileW(f, &fd));
    FindClose(f);
}

/* Roots: every fixed or CD drive's \Games folder (SHELL.md default D:\Games). */
void VtLibraryLoad(VT_LIBRARY *lib)
{
    WCHAR root[8];
    DWORD drives = GetLogicalDrives();
    int d;

    lib->count = 0;
    for (d = 2; d < 26; d++) { /* skip A: and B: so floppies never spin up */
        UINT type;
        WCHAR probe[4];
        if (!(drives & (1u << d)))
            continue;
        wsprintfW(probe, L"%c:\\", L'A' + d);
        type = GetDriveTypeW(probe);
        if (type != DRIVE_FIXED && type != DRIVE_CDROM && type != DRIVE_REMOVABLE)
            continue;
        wsprintfW(root, L"%c:\\Games", L'A' + d);
        if (GetFileAttributesW(root) != INVALID_FILE_ATTRIBUTES)
            ScanRoot(lib, root);
    }
    VtLog(L"library loaded: %d games", lib->count);
}
