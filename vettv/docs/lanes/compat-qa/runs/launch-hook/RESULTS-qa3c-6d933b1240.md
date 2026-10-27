# Launch hook / shell QA: qa3c, vettv/qa-payload-3c 6d933b1240
Image: `3097cc548f` + shell `66928988a6` (old launcher, not `3cd71db60a`) + vetvdm 3c `aeaaf3ef02`.
ISOs copied to iso/ and sha256 checked: bootcd `bed5321a…10cb9` OK, regtest `b85e5854…b66b` OK.
QEMU 10.0.13 TCG pentium3, 1024 MB, IDE raw `vm/qa-3c.raw`, **-rtc base=localtime**, one case per boot (vm/qa3c.sh, vm/qa3c-case.sh, vm/qa3c-boot.py, qa3c/*.cmd). VMs qa-3c-*. Times are CDT.
Library: SkiFree (G011), Jazz ep1 (G008), Lure 1.1 (G010), DosTest (synthetic dos3.exe, exits 3).

| # | Test | Result | Evidence |
|---|---|---|---|
| 1 | Clock is Central local time | PASS | Taskbar shows 3:08 PM at box 15:08:44 CDT (vm/qa3c-b1clock-t180.png); guest `%TIME%` 15:05:57 at boot; shell header 15:10 at box 15:11:04 |
| 2a | SkiFree from shell (Win16/WineVDM) | PASS | SkiFree window up (vm/qa3c-b2ski-e20.png); no 0xE0000005. Ended by quit combo, so shell.log `exit code=0xE0000009 seconds=38` (old shell kills vetvdm with 0xE0000009) |
| 2b | Jazz from shell | Expected FAIL (routing), no E_START | Routed to WineVDM, otvdm "Interrupt 0D #GP (11E7:046C)" (vm/qa3c-b3jazz-e40.png); vetvdm `kind=win16 layer=winevdm exit=0x00000100 duration_ms=72997`; shell "Played Jazz for 1 min." Needs rev 3d |
| 3 | H15c no console from shell | PASS | DosTest/Lure launched from the shell: no console window or console taskbar button (vm/qa3c-b4dos1-e2.png, -e7.png). Jazz shows otvdm's own `C:\Games\Jazz\JAZZ.EXE` window, which is WineVDM's, not a vetvdm console |
| 4 | Logs append across launches/boots | PASS | vetvdm.log: Jazz 15:17:56 (boot 3), DosTest 15:21:07 (boot 4), DosTest 15:24:12 (boot 5), each a clean exit=3, no stale tail bytes. shell.log: entries from all boots kept (5+ "first library frame" lines). Quit-combo exits write no vetvdm.log line, because vetvdm is terminated |
| 5 | No layer process after quit combo | PASS | SkiFree: vetvdm+otvdmw gone by the next tasklist (15:13:55). Lure: vetvdm+dosbox-x present at 15:31:33, gone at 15:31:47 (quit at 15:31:41) |
| 6 | warn=job-nested | LOGGED (fallback path only) | Normal DOSBox-X/WineVDM launches from the shell: no job-nested (breakaway works). Appears in test 7: `warn=layer-fallback,job-nested` |
| 7 | Missing DOSBox-X → 0xE0000004 + "isn't installed" | FAIL (as built) | Renamed C:\VetTV\dosbox-x (host mren; an in-guest `ren` at startup didn't take effect). DosTest from shell → `layer=ntvdm exit=0xE0000005 duration_ms=735 warn=layer-fallback,job-nested`, shell shows "The DOS/16-bit player didn't start. Details: C:\VetTV\logs\vetvdm.log" (vm/qa3c-b7nolayer-e5.png). Unforced routes fall back to NTVDM (route.c) instead of returning NOLAYER. Dir restored afterwards |

Repaint stall: not seen with SkiFree (38 s) or DosTest. In Lure (DOSBox-X), the screen changed at +10/+20/+30 s, the music-note icon still blinked at +40/+50 s, and the frame was identical from +50 s to +60 s. Because it's a static title screen, this doesn't prove a stall; possible onset ~50 s (vm/qa3c-b6lure-e*.png). The quit combo still worked at +66 s.
Flake: the first boot of the Lure case hung at the desktop (empty taskbar, guest clock stuck at 3:24 PM); retried OK (screens in vm/qa3c-hung/).
Final disk kept: vm/qa-3c.raw. Temp vm/qa-3c-base.raw deleted.
