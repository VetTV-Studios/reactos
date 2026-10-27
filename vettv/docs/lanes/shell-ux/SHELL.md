# vettvshell.exe — Shell/UX spec (P-6 draft v0.1)

Owner lane: RetroOS Shell/UX · Drafted Fri Oct 2, 2026 (PT) · Status: **review**
Inputs: BRIEF.md §3.4 and M4, `lanes/dosbox-x/VETVDM_SPEC.md` v0.1.
Code will live in `modules/vettv/shell/` inside the fork (nothing written there yet; fork not approved).

---

## 1. UI stack decision

**Pick: plain Win32 + GDI, software-rendered, C (no C++ runtime), built in-tree with RosBE as a ReactOS module.**

| Option | Verdict | Why |
|---|---|---|
| **Win32 + GDI, own software renderer into a 32-bpp DIB section, `StretchDIBits`/`BitBlt` to screen** | **Chosen** | Works on VBE/VGA fallback with no GPU (the Asus will have none, BRIEF §5.4/§10). Only `user32`/`gdi32`/`kernel32`/`winmm`/`xinput`/`dinput8`, all of which ReactOS ships. Building in-tree (`add_executable` + `set_module_type(vettvshell win32gui UNICODE)`) means the linker only sees ReactOS's own NT5 exports, so we can't accidentally call a Vista+ API. No extra runtime to ship or license |
| SDL2 app | Plan B | DOSBox-X's XP SDL2 build proves an SDL2 can run on ReactOS, but it adds a DLL + version pinning, and SDL's renderer prefers D3D/OpenGL, which are the weakest parts of ReactOS. Revisit only if GDI blits are too slow at 1024×768 |
| Explorer-style shell extension / HTML / .NET / WinUI | Rejected | Not XP-compatible, too heavy, or both |

Third-party code (all GPLv2-compatible, to be listed in `LICENSES.md`):
- `stb_image.h` (public domain / MIT dual) for PNG/JPEG art decoding. Avoids depending on ReactOS's Wine `windowscodecs`/`gdiplus`.
- A small MIT JSON parser (cJSON or jsmn, pick at M4 start) for `vettv.game.json`.
- One bundled **OFL** font for the UI (loaded with `AddFontResourceExW(FR_PRIVATE)`), so we never rely on Microsoft fonts. Fallback: ReactOS already ships Source Sans Pro (OFL) and Fixedsys Excelsior (`FSEX301.ttf`, a free pixel font) in `media/fonts/`. They suit body text and the CRT pixel look, so the shell can run without bundling anything if it has to. The image's `arial.ttf`, `tahoma.ttf` and similar files are ReactOS's own replacements that just reuse the familiar names. Don't refer to them by those names anywhere in the UI.

Rendering rules:
- Render at the **native mode** (VBE commonly gives 800×600, 1024×768, 1280×1024; VMs can do 16:9). Layout uses a logical grid that adapts to 4:3 and 16:9. Minimum supported: 800×600; 640×480 = "safe mode" with fewer tiles.
- Redraw dirty rects only. Target 30 fps for focus animations, ~0% CPU when idle, and **zero rendering while a game is running** (window hidden, only the input thread wakes at 10 Hz).
- Art is pre-scaled once per resolution and cached in memory; no per-frame scaling.

## 2. Process model

- **Startup (decided Oct 2, 2026 with Build & Kernel):** `Winlogon\Shell` stays `explorer.exe`, because ReactOS explorer only becomes the desktop/taskbar when that value contains "explorer.exe". `Winlogon\Userinit` is **not** used either: ReactOS `msgina.c` runs the whole value as one command line and doesn't split it on commas. So vettvshell starts from `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Run`, value `VetTVShell` = `%SystemDrive%\VetTV\shell\vettvshell.exe`, which Build & Kernel sets in `hivesft.inf`. Explorer launches it once it's up as the shell. Nothing upstream is patched. Expect a brief flash of the desktop before the launcher covers it. If that's ever a problem, the fix is a small `userinit.c` patch that starts the launcher sooner (owned by Build & Kernel).
  - The shell opens a borderless, monitor-sized `WS_EX_TOPMOST` window, so explorer's taskbar loading underneath is never visible. If explorer's taskbar ever comes up on top during that race, the shell re-asserts z-order once explorer's tray window exists (via `FindWindow` on its tray class name, checked at startup).
  - For BRIEF M4 #1, "default shell" means the launcher is the first thing you see after logon. Lead confirmed on Oct 2, 2026, and the Run-key approach meets it.
- **Single instance** (named mutex `Local\VetTV.Shell`). A second start with `--activate` signals the running copy (named event) to come to the front. The desktop gets a "VetTV Launcher" shortcut that does this, which is how **"Return to launcher"** works.
- **Exit to desktop** drops `WS_EX_TOPMOST` and hides the shell window. Explorer is already running, so the desktop is just underneath. The shell keeps running hidden. Nothing is killed or restarted.
- Boot-time cost: explorer loads in parallel with the launcher. QA's boot measurement (below) will show whether that threatens the < 60 s target. If it does, the fallback is a "launcher-only" policy that skips explorer's startup items. Explorer itself still starts.
- **Launching a game:**
  - `platform = dos | win16` → `vetvdm.exe --kind=<platform> --profile="<path to vettv.game.json>" --log=... -- "<exe>" <args>` with cwd = game folder. The shell calls vetvdm **directly**, so launcher launches don't depend on the M2 kernel32 hook or on how Win16 double-click works today (VETVDM O-1).
  - `platform = win32` → `CreateProcessW` on the EXE directly, cwd = game folder, plus any `env` from the JSON.
  - The launched process goes into a **kill-on-close Job Object** (implemented in ReactOS `ntoskrnl/ps/job.c`, confirmed working on the M0-8 image by Compat QA on Oct 2, 2026, see `lanes/compat-qa/runs/m0-8-job/`) so "quit game" can end the whole tree (DOSBox-X, WineVDM, launcher stubs). Fallback if ReactOS's job support misbehaves: Toolhelp32 snapshot of child PIDs (O-S3).
  - Shell hides its window, waits on the process handle, records play time, then reappears on the game's detail screen.
- **Boot timing (M4 #5):** the shell writes one line to `%SystemDrive%\VetTV\logs\shell.log` with `GetTickCount()` (ms since boot) when the first library frame is on screen. QA reads that instead of a stopwatch.

## 3. Screens

| ID | Screen | Contents | Notes |
|---|---|---|---|
| S0 | Splash | VetTV Retro OS mark, version string, controller-detected hint | Auto-advances once the library is loaded; never blocks on input |
| S1 | Home / Library | Top bar (logo, clock, pad status). Horizontal **shelves**: Continue (recently played), DOS & 16-bit, 32-bit PC, All. Tiles = 3:4 cover art + title + small platform badge + compat badge (Gold/Silver/… from the compat DB, hidden if N/T) | Empty state: one big "Add games" tile that opens S4 |
| S2 | Game detail | Hero art, title, year, platform, "Runs in DOSBox-X / WineVDM / natively", last played, total play time, compat rating + notes. Actions: **Play**, Options, Remove from library | Remove only edits the library index; never deletes game files |
| S2a | Game options | Layer override (auto / DOSBox-X / WineVDM / Win3.1 guest / NTVDM / native), fullscreen on/off, DOS speed preset (auto/max/fixed), arguments, "Reset to default profile" | Writes back to that game's `vettv.game.json` |
| S3 | Launching / returned | "Starting in DOSBox-X…" card, then the shell hides. On return: "Played 12 min". On failure: plain-English error (table §5) with **Try again** / **Options** | |
| S4 | Add game wizard | 1) Pick a folder (pad-navigable drive/folder list, no common dialog). 2) Scan + classify (§6). 3) Review list: one row per detected game with suggested title, platform and main EXE; toggle include/exclude, edit title (on-screen keyboard), change EXE. 4) Write `vettv.game.json` per game and refresh the library | Default scan root `D:\Games\`. Never copies or moves files |
| S5 | Settings | Display (mode list from `EnumDisplaySettings`), Audio (master volume), Controllers (test screen + shell button remap), Compatibility (`DosMode`, `Win16Mode`, `Win31Path` written to the **HKCU** override that vetvdm already supports), Library folders, About (versions, licenses, source-offer text) | About screen is part of our GPL obligations |
| S6 | Power menu (Start) | Exit to desktop, Restart, Shut down, Cancel | Restart/Shut down use `ExitWindowsEx` after a confirm |
| — | On-screen keyboard | Grid keyboard, pad-driven, used by S2a/S4/S5 | Physical keyboard always works too |

## 4. Navigation map

Focus model: exactly one focused element, a thick high-contrast focus ring (not color-only), focus wraps within a shelf and moves between shelves vertically. Every screen is fully usable with **pad only**, **keyboard only**, or **mouse/touchpad only** (M4 #3).

| Action | Pad (XInput layout) | Keyboard | Mouse |
|---|---|---|---|
| Move focus | D-pad / left stick (repeat after 400 ms, then every 90 ms) | Arrow keys | Hover |
| Select / Play | A | Enter / Space | Left click |
| Back | B | Esc / Backspace | Right click |
| Game options (on a tile) | X | F2 / Menu key | — |
| Search / filter by letter | Y | Type a letter | — |
| Previous / next shelf | LB / RB | PgUp / PgDn | Wheel |
| Jump to top / bottom | LT / RT | Home / End | — |
| Power menu | Start | F10 | Top-bar button |
| Settings | Back/View (tap) | F9 | Top-bar button |
| **Quit running game** | **Hold Back + Start for 2 s** | **Ctrl+Shift+F12** | — |

Quit-game details: while a game runs the shell's input thread keeps polling the pad (XInput works without window focus) and uses a low-level keyboard hook for the key combo. On trigger it ends the game's Job Object and comes back to S2. No overlay is drawn over the game, because we can't reliably draw over fullscreen DOSBox-X or DirectDraw. The combo is deliberately one that games and DOSBox-X's default mapper don't use; Confirmed safe by DOSBox-X Integration, and locked in VETVDM_SPEC §5.1.

Input stack, in priority order:
1. **XInput** (`xinput1_3`, Wine-derived, shipped by ReactOS). Poll at 60 Hz while the shell is visible; re-scan for hot-plugged pads every 1 s.
2. **DirectInput 8** for non-XInput USB pads, mapped to the same logical buttons; mappings editable in S5.
3. **WinMM `joyGetPosEx`** as the last resort (oldest, most likely to work on odd drivers).
4. Keyboard (`WM_KEYDOWN` + low-level hook for the quit combo) and mouse.

RK-XHCI note (Oct 2, 2026, 9:24 PM PT): on the Asus GL552VW, ReactOS has no xHCI driver, so USB pads, mice and keyboards are dead and the touchpad (I2C HID) has no driver. Only the built-in PS/2 keyboard works. So: keyboard-only must be a first-class path, not a fallback. S0 must not show a "connect a controller" prompt or wait for a pad when none is found; it shows keyboard hints instead. The quit combo uses the keyboard hook, which works on PS/2. When the M4 shell lands, R-T04 gets a keyboard-only shell pass (navigate, launch, quit-combo back to S2).

## 5. Error text (vetvdm exit codes → what the user sees)

| Code | Shown text |
|---|---|
| 0xE0000002 | "Can't find the game's program file. It may have been moved." |
| 0xE0000003 | "This file isn't a DOS or 16-bit program." (shell bug: shouldn't happen, log it) |
| 0xE0000004 / 05 | "The DOS/16-bit player didn't start. Check Settings → About for versions." |
| 0xE0000006 | "This game is set to use your own Windows 3.1 install, but none is set up. Add it in Settings → Compatibility." |
| 0xE0000007 | "This game's settings file is damaged. Reset it in Options." |
| 0xE0000009 | No error shown (normal when the player window is closed); just return to S2 |
| other ≥ 0xE0000000 | "Something went wrong starting the game (code 0x…)." |
| Win32 `CreateProcess` failure | "Couldn't start the game (error N)." + `GetLastError` text |

## 6. Add-game classification (S4)

For every `.EXE`, `.COM`, `.BAT` found (recursive, max depth 3, skip > 64 MB files for header reads only):
1. `.COM` or `.BAT` → **DOS**.
2. No `MZ` signature → ignore.
3. Read `e_lfanew` (offset 0x3C) only if `e_lfarlc >= 0x40` and the offset is inside the file; read the 2-byte signature there:
   - `NE` → **Win16**
   - `PE\0\0` → check `Machine`: `0x014C` (i386) → **Win32**; anything else (x64, etc.) → listed as **Not supported** (greyed out)
   - `LE` / `LX` → **DOS** (DOS-extender games, e.g. DOS/4GW titles). Exception: files ending in `.VXD`/`.386` are skipped
   - otherwise → **DOS** (plain MZ)
4. Main-EXE heuristic per folder: prefer a name matching the folder name, then a known name from the compat DB (`by-hash`, then `by-name` + size), then the largest non-excluded EXE. Excluded by default: `SETUP*`, `INSTALL*`, `UNINST*`, `CONFIG*`, `SETSOUND*`, `README*`, `*.BAT` that only calls another EXE in the same folder (keep the EXE).
5. A SHA-256 of the chosen EXE is looked up in `<ProfileDirs>\by-hash\` to prefill layer, profile and compat rating.

This is the logic M4 #4 tests against the M3 titles; QA gets a `vettvshell.exe --classify <folder>` CLI that prints the result as CSV so it can be tested headless.

## 7. `vettv.game.json` (schema v1)

Lives in each game folder (`D:\Games\<Title>\vettv.game.json`). vetvdm reads `layer`, `exe`, `args`, `profile` and `dosbox` (same shape as its profile JSON); the rest is shell-only.

```json
{
  "schema": 1,
  "id": "doom-sw-1.9",
  "title": "Doom (Shareware)",
  "year": 1993,
  "platform": "dos",
  "layer": "auto",
  "exe": "DOOM.EXE",
  "args": "",
  "cwd": ".",
  "env": {},
  "profile": "doom-sw-1.9",
  "dosbox": { "cycles": "max" },
  "art": { "cover": "vettv/cover.png", "hero": "vettv/hero.png" },
  "source": { "url": "<where Vet got it>", "sha256": "<exe hash>", "legal": "shareware" },
  "added": "2026-10-02"
}
```

| Field | Required | Values / notes |
|---|---|---|
| `schema` | yes | `1` |
| `id` | yes | lowercase slug, unique in the library |
| `title` | yes | display name |
| `platform` | yes | `dos` \| `win16` \| `win32` (set by the classifier) |
| `layer` | no | `auto` (default) \| `dosboxx` \| `winevdm` \| `win31` \| `ntvdm` \| `native` |
| `exe` | yes | path relative to the folder |
| `args`, `cwd`, `env` | no | `cwd` relative to the folder |
| `profile` | no | id or relative path of a compat-DB profile |
| `dosbox` | no | inline overrides, same keys as vetvdm's profile `dosbox` object |
| `art` | no | relative paths, PNG/JPEG/BMP. Missing art → generated title card |
| `source` | no | provenance for QA / legal record; never shown publicly |
| `added`, `year` | no | |

Shell-only state (last played, play time, favorites) goes in `D:\Games\.vettv\library.json`, so per-game files stay clean and diff-able.

## 8. Visual identity

- 10-foot UI, original VetTV art only. No Microsoft, Windows, Xbox, PlayStation or Nintendo names, logos, icons, button glyph art, fonts or look-alike trade dress. Pad prompts use neutral glyphs ("A/B/X/Y" letters in plain circles are fine; no copied colors/shapes from any console).
- Shelf names avoid "Windows": **DOS & 16-bit** (DOS, including DOS4GW 32-bit-extender games, plus Win16) and **32-bit PC** (Win32). Decided by Lead, Oct 2, 2026.
- **Art direction (decided by Vet, Oct 2, 2026 PT): late-90s beige PC with a CRT glow.** Theme `beige-crt`:
  - Shell chrome looks like a beige/putty PC: warm off-white panels with bevels, chunky raised buttons, a power-LED style focus accent and a drive-activity blink while loading. All of it is original art, with no copied Win9x chrome or icons.
  - The content area is drawn as a slightly curved CRT screen: a dark phosphor background, soft glow behind the focused tile, and optional faint scanlines and vignette. The scanlines and vignette are pre-rendered overlay bitmaps, so they cost nothing per frame on GDI. Settings → Display has a "CRT effect" toggle (off / subtle / strong).
  - Palette: putty `#D8D0B8`, shadow beige `#A89F86`, bezel `#3A3631`, phosphor background `#0E1410`, glow green `#7CFF9A`, with amber `#FFB347` as an alternate phosphor theme.
  - Type is a bundled OFL pixel-ish display face for headings and a clean OFL sans for body text, at least 24 px at 720p logical.
  - The splash (S0) is a short CRT "power-on" effect, a line expanding to full screen, plus the VetTV mark. It's capped at 1.5 s and skipped on any input.

## 9. Planned code layout (`modules/vettv/shell/`)

```
CMakeLists.txt        # add_executable + set_module_type(win32gui UNICODE), add_cd_file
src/main.c            # WinMain, single-instance, message loop
src/render/           # DIB canvas, rects, text (GDI), image cache, theme
src/ui/               # screens S0–S6, focus engine, on-screen keyboard
src/input/            # xinput, dinput8, winmm, keyboard hook → logical buttons
src/library/          # game JSON load/save, library.json, scan + classify
src/launch/           # vetvdm / CreateProcess, job object, play-time
third_party/          # stb_image.h, JSON parser (+ license files)
assets/               # original VetTV art + OFL font (+ OFL.txt)
```

## 10. Open questions

- ~~O-S1 (Vet): visual direction~~. Decided: beige PC + CRT glow (§8).
- ~~O-S2~~ Resolved: no, so `Shell=explorer.exe` stays and the shell starts from the HKLM `Run` key (`VetTVShell`). `Userinit` isn't comma-split on ReactOS (§2).
- **O-S3** Resolved Oct 2, 2026, 7:03 PM PT. Compat QA killed `vettv-instant-i686.exe` on the M0-8 VM and DOSBox-X ended within 3 s (the log shows `job=1`). The Toolhelp32 fallback stays as defensive code only.
- **O-S4** Shelves decided: "DOS & 16-bit" and "32-bit PC". Still open for Release Steward: is descriptive use of "Windows 3.1" OK in in-app help text (e.g. "your own Windows 3.1 install")? **Closed Oct 2, 2026, 8:23 PM PT:** product names are allowed only as plain descriptions, never as names, logos, icons or button labels (see IMP-4 below).
- ~~O-S5~~ Resolved: no conflicts. vetvdm locks `hostkey=mapper`, binds no Back/Start, keeps Ctrl+F9, and uses kill-on-close jobs (VETVDM_SPEC §5.1). In-game pad mapping is owned by the vetvdm profile; the shell owns navigation + the quit combo. Original question: does DOSBox-X 2026.10.01's default mapper bind Back+Start or Ctrl+Shift+F12? And who owns in-game pad → keyboard mapping for DOS games: vetvdm profile (`dosbox` mapper file) or the shell? Proposal: vetvdm profile owns in-game mapping; shell owns shell navigation + the quit combo only.

## IMP-4: "Import your own files" UI (placeholder, starts after M2)
- The spec is `IMPORT_OWN_FILES.md` (owned by Lead). Shell/UX builds the Settings page, the wizard step, the ownership checkbox (off by default and required) and the remove option, in both vettvshell and Instant Mode.
- Naming rule, from Lead on Oct 2, 2026, 8:23 PM PT, which closes O-S4: "Windows 3.1", "DirectX" and "Microsoft" may appear only as plain descriptions of what the user is importing. They must never appear in a product name, logo, icon or button label. Buttons stay generic, for example "Import files…".

### DOS exit-line wording (Oct 2, 2026, 9:42 PM PT, from QA's M1-USB note)
The AUTOEXEC hint must name what WIN.BAT actually returns to:
- Standalone DOS stick (`vettvdos-usb.img`): "Type WIN to restart. Remove the stick first to leave DOS."
- Two-entry image (DOS on p2 next to Retro OS): "Type WIN to restart into VetTV Retro OS."
- Real Reboot from Instant Mode on a Windows host: "Type WIN to go back to Windows." Allowed as plain description under the O-S4 ruling.
Never say "Windows" when no Windows is on the machine.

## Crash fallback (from win4wine review, Oct 13)
VetTVShell starts from a Run key in hivesft.inf; explorer stays the system shell (Build & Kernel M4 plan).
Crash fallback = stop relaunching the launcher after N crashes in a short window, show a one-line notice; never touch the system shell setting.

## Profile paths (RK-USERS, Oct 18)
- vettvshell.exe resolves every save/config path via SHGetFolderPathW (CSIDL_LOCAL_APPDATA, CSIDL_APPDATA, CSIDL_PERSONAL, CSIDL_COMMON_APPDATA); never env-var expansion, never literal "Documents and Settings"/"Users"/"My Documents".
- Add-game wizard: start folder = last-used folder, else drive list (My Computer equivalent); never assume My Documents exists.
- If SHGetFolderPathW fails: fall back to <exe folder>\data and show a one-line notice; never create a literal %VAR% folder.
- Test on both layouts: pre-sync image (d004b2c119 / Documents and Settings) and post-#9573 image (Users).
- Exception: i5 fade wrapper (vettv_instant.cpp) reads env LOCALAPPDATA, but it runs only on Windows hosts (Instant Mode), where it is always set. Switch to SHGetFolderPathW anyway when next touched.

## Launching titles vs the launch hook (Oct 18)
vettvshell starts DOS/Win16 titles by calling `<SystemRoot>\System32\vetvdm.exe --kind=… --image="…" -- <cmd>` directly (no `--from-hook`, so vetvdm may show its own UI), instead of CreateProcess on the game exe and relying on the kernel32 hook (off unless HookEnabled). Win32 titles: plain CreateProcess.
