# vetvdm.exe — spec (P-2 draft v0.1)

Owner: RetroOS DOSBox-X Integration · Drafted Fri Oct 2, 2026 (PT) · Status: draft for Lead review
Code location (once the fork exists): `modules/vettv/vetvdm/` · Installed as `%SystemRoot%\System32\vetvdm.exe` (the hook's fixed path; DOSBox-X etc. stay under `%SystemDrive%\VetTV`) · Toolchain: RosBE i386 (GCC 8.4, Mingw-w64), C, plain Win32, NT5.x exports only, no CRT beyond msvcrt.dll, no MSVC runtime.

## 1. Job
`vetvdm.exe` is the one place that decides **how a 16-bit program runs**. It is called by:
1. the kernel32 hook (M2, Build & Kernel) whenever `CreateProcessInternalW` sees a DOS (`STATUS_INVALID_IMAGE_NOT_MZ` / `_PROTECT`) or Win16 (`STATUS_INVALID_IMAGE_WIN_16`) image;
2. `vettvshell.exe` (Shell/UX) for library launches with an explicit profile;
3. people, from `cmd.exe`, for testing.

It picks a layer (DOSBox-X, WineVDM, or ReactOS NTVDM), builds the config, starts the layer, waits, and returns the program's exit code.

## 2. Command line
```
vetvdm.exe [options] [--] <program> [program args...]
```
| Option | Meaning |
|---|---|
| `--kind=dos\|win16\|auto` | What the hook detected. `auto` (default) = vetvdm reads the MZ/NE header itself |
| `--layer=dosboxx\|winevdm\|ntvdm\|win31` | Force a layer, overriding policy and profile |
| `--profile=<id or path.json>` | Use a specific profile instead of lookup |
| `--image="<path>"` | Full path of the image the hook resolved (LAUNCH-HOOK.md). Wins over the first token after `--` when both are present. The hook passes the Win32 full path (Build review of `b29ec6542b`: `lpApplicationName`, not `PathName`). Defensive only: if an NT-style path arrives, vetvdm normalises `\??\C:\…` to `C:\…` and `\??\UNC\srv\…` to `\\srv\…` before any Win32 call, DOSBox-X `mount`, log line or SHA lookup; any other `\Device\…` form = 0xE0000001 |
| `--from-hook` | Set by the kernel32 hook. Implies: no UI unless policy says `ask`, and inherit the caller's console/cwd |
| `--caller=shell` | Set by vettvshell. Log-only: vetvdm.log records `caller=shell`. Any other value is a command-line error (`0xE0000001`) |
| `--cwd=<dir>` | Working directory (default: the program's folder, or the caller's cwd with `--from-hook`) |
| `--keep-conf` | Don't delete the generated `dosbox-x.conf` (debugging) |
| `--dry-run` | Print the chosen layer + generated conf/command line and exit 0. **QA uses this for routing tests without starting DOSBox-X** |
| `--log=<file>` | Append a log line (see §7) |
| `--version`, `--help` | |

Everything after `--` (or after the first non-option argument) is passed to the program untouched. vetvdm takes this tail from the raw `GetCommandLineW()` text after the first ` -- `, so the caller's original quoting survives (the hook passes `lpCommandLine` verbatim). Quoting follows the normal `CommandLineToArgvW` rules; vetvdm re-quotes when it builds the DOS `[autoexec]` line (DOS 127-character command tail limit: truncate + log a warning).

## 3. Policy (registry)
Key: `HKLM\Software\VetTV\RetroOS\VDM` (machine) with optional per-user override at `HKCU\Software\VetTV\RetroOS\VDM` (HKCU wins, value by value).

| Value | Type | Default | Meaning |
|---|---|---|---|
| `Enabled` | DWORD | 1 | 0 = the kernel32 hook does nothing (pure upstream behaviour). **The hook reads this itself** so a broken vetvdm can't block 16-bit launches |
| `DosMode` | SZ | `dosboxx` | `dosboxx` \| `ntvdm` \| `ask` |
| `Win16Mode` | SZ | `winevdm` | `winevdm` \| `dosboxx` (= Win3.1 guest) \| `ask` |
| `DosBoxXPath` | EXPAND_SZ | `%SystemDrive%\VetTV\dosbox-x\dosbox-x.exe` | |
| `WineVdmPath` | EXPAND_SZ | *(detected; see open question O-1)* | Path to `otvdm.exe` / `otvdmw.exe` |
| `Win31Path` | EXPAND_SZ | *(empty)* | Folder holding the **user's own** Windows 3.1 install. Empty = the Win3.1 guest is unavailable and vetvdm falls back to WineVDM with a warning |
| `ProfileDirs` | MULTI_SZ | `%SystemDrive%\VetTV\profiles` | Searched in order |
| `ConfTemplate` | EXPAND_SZ | `%SystemDrive%\VetTV\vetvdm\default.conf` | Base DOSBox-X conf |
| `PassThrough` | MULTI_SZ | *(installer stubs list)* | EXE names / SHA-256s that always go to NTVDM (e.g. 16-bit setup stubs inside 32-bit installers). Covers BRIEF risk "hook breaks installers" |
| `LogLevel` | DWORD | 1 | 0 off, 1 errors+launches, 2 verbose |

`ask` shows a tiny Win32 chooser (DOSBox-X / NTVDM, remember for this program). With `--from-hook` and no desktop (service/session 0) `ask` falls back to the default.

## 4. Profile lookup
Order, first match wins:
1. `--profile`.
2. `vettv.game.json` next to the program or in its parent folder (Shell/UX schema; vetvdm reads `layer`, `exe`, `args`, `profile`, and an optional `dosbox` object).
3. `<ProfileDirs>\by-hash\<sha256 of the EXE>.json` (compat DB, shipped by us, built from QA results).
4. `<ProfileDirs>\by-name\<UPPERCASE EXE NAME>.json` (weaker; only used when the file size also matches `match.size`).
5. Built-in defaults.

Profile JSON (subset; full schema shared with Shell/UX in SHELL.md):
```json
{
  "id": "doom-sw-1.9",
  "match": { "sha256": "…", "size": 709905, "name": "DOOM.EXE" },
  "layer": "dosboxx",
  "dosbox": {
    "machine": "svga_s3", "memsize": 16, "cycles": "max",
    "sbtype": "sb16", "gus": false, "mpu401": "intelligent",
    "output": "opengl", "fullscreen": true,
    "mounts": [ { "drive": "C", "path": ".", "type": "dir" } ],
    "autoexec": [ "DOOM.EXE %ARGS%" ],
    "extra": { "dos": { "ver": "6.22" } }
  },
  "notes": "QA G001"
}
```

## 5. Conf generation (DOSBox-X)
1. Start from `ConfTemplate` (ours, shipped with the payload — not upstream's `dosbox-x.conf`). ReactOS-safe defaults: `output=surface` (QA run on M0-5, 5:19 PM PT Oct 2 2026: `opengl` draws garbage then crashes in ReactOS's own OpenGL, and the Direct3D default prints "no 3D acceleration". See `lanes/compat-qa/runs/m0-5-dbx/RESULTS.md`. `opengl` can be enabled per profile only), `fullscreen=true`, `sbtype=sb16`, `machine=svga_s3`, `memsize=16`, `cycles=auto`, `[sdl] autolock=true`, `[dosbox] quit warning=false`, `[log] logfile=` set to the temp log.
2. Overlay profile values.
3. Mount: the program's folder as `C:` (or profile mounts; `.`/relative paths resolve against the profile file). CD images via `imgmount d <file> -t iso`.
4. `[autoexec]`: `C:`, `cd <subdir>`, the program + args, then `exit`. Nothing else, so closing the game closes DOSBox-X (M2 exit criterion 1).
5. Write to `%TEMP%\vetvdm\<pid>-<tick>\dosbox-x.conf`, launch `dosbox-x.exe -conf <file> -exit -nopromptfolder -fastlaunch`, delete the folder afterwards unless `--keep-conf`. (Exact flag set to be checked against 2026.10.01 `--help` once QA has it running.)

### 5.1 Locked defaults (Shell/UX quit contract — Lead decision, Oct 2, 2026 PT)
These values are **forced by vetvdm after the profile overlay**. A template or per-game profile cannot override them; if one tries, vetvdm ignores it and writes a warning to the log.
| Setting | Value | Why |
|---|---|---|
| `[dosbox] hostkey` | `mapper` (host key = F11 on Windows) | `hostkey=ctrlshift` would turn Ctrl+Shift into the host key and collide with the launcher's Ctrl+Shift+F12 quit combo |
| Gamepad hotkeys | none (DOSBox-X 2026.10.01 maps no controller buttons to hotkeys by default; vetvdm ships no `mapperfile` that binds Back/Start) | Back+Start belongs to vettvshell |
| Ctrl+F9 "shutdown" binding | left at the upstream default | Keyboard hard-quit fallback |
| Process containment | vetvdm creates a Job Object with `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE` and puts the layer process in it. vettvshell can end the job (or close vetvdm's handle) for a hard stop | Hard-stop fallback that doesn't depend on any key binding (Job Objects confirmed in `ntoskrnl/ps/job.c` by Build & Kernel; verified at runtime by QA on the M0-8 VM, Oct 2 2026 PT: killing the parent ended dosbox-x.exe within 3 s, see `lanes/compat-qa/runs/m0-8-job/RESULTS.md`. No parent-watch fallback needed) |

Regression check (QA): `vetvdm.exe --dry-run` output must show `hostkey=mapper` and no `mapperfile=` line for every profile in the compat DB. The `--dry-run` check must also confirm `hostkey` is written under `[dosbox]`, because DOSBox-X ignores it in `[sdl]` without any warning (verified against the 2026.10.01 `dosbox-x.reference.conf`).

**Win3.1 guest** (`win31` layer, user-supplied media only): mount `Win31Path` as `C:`, the game folder as `D:`, `[autoexec]` = `C:\WINDOWS\WIN.COM D:\<exe>`, then `exit`. vetvdm never copies, downloads, or ships any Windows file; it just checks that `WIN.COM` exists in the folder the user configured.

**WineVDM**: `"<WineVdmPath>" "<program>" <args>`, with the same cwd. **NTVDM**: re-issue `CreateProcessW(image, original tail)` from vetvdm itself. The launch hook (`vettv/launch-hook` 17cc2c3b42) skips any request whose calling process image is `vetvdm.exe`, so this reaches the real VDM. (Superseded 2026-10-18: the earlier `VETTV_VDM_BYPASS=1` env-var idea is not used; don't set it.)

## 6. Exit codes
vetvdm returns the **program's** exit code where the layer can report it, so `cmd.exe` / `ERRORLEVEL` behaves (M2 exit criterion 4). Its own failures use a reserved high range that DOS programs can't produce (DOS codes are 0–255):

| Code | Meaning |
|---|---|
| 0–255 | Program exit code (DOSBox-X: via `exit` after the program; see note) |
| 0xE0000001 | Bad command line |
| 0xE0000002 | Program not found / unreadable |
| 0xE0000003 | Not a DOS or Win16 image (vetvdm got a PE or garbage) |
| 0xE0000004 | Layer executable missing (path in log) |
| 0xE0000005 | Layer failed to start (`CreateProcess` error in log) |
| 0xE0000006 | Win3.1 guest requested but `Win31Path` empty/invalid |
| 0xE0000007 | Profile parse error |
| 0xE0000008 | User cancelled the `ask` dialog |
| 0xE0000009 | Layer exited without a result (DOSBox-X closed or crashed before writing ERRORLVL.TXT) |
| 0xE00000FF | Internal error |

Shell/UX message for any 0xE00000xx: one line naming the meaning above, plus a link to the log. Retro OS log = `%SystemDrive%\VetTV\logs\vetvdm.log` (VETVDM_PATH_SHARED_LOG). `instant.log` is the Instant Mode (Windows host) log only.

Note: DOSBox-X doesn't return the guest program's ERRORLEVEL as its own exit code by default. Plan: the generated `[autoexec]` writes `%ERRORLEVEL%` to a file in the mounted temp folder after the program exits, and vetvdm reads it. If that file is missing (crash, user closed the window), return 0xE0000009 "layer exited without a result". WineVDM's own exit code is passed through.

## 7. Logging
One line per launch to `%SystemDrive%\VetTV\logs\vetvdm.log` (rotated at 1 MB): time (local), PID, caller (hook/shell/cli), program path, sha256, kind, layer, profile id, exit code, duration. QA's smoke scripts grep this.

## 8. Not in scope for v0.1
Per-game save sync, controller remapping (Shell/UX), Win9x guest boot, networking/IPX, auto-download of anything.

## 9. Open questions
- **O-1 (Build & Kernel / QA):** ReactOS i386 kernel32 has no `NtVdm64` support (that's what otvdm's `install.inf` uses, and only for amd64). How does the 0.4.16 `winevdm_setup.exe` make double-clicked Win16 EXEs run today? File associations, a NTVDM shim, or not at all? Answer decides `WineVdmPath` detection and whether our hook is the *first* thing that makes Win16 double-click work.
- **O-2 (Lead / Vet):** WineVDM version: keep upstream's 0.8.1 add-on (what ReactOS ships), or move to 0.9.0 `otvdm-for-old-windows` once its source commit is confirmed? I recommend 0.8.1 for M1/M2 and an A/B in M3.
- **O-3 (QA):** SDL2 vs SDL1 XP build on ReactOS. Default SDL2, fall back to SDL1 if audio/video misbehave.

## RK-XHCI note (2026-10-02)
On Asus GL552VW (Skylake, xHCI-only) ReactOS has no xHCI driver: no USB gamepad reaches DOSBox-X, so vetvdm gamepad mapping is out of scope for R-T04 real-hardware runs; keyboard (PS/2 internal) path still tested. In R-T04D (FreeDOS), USB kbd/mouse depend on BIOS Legacy USB Support; USB gamepads unsupported in DOS (expected).
