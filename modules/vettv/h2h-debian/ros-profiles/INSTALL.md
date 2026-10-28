# ReactOS h2h payload (for bk-ros-q64-base overlays): LOCAL ONLY, not for release

Staged 2026-10-28 by RetroOS DOSBox-X Integration. Per-file hashes are in SHA256SUMS.

## Contents -> guest target paths
| Payload path | Guest path | Pin |
|---|---|---|
| VetTV/dosbox-x/ | C:\VetTV\dosbox-x\ (dosbox-x.exe = dosbox-x_XPx86_SDL2.exe renamed) | dosbox-x-v2026.10.01, zip c2c3223d…4b4467, exe 2fb2be63…2c70fad (deps.lock) |
| VetTV/winevdm/ | C:\VetTV\winevdm\ (otvdmw.exe) | otvdm-for-old-windows-v0.9.0.zip 79b1a4f9…937205 (CI #2446; QA only, GPL source commit unconfirmed). WIN.INI/SYSTEM.INI/MIDIMAP.CFG are padded, as on the QA ISO |
| VetTV/profiles/by-hash, by-name | C:\VetTV\profiles\ | generated from modules/vettv/h2h-debian/dosbox-x by ros-profiles/gen.py |
| vetvdm.exe | C:\ReactOS\System32\vetvdm.exe (replaces the image's copy) | b2 build 19871eb4…7930 (vettv/vetvdm-log-b2 a8da8537ca) |
Game files are not included. Copy them into the overlay yourself (e.g. C:\GAMES\<game>\).

## Getting it into an overlay
1. On the box: `xorriso -as mkisofs -R -J -V VETTVH2H -o ros-h2h-payload.iso ros-payload/` (ros-h2h-payload.iso is already built next to this folder)
2. Boot your own overlay of bk-ros-q64-base with `-drive file=ros-h2h-payload.iso,media=cdrom,if=ide,index=2`.
3. In ReactOS cmd (the CD is usually D:):
       xcopy /E /I /Y D:\VetTV C:\VetTV
       copy /Y D:\vetvdm.exe %SystemRoot%\System32\vetvdm.exe
   (If vetvdm.exe is in use, reboot and copy before launching anything.)
4. Check: `certutil` isn't available, so compare file sizes against SHA256SUMS, or hash on the box after shutdown.

## Expected logs (Lure, by-hash)
- Per-launch folder %TEMP%\vetvdm\<pid>-<tick>\vetvdm.log, written and flushed before DOSBox-X starts:
      launch profile=lure-of-the-temptress (by-hash)
  The same folder gets dosbox-x.out (DOSBox-X stdout and stderr). The folder is removed when DOSBox-X exits normally, unless vetvdm runs with --keep-conf. A power-off with the game running leaves it in place.
- Shared C:\VetTV\logs\vetvdm.log (after exit): `... program="...LURE.EXE" sha256=fdbc0003… kind=dos layer=dosboxx profile=lure-of-the-temptress exit=... dosbox="C:\VetTV\dosbox-x\dosbox-x.exe" ...`

## Profile caveats
- vetvdm only uses a by-name profile when match.size is set. Only JAZZ, LURE and TYRIAN2K have a size (they also have by-hash). The other 8 (DOOM, WOLF3D, KEEN4E, DUKE3D, RAP, PINBALL, JILL1, SKY) won't match on ReactOS until their EXE sha256/size is added. On Debian they match by name alone, so the sides differ there.
- Display matches: vetvdm forces fullscreen=true, autolock=true and output=surface, and the Debian base.conf now sets the same three (Lead ruling 2026-10-28). Sound, cycles, memory, machine and CPU settings are identical too.
