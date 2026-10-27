# shell-ux — log

- 2026-10-02 ~2:25 PM PT — P-6 drafted: `SHELL.md` v0.1 (UI stack = in-tree Win32/GDI software renderer, screens S0–S6, pad/keyboard/mouse map, quit-game combo, add-game MZ/NE/PE/LE classifier, `vettv.game.json` schema v1). Marked P-6 `review` in TASKS.md. Open: O-S1…O-S5.
- 2026-10-02 3:10 PM PT — Vet picked the visual direction: late-90s beige PC + CRT glow. SHELL.md §8 updated (theme beige-crt); O-S1 closed. Mockup in progress.
- 2026-10-02 3:15 PM PT — O-S2 resolved (Userinit start, Shell=explorer.exe), O-S3 mostly resolved, O-S5 resolved. SHELL.md §2/§10 updated.
- 2026-10-02 3:16 PM PT — Startup changed from Userinit to the HKLM Run key value VetTVShell (ReactOS msgina doesn't split Userinit on commas, per Build & Kernel). SHELL.md §2 updated.
- 2026-10-02 3:20 PM PT — S1 mockups done: mockups/s1-library-beige-crt.png (1280x720), -4x3.png (1024x768), make_s1.py, FONTS.md (VT323, Press Start 2P, Inter; OFL). Open follow-ups: two-shelves-per-row layout at 720p vs spec, tile text below 24 px rule.
- 2026-10-02 4:25 PM PT — Lead decisions applied: shelves 'DOS & 16-bit' / '32-bit PC'; M4 #1 = launcher first after logon (Run key OK). Mockup shelves to be relabeled on next pass. Companion projects: Shell/UX owns all three UIs.
- 2026-10-02 4:35 PM PT — V-9 (Qt 6 VM Player UI plan) and R-9 (Restart in DOS Mode wireframes + 80x25 DOS screens) drafted under lanes/shell-ux/companion/; marked review in companion/TASKS.md. Paper only.
- 2026-10-02 4:45 PM PT — HP rule resolved by Lead (RQ-2/VQ-9 closed). SHELL.md font fallback: ReactOS ships Source Sans Pro + Fixedsys Excelsior.
- 2026-10-02 4:45 PM PT — HP rule resolved by Lead (RQ-2/VQ-9 closed). SHELL.md font fallback: ReactOS ships Source Sans Pro + Fixedsys Excelsior.
- 2026-10-02 5:03 PM PT — QA M0-5: ReactOS has software OpenGL 1.1 only and DirectX9 init fails, so DOSBox-X output=opengl is broken there and QA suggests surface as the default. This confirms the GDI software renderer choice for the shell (no GL/D3D path). Plan-B SDL2 must force the software renderer (SDL_RENDER_DRIVER=software).
- 2026-10-02 5:20 PM PT — I-5 prototype cross-built (i686+x64), XP-era imports only, README with QA steps. Status review.
- 2026-10-02 7:05 PM PT — O-S3 resolved (Job Objects work on M0-8, QA). I-5: added RedrawWindow after overlay hide for QA's 1 px stale line; rebuilt, new hashes in SHA256SUMS.txt.
- 2026-10-02 7:12 PM PT — S1 mockups v2: shelves relabelled to 'DOS & 16-bit' (6, 4 visible) and '32-bit PC'; tile titles now 24-30 px (24 px floor); All peek widened. Sent to Vet by DM.
- 2026-10-02 8:24 PM PT — O-S4 closed (descriptive-only naming); IMP-4 placeholder added to SHELL.md.
- 2026-10-02 21:24 PT: added RK-XHCI keyboard-only note to SHELL.md input stack.
- 2026-10-02 21:42 PT: added DOS exit-line wording table to SHELL.md.
- 2026-10-02 22:03 PT: took HOSTWIN.FLG creation into R-9 Instant Mode packaging.
- 2026-10-02 22:05 PT: R-9 HOSTWIN.FLG goes into a per-host working copy after hash check.
- Oct 13: drafted WIN4WINE_REVIEW.md (design-only, pre-release; recheck after Oct 14).
- Oct 13: added crash-fallback plan to SHELL.md (Run-key launch, stop relaunching; no shell-setting change).
- Oct 18: win4wine releases checked (v0.7.0–v0.11.0), no new ideas; shell paths use env vars only, so upstream 'Users' rename doesn't affect shell-ux.
- Oct 18: SHELL.md profile-path rules for RK-USERS (SHGetFolderPathW only, wizard no My Documents assumption, test both layouts).
- Oct 18: I-5 wrapper session default now via SHGetFolderPathW(CSIDL_LOCAL_APPDATA) (RK-USERS). Rebuilt with distro MinGW-w64 (installed g++-mingw-w64 on box; old toolchain was gone). New hashes i686 1bda1a60…25e4, x64 354d9c88…1c68. Imports: GDI32/KERNEL32/SHELL32/USER32/msvcrt, all XP-era. Still cross-built only, never run.
- Oct 18: SHELL.md: launcher calls vetvdm directly, doesn't depend on launch hook.

## 2026-10-19 09:35 CT - vettvshell M4 first slice
- Worktree wt/shell-m4, branch vettv/shell-m4 from studios/vettv/main e59ac077a5.
- Wrote modules/vettv/shell (main, classify, library, launch, paths, CMakeLists, README, THIRD_PARTY; jsmn v1.1.0 vendored, hash matches vetvdm copy).
- Win16 detection switched to GetBinaryTypeW == SCS_WOW_BINARY to match hook rev 6b (Borland type-5 NE stays DOS).
- Sanity cross-compile i686-w64-mingw32 -Wall -Wextra: clean, 0 warnings. Imports only kernel32/user32/gdi32/shell32/msvcrt, all XP-era (AttachConsole, job objects, GetBinaryTypeW, SHGetFolderPathW). Not yet built in-tree with RosBE; Build wires the add_subdirectory line.
- 12:48 CT: dropped _WIN32_WINNT/WINVER defs from shell CMakeLists (same -Werror redefinition as 86a0fdf776). Local commit; not pushed yet.
