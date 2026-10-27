# Launch hook rev 6b on combined core ISO 72ef305c03: QEMU results, 2026-10-19 ~12:00 PM CT
ISO: wt/core-wire/output-core bootcdregtest.iso d36d1fa9…88b8 (copied to iso/bootcdregtest-72ef305c03.iso, sha re-verified; bootcd.iso 0458692d…96ce also verified).
Contents: core-wire + rev 6b 1c01e5ce3c (GetBinaryTypeW==SCS_WOW_BINARY) + 86a0fdf776. QEMU TCG pentium3, VM tags qa-core-s1..s5.
Image ships: real System32\vetvdm.exe (200704 B); NO HookEnabled key by default; NO C:\VetTV\dosbox-x\dosbox-x.exe; NO WineVDM.
Run 6 (qa-core-s3, run6/qa6.cmd): hookstub e615be4c swapped in for H5/H13/H16, real vetvdm restored for H15.
Run 7 (qa-core-s5, run6/qa7.cmd): fake otvdmw.exe (run6/fake.c, logs own path) to see which WineVDM vetvdm picks. s4 hung at desktop before Startup ran; killed, rerun as s5.
Targets: SKI.EXE 0b97b99f (real Win16 NE, local VM only), dos3.exe ac3fb62e, type5.exe 5073f9b9 = SYNTHETIC STUB (MZ + NE header, ne_exetyp=5, no code; no freeware Borland RTM/DPMI16 program with a clear license was found quickly).
Evidence: run6/out/res6.txt, res7.txt, res7fake.txt, h/*.log.

| Check | Result | Evidence |
|---|---|---|
| H2 sanity | PASS | 1 line kind=dos, EL 7 |
| **H5 Win16 SKI.EXE** | **PASS** | `--from-hook --kind=win16 --image="C:\qa\SKI.EXE"` (was kind=dos on rev 5) |
| H5-t5 Borland type-5 (synthetic stub) | PASS | `--kind=dos --image="C:\qa\type5.exe"` |
| **H13b OTVDMW.EXE caller + Win16** | **PASS** | no stub line; went to stock NTVDM (WOW16 "install WineVDM" dialog, dismissed key:ret) |
| H13a myotvdm.exe caller + Win16 | PASS | not skipped, 1 line kind=win16 |
| **H16c otvdm.exe caller + Win16** | **PASS** | no stub line, stock path |
| H16a otvdm caller + DOS | PASS | 1 line kind=dos |
| H16-t5 otvdm caller + type-5 | PASS | 1 line kind=dos (treated as DOS, not skipped) |
| H15a DOS exit code via DOSBox-X | BLOCKED | C:\VetTV\dosbox-x\dosbox-x.exe missing in image; vetvdm.log: `kind=dos layer=ntvdm ... exit=0xE0000005 warn=layer-fallback` (dos3 fixture fails NTVDM) |
| H15b exit 0 / no stale ERRORLVL.TXT | BLOCKED | same missing path |
| H15c no console from shell/Explorer | NOT RUN | needs vettvshell (H17) + DOSBox-X |
| H15d CLI without --from-hook | PARTIAL | vetvdm.log `caller=cli ... layer=ntvdm`; console check needs DOSBox-X |
| H15e WineVDM lookup | PASS | bundled C:\VetTV\winevdm\otvdmw.exe used first; `kind=win16 layer=winevdm exit=5` returned to cmd; with bundled removed -> C:\Program Files\otvdm\otvdmw.exe |
| H15f VETTV_VDM_BYPASS=1 | PASS | no effect, still hooked + winevdm |
| H15g1 WineVdmPath valid | PASS | C:\qa\alt\otvdmw.exe wins over bundled |
| H15g2 WineVdmPath missing | KNOWN GAP (confirmed) | skips bundled VetTV\winevdm, goes to %ProgramFiles%\otvdm\otvdmw.exe |

Verdict: rev 6b fixes H5 and its knock-ons (H13b, H16c); type-5 stays DOS. Hook part is merge-ready from QA side.
H15 DOSBox-X legs blocked until the image ships C:\VetTV\dosbox-x\dosbox-x.exe. vetvdm.log has no WineVDM path field (had to use a fake otvdmw to see it); suggest logging it.
