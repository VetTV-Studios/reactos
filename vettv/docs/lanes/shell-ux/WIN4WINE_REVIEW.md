# win4wine design review (design ideas only) — draft, pre-release
Source: https://github.com/mikuta0407/win4wine (README read Oct 13; no release yet; recheck after Oct 14 release).
Licence: LGPL-2.1-or-later for repo code/patches; wayland/patches/labwc GPL-2.0-only; wlroots patches MIT.
Rule: we reuse NO code and NO XP files/assets/explorer.exe. Ideas only, reimplemented in our own words/code.

## Ideas worth borrowing
1. Bring-your-own-media: setup takes `--media <iso>`, never downloads vendor files, extracts only needed files
   into git-ignored dirs (`localres/`, `.local/`). Matches our BYO-disc model; mirror: user-supplied payloads live
   outside the repo, hash-checked, never committed.
2. Media auto-detection: language/edition detected from the ISO, rejects unsupported variants with a clear message.
   Idea for our Win 3.1/9x disc check UX (pairs with DOSBox-X header classifier).
3. Session supervisor: shell runs under a supervisor; repeated crashes fall back to a plain shell + terminal.
   Adopt for S0: crash-loop counter -> fall back to stock ReactOS explorer with a "launcher failed" notice.
4. Free fallback path: no ISO -> ReactOS shell fetched from official release. Confirms our free-by-default stance.
5. Keyboard parity: Win key, Ctrl+Esc, Win+R, Win+E all wired. Useful for the RK-XHCI keyboard-only pass (R-T04).
6. DPI presets (96/120/144) chosen in one place. Candidate for 5K/1080p hosts.

## Not applicable / avoid
- XP explorer.exe, shell32.dll, Luna, welcome screen: forbidden for us (trademark/asset rule).
- Wine/X11/Wayland bridge-wm: Linux-host specific, not relevant to ReactOS i386.

## Follow-up
- After Oct 14 release: read docs/ (supervisor, reactos.md) for crash-fallback details; update this note.

## Release check (Oct 18)
Releases v0.7.0 (Oct 13 CT) through v0.11.0 (Oct 17 CT) are out; v0.8+ add a Windows 7-style shell and Wayland session. Nothing new for us beyond the ideas above; still design-only, no code or assets reused.
