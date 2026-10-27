/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * vettvshell - first M4 slice: library screen (S1), keyboard navigation,
 * launch + return (S3), quit combo, exit to desktop, --activate, --classify.
 * Keyboard-only is a first-class path (RK-XHCI): no pad prompt, no waits.
 * Pad input, game detail (S2), Add-game wizard (S4) and settings come next.
 */
#include "vtshell.h"
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

#define WM_APP_DONE (WM_APP + 1)
#define VT_EVENT_NAME L"Local\\VetTV.Shell.Activate"
#define VT_MUTEX_NAME L"Local\\VetTV.Shell"

/* beige-crt palette, SHELL.md section 8 */
#define C_PUTTY   RGB(0xD8, 0xD0, 0xB8)
#define C_SHADOW  RGB(0xA8, 0x9F, 0x86)
#define C_BEZEL   RGB(0x3A, 0x36, 0x31)
#define C_PHOSPHOR RGB(0x0E, 0x14, 0x10)
#define C_GLOW    RGB(0x7C, 0xFF, 0x9A)
#define C_AMBER   RGB(0xFF, 0xB3, 0x47)
#define C_HILITE  RGB(0xF2, 0xEC, 0xDC)

enum { SHELF_DOS, SHELF_PC, SHELF_ALL, SHELF_COUNT };
static const WCHAR *g_shelfName[SHELF_COUNT] = { L"DOS & 16-bit", L"32-bit PC", L"All" };

static VT_LIBRARY g_lib;
static int g_shelfIdx[SHELF_COUNT][VT_MAX_GAMES];
static int g_shelfLen[SHELF_COUNT];
static int g_focusShelf, g_focusCol[SHELF_COUNT];
static HWND g_wnd;
static HFONT g_fontHead, g_fontBody, g_fontSmall;
static WCHAR g_status[512];
static BOOL g_running, g_firstFrameLogged, g_dataFallback;
static HHOOK g_kbHook;
static HANDLE g_activate;
static VT_LAUNCH_RESULT g_result;
static int g_launchIdx;

static void BuildShelves(void)
{
    int i, s;
    for (s = 0; s < SHELF_COUNT; s++)
        g_shelfLen[s] = 0;
    for (i = 0; i < g_lib.count; i++) {
        VT_KIND k = g_lib.games[i].kind;
        if (k == VT_KIND_DOS || k == VT_KIND_WIN16)
            g_shelfIdx[SHELF_DOS][g_shelfLen[SHELF_DOS]++] = i;
        else if (k == VT_KIND_WIN32)
            g_shelfIdx[SHELF_PC][g_shelfLen[SHELF_PC]++] = i;
        g_shelfIdx[SHELF_ALL][g_shelfLen[SHELF_ALL]++] = i;
    }
    for (s = 0; s < SHELF_COUNT; s++)
        if (g_focusCol[s] >= g_shelfLen[s])
            g_focusCol[s] = g_shelfLen[s] ? g_shelfLen[s] - 1 : 0;
    if (!g_shelfLen[g_focusShelf])
        for (s = 0; s < SHELF_COUNT; s++)
            if (g_shelfLen[s]) { g_focusShelf = s; break; }
}

static void Reload(void)
{
    VtLibraryLoad(&g_lib);
    BuildShelves();
    if (g_lib.count)
        wsprintfW(g_status, L"%d games. Arrows to move, Enter to play, F5 to refresh, F10 for the desktop.", g_lib.count);
    else
        lstrcpyW(g_status, L"No games yet. Put each game in its own folder under D:\\Games, then press F5.");
    if (g_dataFallback)
        lstrcatW(g_status, L" (Settings are saved next to the launcher.)");
}

static void Fill(HDC dc, int x, int y, int w, int h, COLORREF c)
{
    RECT r = { x, y, x + w, y + h };
    HBRUSH b = CreateSolidBrush(c);
    FillRect(dc, &r, b);
    DeleteObject(b);
}

static void Bevel(HDC dc, int x, int y, int w, int h, BOOL raised)
{
    Fill(dc, x, y, w, 2, raised ? C_HILITE : C_SHADOW);
    Fill(dc, x, y, 2, h, raised ? C_HILITE : C_SHADOW);
    Fill(dc, x, y + h - 2, w, 2, raised ? C_SHADOW : C_HILITE);
    Fill(dc, x + w - 2, y, 2, h, raised ? C_SHADOW : C_HILITE);
}

static void Text(HDC dc, HFONT f, COLORREF c, int x, int y, int w, int h, LPCWSTR s, UINT fmt)
{
    RECT r = { x, y, x + w, y + h };
    SelectObject(dc, f);
    SetTextColor(dc, c);
    SetBkMode(dc, TRANSPARENT);
    DrawTextW(dc, s, -1, &r, fmt | DT_NOPREFIX);
}

static void DrawTile(HDC dc, int x, int y, int w, int h, const VT_GAME *g, BOOL focus)
{
    LPCWSTR badge = g->kind == VT_KIND_DOS ? L"DOS" :
                    g->kind == VT_KIND_WIN16 ? L"16-BIT" : L"32-BIT";
    if (focus) {
        Fill(dc, x - 8, y - 8, w + 16, h + 16, RGB(0x1E, 0x40, 0x28)); /* glow */
        Fill(dc, x - 4, y - 4, w + 8, h + 8, C_GLOW);                   /* ring */
    }
    Fill(dc, x, y, w, h, C_BEZEL);
    Fill(dc, x + 8, y + 8, w - 16, h - 56, RGB(0x16, 0x22, 0x1A));
    Text(dc, g_fontBody, focus ? C_GLOW : C_PUTTY, x + 14, y + 14, w - 28, h - 70,
         g->title, DT_CENTER | DT_VCENTER | DT_WORDBREAK);
    Fill(dc, x + 8, y + h - 40, 70, 28, C_PUTTY);
    Text(dc, g_fontSmall, C_BEZEL, x + 8, y + h - 40, 70, 28, badge, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (focus)
        Fill(dc, x + w - 22, y + h - 32, 12, 12, C_GLOW); /* power-LED accent */
}

static void Paint(HDC screen, int cw, int ch)
{
    HDC dc = CreateCompatibleDC(screen);
    HBITMAP bmp = CreateCompatibleBitmap(screen, cw, ch), old;
    int barH = 72, statusH = 56, tileW = 180, tileH = 240, gap = 28;
    int shelfH = tileH + 70, s, y, visible;
    SYSTEMTIME st;
    WCHAR clock[16];

    old = (HBITMAP)SelectObject(dc, bmp);
    if (ch < 640) { tileW = 132; tileH = 176; shelfH = tileH + 60; } /* safe mode */

    /* putty top bar */
    Fill(dc, 0, 0, cw, barH, C_PUTTY);
    Bevel(dc, 0, 0, cw, barH, TRUE);
    Text(dc, g_fontHead, C_BEZEL, 24, 0, cw / 2, barH, L"VetTV Retro OS", DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    GetLocalTime(&st);
    wsprintfW(clock, L"%02u:%02u", st.wHour, st.wMinute);
    Text(dc, g_fontBody, C_BEZEL, cw - 224, 0, 200, barH, clock, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    /* CRT content area */
    Fill(dc, 0, barH, cw, ch - barH - statusH, C_PHOSPHOR);
    for (y = barH; y < ch - statusH; y += 3)
        Fill(dc, 0, y, cw, 1, RGB(0x0B, 0x10, 0x0D)); /* faint scanlines */

    y = barH + 24;
    visible = (ch - barH - statusH - 24) / shelfH;
    if (visible < 1) visible = 1;
    for (s = 0; s < SHELF_COUNT; s++) {
        int i, x, perRow, start;
        if (g_focusShelf >= visible && s < g_focusShelf - visible + 1)
            continue; /* scroll shelves so the focused one is on screen */
        if (y + shelfH > ch - statusH + 8)
            break;
        Text(dc, g_fontBody, s == g_focusShelf ? C_GLOW : C_SHADOW, 40, y, cw - 80, 36,
             g_shelfName[s], DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        perRow = (cw - 80 + gap) / (tileW + gap);
        if (perRow < 1) perRow = 1;
        start = g_focusCol[s] >= perRow ? g_focusCol[s] - perRow + 1 : 0;
        x = 40;
        if (!g_shelfLen[s])
            Text(dc, g_fontSmall, C_SHADOW, 40, y + 44, cw - 80, 30, L"Nothing here yet.",
                 DT_LEFT | DT_SINGLELINE);
        for (i = start; i < g_shelfLen[s] && i < start + perRow; i++) {
            DrawTile(dc, x, y + 44, tileW, tileH, &g_lib.games[g_shelfIdx[s][i]],
                     s == g_focusShelf && i == g_focusCol[s]);
            x += tileW + gap;
        }
        y += shelfH;
    }

    /* status bar */
    Fill(dc, 0, ch - statusH, cw, statusH, C_PUTTY);
    Bevel(dc, 0, ch - statusH, cw, statusH, TRUE);
    Text(dc, g_fontSmall, C_BEZEL, 24, ch - statusH, cw - 48, statusH, g_status,
         DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

    BitBlt(screen, 0, 0, cw, ch, dc, 0, 0, SRCCOPY);
    SelectObject(dc, old);
    DeleteObject(bmp);
    DeleteDC(dc);

    if (!g_firstFrameLogged) {
        g_firstFrameLogged = TRUE;
        VtLog(L"first library frame ms_since_boot=%lu", GetTickCount()); /* M4 #5 */
    }
}

static DWORD WINAPI LaunchThread(LPVOID p)
{
    VtLaunch(&g_lib.games[(int)(INT_PTR)p], &g_result);
    PostMessageW(g_wnd, WM_APP_DONE, 0, 0);
    return 0;
}

static void StartGame(void)
{
    HANDLE t;
    int s = g_focusShelf;
    if (g_running || !g_shelfLen[s])
        return;
    g_launchIdx = g_shelfIdx[s][g_focusCol[s]];
    g_running = TRUE;
    wsprintfW(g_status, L"Starting %s...", g_lib.games[g_launchIdx].title);
    InvalidateRect(g_wnd, NULL, FALSE);
    UpdateWindow(g_wnd);
    t = CreateThread(NULL, 0, LaunchThread, (LPVOID)(INT_PTR)g_launchIdx, 0, NULL);
    if (!t) {
        g_running = FALSE;
        lstrcpyW(g_status, L"Couldn't start the game.");
        return;
    }
    CloseHandle(t);
    ShowWindow(g_wnd, SW_HIDE); /* no rendering while a game runs */
}

/* Size the launcher to the whole screen and put it above the taskbar.
 * Called on every show and on WM_DISPLAYCHANGE: on a first boot onto AHCI the
 * display mode can change after we were created, which left the taskbar over
 * our bottom edge (qa3e). */
static void CoverScreen(void)
{
    SetWindowPos(g_wnd, HWND_TOPMOST, 0, 0,
                 GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                 SWP_SHOWWINDOW);
}

static void GameDone(void)
{
    WCHAR err[512];
    g_running = FALSE;
    if (VtErrorText(&g_result, err, 512))
        lstrcpynW(g_status, err, 512);
    else if (g_result.seconds >= 60)
        wsprintfW(g_status, L"Played %s for %lu min.", g_lib.games[g_launchIdx].title, g_result.seconds / 60);
    else
        wsprintfW(g_status, L"Back from %s.", g_lib.games[g_launchIdx].title);
    CoverScreen();
    SetForegroundWindow(g_wnd);
    InvalidateRect(g_wnd, NULL, FALSE);
}

static void ExitToDesktop(void)
{
    SetWindowPos(g_wnd, HWND_NOTOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_HIDEWINDOW);
    ShowWindow(g_wnd, SW_HIDE); /* keep running; --activate brings it back */
    VtLog(L"exit to desktop");
}

static void Activate(void)
{
    if (g_running)
        return;
    CoverScreen();
    SetForegroundWindow(g_wnd);
}

/* Ctrl+Shift+F12 ends the running game (SHELL.md section 4). PS/2-safe. */
static LRESULT CALLBACK KbHook(int code, WPARAM wp, LPARAM lp)
{
    if (code == HC_ACTION && g_running && (wp == WM_KEYDOWN || wp == WM_SYSKEYDOWN)) {
        const KBDLLHOOKSTRUCT *k = (const KBDLLHOOKSTRUCT *)lp;
        if (k->vkCode == VK_F12 && (GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
            (GetAsyncKeyState(VK_SHIFT) & 0x8000)) {
            VtLaunchAbort();
            return 1;
        }
    }
    return CallNextHookEx(g_kbHook, code, wp, lp);
}

static void Move(int dShelf, int dCol)
{
    int s = g_focusShelf;
    if (dShelf) {
        int tries;
        for (tries = 0; tries < SHELF_COUNT; tries++) {
            s = (s + dShelf + SHELF_COUNT) % SHELF_COUNT;
            if (g_shelfLen[s]) break;
        }
        g_focusShelf = s;
    } else if (g_shelfLen[s]) {
        g_focusCol[s] = (g_focusCol[s] + dCol + g_shelfLen[s]) % g_shelfLen[s]; /* wraps */
    }
    InvalidateRect(g_wnd, NULL, FALSE);
}

static LRESULT CALLBACK WndProc(HWND w, UINT m, WPARAM wp, LPARAM lp)
{
    switch (m) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        RECT rc;
        HDC dc = BeginPaint(w, &ps);
        GetClientRect(w, &rc);
        Paint(dc, rc.right, rc.bottom);
        EndPaint(w, &ps);
        return 0;
    }
    case WM_DISPLAYCHANGE:
        if (IsWindowVisible(w))
            CoverScreen();
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_TIMER:
        InvalidateRect(w, NULL, FALSE); /* clock, once a minute is plenty */
        return 0;
    case WM_KEYDOWN:
        if (g_running) return 0;
        switch (wp) {
        case VK_LEFT: Move(0, -1); break;
        case VK_RIGHT: Move(0, 1); break;
        case VK_UP: case VK_PRIOR: Move(-1, 0); break;
        case VK_DOWN: case VK_NEXT: Move(1, 0); break;
        case VK_HOME: g_focusCol[g_focusShelf] = 0; InvalidateRect(w, NULL, FALSE); break;
        case VK_END:
            if (g_shelfLen[g_focusShelf]) g_focusCol[g_focusShelf] = g_shelfLen[g_focusShelf] - 1;
            InvalidateRect(w, NULL, FALSE); break;
        case VK_RETURN: case VK_SPACE: StartGame(); break;
        case VK_F5: Reload(); InvalidateRect(w, NULL, FALSE); break;
        case VK_F10: ExitToDesktop(); break;
        }
        return 0;
    case WM_SYSKEYDOWN:
        if (wp == VK_F10) { ExitToDesktop(); return 0; } /* F10 arrives as a system key */
        break;
    case WM_LBUTTONDOWN:
        /* Only a click on the visible, active launcher starts a game; after
           F10 a stray click (e.g. meant for the taskbar) must not. */
        if (!g_running && IsWindowVisible(w) && GetForegroundWindow() == w)
            StartGame();
        return 0;
    case WM_APP_DONE:
        GameDone();
        return 0;
    case WM_CLOSE:
        ExitToDesktop(); /* never quit by accident; the shell stays resident */
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(w, m, wp, lp);
}

static void AttachCli(void)
{
    /* GUI program: print to the caller's console for --classify */
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        _wfreopen(L"CONOUT$", L"w", stdout);
    }
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE prev, LPWSTR cmdLine, int show)
{
    WNDCLASSW wc;
    MSG msg;
    HANDLE mutex;
    WCHAR data[MAX_PATH];
    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    (void)prev; (void)cmdLine; (void)show;

    SetErrorMode(SEM_FAILCRITICALERRORS); /* empty CD/USB drives: no dialogs */

    if (argv && argc >= 3 && !lstrcmpiW(argv[1], L"--classify")) {
        AttachCli();
        return VtClassifyCli(argv[2]);
    }

    mutex = CreateMutexW(NULL, TRUE, VT_MUTEX_NAME);
    if (mutex && GetLastError() == ERROR_ALREADY_EXISTS) {
        /* Second start (or --activate from the desktop shortcut): wake the
           running copy and leave. */
        HANDLE e = OpenEventW(EVENT_MODIFY_STATE, FALSE, VT_EVENT_NAME);
        if (e) { SetEvent(e); CloseHandle(e); }
        return 0;
    }
    g_activate = CreateEventW(NULL, FALSE, FALSE, VT_EVENT_NAME);

    VtLaunchInit();
    VtDataDir(data, MAX_PATH, &g_dataFallback);
    VtLog(L"vettvshell start, data=%s fallback=%d", data, g_dataFallback);

    g_fontHead = CreateFontW(34, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0,
                             ANTIALIASED_QUALITY, 0, L"Fixedsys Excelsior 3.01");
    g_fontBody = CreateFontW(26, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0,
                             ANTIALIASED_QUALITY, 0, L"Source Sans Pro");
    g_fontSmall = CreateFontW(20, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0,
                              ANTIALIASED_QUALITY, 0, L"Source Sans Pro");

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.lpszClassName = L"VetTVShell";
    RegisterClassW(&wc);

    Reload();
    g_wnd = CreateWindowExW(WS_EX_TOPMOST, wc.lpszClassName, L"VetTV Launcher", WS_POPUP,
                            0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
                            NULL, NULL, inst, NULL);
    if (!g_wnd)
        return 1;
    ShowWindow(g_wnd, SW_SHOW);
    CoverScreen(); /* the taskbar may have come up after we were created */
    SetForegroundWindow(g_wnd);
    SetTimer(g_wnd, 1, 30000, NULL);
    /* --no-kbhook: debug switch so QA/Build can rule the quit-combo hook in
       or out of keyboard problems. The quit combo is unavailable with it. */
    if (argv && argc >= 2 && !lstrcmpiW(argv[1], L"--no-kbhook")) {
        VtLog(L"keyboard hook disabled by --no-kbhook; quit combo unavailable");
    } else {
        g_kbHook = SetWindowsHookExW(WH_KEYBOARD_LL, KbHook, inst, 0);
        if (!g_kbHook)
            VtLog(L"keyboard hook failed error=%lu; quit combo unavailable", GetLastError());
    }

    for (;;) {
        DWORD r = MsgWaitForMultipleObjects(1, &g_activate, FALSE, INFINITE, QS_ALLINPUT);
        if (r == WAIT_OBJECT_0) {
            Activate();
            continue;
        }
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT)
                return (int)msg.wParam;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
}
