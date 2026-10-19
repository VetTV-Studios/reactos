# vettvshell

VetTV Retro OS game launcher (milestone M4). Full spec: `lanes/shell-ux/SHELL.md`.

First slice:
- Library screen with three shelves (DOS & 16-bit, 32-bit PC, All), beige-crt look, plain GDI.
- Keyboard: arrows, PgUp/PgDn, Home/End, Enter or Space to play, F5 refresh, F10 back to the desktop.
- Games are folders under `<drive>:\Games` on any fixed, CD or removable drive (not A:/B:). A `vettv.game.json` wins; otherwise the main EXE is picked and classified from its header.
- Win16 is decided by kernel32 `GetBinaryTypeW` (`SCS_WOW_BINARY`), the same rule as launch hook rev 6b.
- DOS and Win16 run through `<System32>\vetvdm.exe --kind=... --image=...` with no console window; Win32 games start directly. Everything runs in a kill-on-close job.
- Ctrl+Shift+F12 ends the running game.
- Single instance (`Local\VetTV.Shell`); starting it again brings the running copy back.
- `vettvshell.exe --classify <folder>` prints `path,kind` CSV for QA.
- `<SystemDrive>\VetTV\logs\shell.log` records launches, exit codes and the first library frame tick.

Not yet: gamepad, game detail screen, Add-game wizard, settings, crash-loop fallback.
