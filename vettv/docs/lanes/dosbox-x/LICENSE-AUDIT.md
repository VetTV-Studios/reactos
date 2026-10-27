# VetTV Retro OS: 16-bit layer licence audit (QA payload)
Updated 2026-10-19 12:40 PM CT. Project licence target: GPLv2-or-later compatible. Only the QA ISO contains binaries; CI and the repo never do.

| Component | Version / hash | Licence | Compatible | Ships in |
|---|---|---|---|---|
| DOSBox-X (XP SDL2 build) | 2026.10.01, dosbox-x.exe 2fb2be63…2c70fad | GPL-2.0-or-later (COPYING included) | Yes | C:\VetTV\dosbox-x\ (QA ISO) |
| WineVDM (otvdm-for-old-windows) | 0.9.0 CI #2446, zip 79b1a4f9…937205 | GPL-2.0 (LICENSE included); Wine-derived parts LGPL-2.1+ | Yes | C:\VetTV\winevdm\ (QA ISO only; source commit unconfirmed, so release is blocked) |
| jsmn | 1.1.0 fdcef3eb | MIT | Yes | compiled into vetvdm.exe |

Excluded: inpout32.dll, DOSBox-X fonts (pending review), any Microsoft file (VCRUNTIME etc.), all games.
