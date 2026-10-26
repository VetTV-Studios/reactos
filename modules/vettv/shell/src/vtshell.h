/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * vettvshell - VetTV Retro OS game launcher (M4, first slice).
 * Plain Win32 + GDI, NT 5.x exports only. See lanes/shell-ux/SHELL.md.
 */
#ifndef VTSHELL_H
#define VTSHELL_H

#include <windows.h>
#include <wchar.h>

#define VT_MAX_GAMES 512
#define VT_TITLE_LEN 96

typedef enum {
    VT_KIND_SKIP = 0,        /* not a program, or nothing we list */
    VT_KIND_DOS,
    VT_KIND_WIN16,
    VT_KIND_WIN32,
    VT_KIND_UNSUPPORTED      /* PE for a non-i386 machine (greyed out) */
} VT_KIND;

typedef struct {
    WCHAR title[VT_TITLE_LEN];
    WCHAR folder[MAX_PATH];  /* game folder (cwd root) */
    WCHAR exe[MAX_PATH];     /* full path of the main program */
    WCHAR args[256];
    WCHAR json[MAX_PATH];    /* vettv.game.json, empty if none */
    VT_KIND kind;
} VT_GAME;

typedef struct {
    VT_GAME games[VT_MAX_GAMES];
    int count;
} VT_LIBRARY;

/* classify.c */
VT_KIND VtClassifyFile(LPCWSTR path);
LPCWSTR VtKindName(VT_KIND kind);
int VtClassifyCli(LPCWSTR folder);          /* --classify: CSV on stdout */
BOOL VtPickMainExe(LPCWSTR folder, LPWSTR exeOut, VT_KIND *kindOut);

/* paths.c */
BOOL VtSystemDrive(LPWSTR out, DWORD cch);  /* "C:" from the system folder */
BOOL VtVetvdmPath(LPWSTR out, DWORD cch);   /* <System32>\vetvdm.exe */
BOOL VtDataDir(LPWSTR out, DWORD cch, BOOL *usedFallback);
void VtLog(LPCWSTR fmt, ...);               /* <SystemDrive>\VetTV\logs\shell.log */

/* library.c */
void VtLibraryLoad(VT_LIBRARY *lib);

/* launch.c */
typedef struct {
    DWORD exitCode;
    BOOL started;
    DWORD createError;       /* GetLastError() when !started */
    DWORD seconds;           /* play time */
} VT_LAUNCH_RESULT;

void VtLaunchInit(void);
void VtLaunch(const VT_GAME *g, VT_LAUNCH_RESULT *r);
void VtLaunchAbort(void);                   /* quit combo: end the job */
BOOL VtErrorText(const VT_LAUNCH_RESULT *r, LPWSTR out, DWORD cch);


/* vetvdm exit codes. Mirrors core/vetvdm_core.h on vettv/vetvdm-core, which is
   the source of truth; switch to including it once that branch is merged. */
#ifndef VETVDM_E_NOLAYER
#define VETVDM_E_CMDLINE   0xE0000001u
#define VETVDM_E_NOPROG   0xE0000002u
#define VETVDM_E_NOTDOS    0xE0000003u
#define VETVDM_E_NOLAYER   0xE0000004u
#define VETVDM_E_START     0xE0000005u
#define VETVDM_E_WIN31   0xE0000006u
#define VETVDM_E_PROFILE   0xE0000007u
#define VETVDM_E_CANCEL 0xE0000008u
#define VETVDM_E_NORESULT  0xE0000009u
#define VETVDM_E_INTERNAL  0xE00000FFu
#endif

#endif
