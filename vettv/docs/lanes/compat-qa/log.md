# compat-qa — log

## 2026-10-02 PT
- P-4: `compat/matrix.csv` drafted (22 rows). 10 DOS + 1 Win16 archives downloaded to `games-local/_src/` (local only) with SHA-256; redistribution terms read where the archive ships them. Added "Vet-owned" column per Lead.
- Finding: G008 BASS CD and G009 FOTAQ ScummVM freeware packages are data-only (no DOS exe) → can't run in DOSBox-X. Proposing swaps.
- Finding: G011 SkiFree 1.0 from author's site is true NE (Win16). G018 RA Win95 setup is 16-bit (extra WineVDM installer test). G019 free StarCraft 1.18 has mixed XP support → prefer Vet's 1.16 CD.
- P-5: `smoke-test.py` skeleton (QEMU vvfat game drive, QMP send-key launch, 60 s, screendump, blank-screen check). Untested until M0-5.
- M0-6: `VBOX_WINDOWS_HOST.md` (Windows host, Hyper-V/VBS check first).
- 4:2x PM: Swapped G008→Jazz Jackrabbit ep.1, G009→ROTT 1.3 shareware (Lead approved). OMF CD ISO (omf21iso.zip, sha 042ba557…7c26) rejected: fan-made image, readme claims all rights reserved; CD-ROM test will use a self-built ISO of the freeware v2.1 files instead.
- M0-4 ISOs received from Build: bootcd.iso 51dab49e…e67b → for Vet's VBox run (M0-6).
- R-10 drafted: companion/R-10_reboot_matrix.csv (HP 255 G10 out of scope, real hardware gated on R-GATE).
- Lead: iMac out of scope for Real Reboot (kept for Instant Mode/VM Player); R-T06 updated.
- DOSBox-X M0-5 run: SDL2/SDL1 surface OK, opengl broken, default shows ReactOS no-3D modal. See runs/m0-5-dbx/RESULTS.md
- Queue: I-4/I-6 fade wrapper (lanes/shell-ux/companion/i5-fade-wrapper/) — needs a Windows host (Vet's Asus/iMac); optional pre-check: --dry-run inside the ReactOS VM. Doom sound check next on m0-5-dbx VM.
- 2026-10-02 19:02 I-5 Job Object test on M0-8 VM: PASS (runs/m0-8-job/RESULTS.md)
- 2026-10-02 22:23 R-T04 marked provisional: Asus now macOS; Windows-host tests move to iMac
- 2026-10-02 22:27 R-T04 back on Asus via USB HDD + SATA fallback + QEMU usb-storage pre-check
- 2026-10-02 23:24 R-T04 -> SATA default + split input checks (RK-XHCI); R-T04D input checks + Legacy USB setting
- 2026-10-02 23:25 R-T04 -> same drive in 2.5in bay; AHCI-restore step; M4 keyboard-only shell pass noted
- 2026-10-02 23:27 R-T04D target pending (32GB stick vs SanDisk Extreme), X9 Pro off-limits, 0x80/removable check; R-T04 needs 2.5in SSD

- 2026-10-02 23:41 PT: M1-USB xHCI pre-check (runs/m1-usb-precheck). vettvdos-usb.img PASS on xHCI (removable on/off) and EHCI; two-entry image xHCI: ReactOS 0x7B (expected), DOS entry hangs at Loading FreeDOS (reproduced). R-T04D unblocked from the image side, still waiting on Vet to confirm the target stick.

- 2026-10-02 23:53 PT: M0-10 Release artifact: live boot branded PASS; installed first logon stuck at half-drawn taskbar at 100% CPU after 4+ min (runs/m0-10-ci). M0-10 not closed.

- 2026-10-03 00:05 PT: M0-10 re-run PASS (Release rel2 + Debug dbg2 installs reach full branded desktop); earlier stuck run was a clean install (post-kill) so logged as intermittent first-logon hang. v2 stick 66e1cca3 PASS on xHCI/EHCI; stray 0-byte N file on boot drive reported to DOSBox-X.
2026-10-03 00:26 v3 stick 70626d35 xHCI/EHCI PASS, no N file
2026-10-12 14:34 v3 two-entry 523c88c6 IDE/EHCI PASS
2026-10-18 14:27 matrix: added pre/post-#9573 save/install path columns (RK-USERS)
2026-10-18 14:28 matrix: split path columns into base d004b2c119 / hold a2c17cb765 / post-rename f0a83320d7 per Build
2026-10-18 23:23 I-4/I-6: wrapper pinned to x64 354d9c88 / i686 1bda1a60 (old 132953cc/d616f178 superseded)
2026-10-18 23:24 lane check after box loss: deliverables OK (stick zip img 70626d35, ps1 61061cf1, wrapper 354d9c88/1bda1a60). qemu-system-i386, qemu-img and mtools are MISSING; apt install blocked by another agent's apt lock, retry before next VM run. Recreated empty runs/r-t04d-asus, runs/win31-disc-check.
2026-10-18 23:25 reinstalled qemu-system-x86, qemu-utils, mtools (Debian 13 packages): QEMU emulator version 10.0.13 (Debian 1:10.0.13+ds-0+deb13u1)
2026-10-18 23:28 launch-hook test plan written for 17cc2c3b427
2026-10-19 00:27 apps-candidates.csv: 178 Audio/Video/Graphics/Internet/Office/Tools entries from reactos/rapps-db @68402a3 (trial excluded; 130 lack LicenseType, check license before testing). Untested. H10 WineVDM re-entry added to launch-hook PLAN.
2026-10-19 00:29 launch-hook test target pinned to c8d8280341f (rev 4)
2026-10-19 00:33 plans: launch-hook H16-H17 (rev5/shell), sync-a2c17cb765 S1-S5
2026-10-19 00:33 launch-hook PLAN retargeted to rev 5; H11 now expects one kind=dos stub line
2026-10-19 00:35 PLAN pinned to bd298ac19e9 (rev 5); H11/H16 expect one kind=dos line
2026-10-19 00:36 rev5 hook ISO (merge 313e66dc7c) sha verified 9d08b6e8...40a3; local copy in runs/launch-hook/iso/. H1 runs before HookEnabled=1 (H1 = off-by-default check). Run waits for bk-hook5-smoke result.
2026-10-19 01:02 H15 expanded for vetvdm-core rev2 4a98e7328ea (a-f)
2026-10-19 01:03 started qa-hook5-s1 (stage1 unattended setup, bootcdregtest 0fece3b1, 1GB, TCG), pidfile runs/launch-hook/vm/qa-hook5-s1.pid
2026-10-19 01:05 stage1 done; injected hookstub e615be4c as System32\vetvdm.exe, dos3.exe(ac3fb62e, exits 3), launch.exe(9b753de5) as myotvdm/otvdm/otvdmw/VetVdm launchers, guest/regtest.cmd as GuiRunOnce script. Started qa-hook5-s2.
2026-10-19 01:08 stage2 (GUI setup) done; qa-hook5-s3 (pid 398768) running GuiRunOnce QA script. A duplicate launcher I started by mistake was refused by QEMU's image lock and has been stopped.
2026-10-19 09:01 s3 idled with no disk writes after 01:07 (results absent); timed out. Started qa-hook5-s5 with QMP to see the screen.
2026-10-19 09:05 s5: upstream regtest ran instead (cwd C:\Windows\bin). Put QA script in All Users Startup + bin\regtest.cmd with run-once flag; started qa-hook5-s6.
2026-10-19 09:10 run1 done but hookstub.log garbled (cmd >> markers + stub append clobber on FAT); ren of vetvdm.exe failed so H8 invalid; ne.exe fixture got kind=dos. Run2 (qa2.cmd): per-test log copy, SKI.EXE (G011, sha 2d3da30c zip) as real Win16 target, del-based H8. Started qa-hook5-s7.
2026-10-19 09:13 run2 done: 14 PASS, 3 FAIL (H5, H13b, H16c; NE seen as DOS, section.c never returns WIN_16/NE_FORMAT), H10/H15/H17 not run. RESULTS-bd298ac19e9.md
2026-10-19 09:25 sync ISO hashes verified (bootcd 5cd99b63, regtest 0a0f0d8e). Started qa-sync-s1 (IDE) with sfp.exe helper for S2/S3.
2026-10-19 09:30 sync S2 PASS (IDE): LOCALAPPDATA set+exists, profile in Documents and Settings, no C:\Users. sfp.exe fixture fix (wprintf->printf) sha b48cd7cb...6f35, rerunning S3 as qa-sync-s5
2026-10-19 11:38 core-72ef: ISOs copied+sha verified; image ships real vetvdm.exe (reactos.inf), no DOSBox-X/otvdm found in output-core; started qa-core s1+s2 (vm/core.sh pid 567077)
2026-10-19 11:40 sync S1 EHCI boot started (qa-sync-ehci, usb-bot+scsi-cd on usb-ehci)
2026-10-19 11:42 S1 EHCI PASS: VetTV LiveCD language screen over usb-ehci usb-bot scsi-cd (vm/ehci.png); qa-sync-ehci killed
2026-10-19 11:42 core s1/s2 done; injected hookstub e615be4c, SKI 0b97b99f, type5 stub 5073f9b9 (synthetic), fake otvdmw d1dc49fe, launch.exe copies, run6/qa6.cmd as regtest.cmd+Startup; s3 started
2026-10-19 11:49 run6 done (res in run6/out). H5/H5t5/H13b/H16c pass; started s4 (qa7.cmd) to log which otvdmw H15e/g pick (fake fixed to %ls)
2026-10-19 11:49 wrote runs/sync-a2c17cb765/RESULTS.md
2026-10-19 11:56 s4 hung at desktop (empty taskbar, clock frozen, Startup qa7 never ran); killed my qa-core-s4 by PID, retry as s5
2026-10-19 12:00 s5 run7 done: H15e/f/g1 pass, g2 known gap confirmed. Wrote RESULTS-72ef305c03.md. No qa-core/qa-sync QEMU running.
2026-10-19 12:47 PLAN.md: added H15g2 rerun, H15h, H15i for vetvdm-core rev 3 98960d070e0
2026-10-19 12:47 drafted runs/asus-m2-install/STEPS-DRAFT.md (Bazzite writer, USB2, AHCI first, F1-F3 need Vet yes)
2026-10-19 12:50 payload fdf6e939ca: copied ISOs to iso/ with hash names, sha256 re-verified
2026-10-19 12:52 started qa-pay-s1/s2 (vm/pay.sh); wrote payload/qapay.cmd (H18 checks, classify, H10 ski+jazz via hook, then relaunch shell)
2026-10-19 12:58 s1/s2 done; injected SKI->C:\Games\SkiFree, Jazz ep1 (G008)->C:\Games\Jazz, type5->C:\qa, qapay.cmd as bin\regtest.cmd + All Users Startup; started qa-pay-s3
2026-10-19 13:03 s3: SkiFree window up via hook (pay-h10-ski.png)
2026-10-19 13:04 s3: Jazz JAZZ.EXE (Borland NE, DPMI16/RTM DOS extender, NE at 0xe3e) routed via hook into WineVDM -> 'Interrupt 0D #GP' fault dialog (pay-h10-jazz-otvdm-gp.png). Not DOSBox-X.
2026-10-19 13:14 s3 powered down (QMP). H18 checks OK. Shell listed SkiFree+Jazz on DOS & 16-bit (both 16-BIT). Shell Enter on SkiFree and Jazz -> "The DOS/16-bit player didn't start" (vetvdm exit 0xE0000005 in 30 ms): nested job (shell job + vetvdm core/launch.c AssignProcessToJobObject, no BREAKAWAY). vetvdm.log/shell.log overwritten at offset 0 (FILE_APPEND_DATA-only handle on ReactOS). --classify output not captured by redirect (CONOUT$).
2026-10-19 13:14 injected Lure 1.1 (G010, plain MZ Lure.exe) -> C:\Games\Lure, qapay2.cmd in Startup; started qa-pay-s4
2026-10-19 13:20 s4: Lure.exe via hook -> DOSBox-X 2026.10.01 title screen (pay-h10-lure-dosbox.png). Missed classify console screenshot (timing).
2026-10-19 13:22 rev3b ISO 3097cc548f hashes verified (aaacbcbd/6fe40b8f), copied to iso/; next H15a-i, H18, WineVDM recheck
2026-10-26 12:38 qemu/mtools missing again; reinstalled (QEMU emulator version 10.0.13 (Debian 1:10.0.13+ds-0+deb13u1)). Started qa-r3b s1-s3 (vm/r3b.sh)
2026-10-26 13:10 (from Build & Kernel, bk-default-smoke, default image 3097cc548f no payload) re QA finding "shell launch -> vetvdm 0xE0000005": a DIRECT cmd.exe call (no shell, no job object) `vetvdm.exe C:\Games\DosTest\DOSTEST.EXE` (dos3.exe ac3fb62e) also exits 0xE0000005 (VETVDM_E_START) in 30 ms: kind=dos layer=ntvdm warn=layer-fallback (DOSBox-X absent -> NTVDM fallback fails to start). So E_START is not only the nested job; on the payload image the job theory still needs a direct-vs-shell A/B. Logs-overwritten confirmed on default image: after relaunching from the shell, shell.log's first-frame line was gone and leftover tail bytes of older lines remained. For DOSBox-X Integration + Shell/UX. Evidence: lanes/build-kernel/runs/bk-default/vm/out/, RESULTS-item5.md
2026-10-26 13:30 rev3b 3097cc548f run (qa-r3b-s1..s6): H18, INIs (61B), H15a/b/d/e/f/g1/g2/i PASS; g2 fixed (wine_from=bundled); H15h2 routes to DOSBox-X; H15h1/h3, Jazz, shell A/B not finished (guest batch stalls after GUI start). Direct vetvdm dos3 -> exit 3 on payload image (no E_START). RESULTS-3097cc548f.md. All qa-r3b VMs stopped.
2026-10-26 14:08 r3b follow-up (one case/boot): H15h1/h2/h3 PASS (kind-override correct), Jazz still otvdm #GP (FAIL), shell A/B: direct hook OK vs shell 0xE0000005 in 15ms for Ski+Jazz -> shell-only/job. H15c blocked. qa-r3b-c VMs stopped.

## 2026-10-26 14:25 CDT: Asus USB installer (rev 3b) QA check
- Hash: `sha256sum -c SHA256SUMS` passes for both the .img and the .img.xz (files in lanes/build-kernel/build/usb-installer/).
- QEMU (own VMs qa-usb-*, script runs/asus-m2-install/vm/qa-usb.sh): 512G sparse target on ich9-ahci.
  - No confirmation (typed "erase"): the installer printed "That did not match. Nothing was written." The target had 0 allocated extents. PASS
  - EHCI and xHCI: prompt shown, `ERASE sda` accepted, VETTV-INSTALL-OK, read-back sha 6b71c9f3…3052 matches README. PASS for both
  - Target boot on AHCI (both): VetTV launcher shown, plus the New Hardware Wizard in the taskbar. PASS
  - Note: the guest clock shows UTC-like time (6:56 PM at ~1:56 PM CDT), even though unattend sets Central. Minor, for Build.
- STEPS-DRAFT.md rewritten (7-Zip + Win32 Disk Imager / GNOME Disks, no Rufus), marked PENDING the rebuilt image.
- 2026-10-26 14:40 disk cleanup: removed superseded ISOs/raw disks (kept aaacbcbd, 6fe40b8f, 58b6b6f4+5229ce9b, qa-r3b.raw)
- 2026-10-26 15:40 CDT qa3c 6d933b1240: ISO hashes OK (bed5321a/b85e5854), copied to iso/. QEMU -rtc base=localtime, VMs qa-3c-*. Clock PASS (Central), SkiFree from shell PASS (no E_START), Jazz still otvdm #GP (exit 0x100), H15c PASS, log append PASS (vetvdm.log 3 clean lines across 3 boots, shell.log kept), no layer process after quit combo PASS, job-nested only on the NTVDM fallback, missing-layer FAIL (NTVDM fallback 0xE0000005 + "didn't start", not 0xE0000004). Stall not proven (Lure title frame static from 50-60 s). One desktop-hang boot, retried. RESULTS-qa3c-6d933b1240.md. Kept vm/qa-3c.raw, no qa-3c VM running.

- 2026-10-26 18:03 CDT qa3d 9a64ae55ca: hashes OK. Missing-layer 0xE0000004 PASS; Jazz still WineVDM #GP FAIL (shell forces win16; direct vetvdm also otvdm); SkiFree/H15c/quit combo 0xE0000008 PASS; Lure no key reaction; KEYECHO via vetvdm and at Z:\> PASS (keys reach DOSBox-X); found ReactOS "no 3D acceleration" modal box; USB EHCI+xHCI PASS. RESULTS-qa3d-9a64ae55ca.md. No qa-3d VM running.
- 2026-10-26 20:45 CDT qa3e 422f008d1a: all hashes OK incl. on-disk vetvdm f1858d8e / vettvshell c7d3fcf0. PASS: missing layer 0xE0000004, SkiFree/H15c/quit combo 0xE0000008, KEYECHO vdm+prompt+tile (first key OK, no click), Jazz → DOSBox-X tile+direct, F10/taskbar no launch, no click-through, no OpenGL box, game to front, USB EHCI+xHCI→AHCI. FAIL: vetvdm.log caller=cli for shell launches (host_log.c ignores from_shell). Unit test: disk classifier OK, but "opengl blocked" + "template hostkey counted once" fail. New Hardware Wizard on AHCI first boot after USB install (not IDE-caused); launcher stays front but taskbar overlaps it (not in qa3d). RESULTS-qa3e-422f008d1a.md.
- 2026-10-26 21:00 CDT vetvdm-core-3f 318a304396 (qa3e tree + 3f conf.c/host_log.c, scratch build): vetvdm_test in qa-3e VM with golden files: 206 checks ok, exit=0. 3f vetvdm.exe from KeyEcho tile logs caller=shell. Exes/output in runs/launch-hook/qa3e/3f/. Temp disk deleted.
- 2026-10-26 23:40 CDT qa3f 51fd9488e5: all hashes OK (on-disk vetvdm 1946f4e7, vettvshell 52198030). ALL PASS: qa3e cases, caller=shell, vetvdm_test 206 ok with golden, USB EHCI+xHCI→AHCI no wizard + hint line visible + Central, --activate covers taskbar and after-game return OK. Clean for the stick. RESULTS-qa3f-51fd9488e5.md.
