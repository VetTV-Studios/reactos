# build-kernel — log

## 2026-10-02 (PT)
- M0-3 done: partial clone (`--filter=blob:none`, master only) at `src/reactos`, pinned upstream SHA `8a5b9300c1cd7a9627727246d7d55ee0093a063f` (2026-10-02 01:03 PT, "[CRT:MATH] Import trunc/truncf from musl (#9597)").
- M0-2 in progress: RosBE-Unix 2.2.1 source tarball, sha256 `d720c0a77dc08c6da3929fd282de3fe7af6d697b6fb104889a0362e770742565`, building into `toolchain/RosBE` (log: `lanes/build-kernel/rosbe-build.log`). Host is Debian 13 (Python 3.13, host GCC 14).
  - Fix 1: added `python-is-python3`, `pkg-config` (builder prereq check).
  - Fix 2: bundled ninja 1.10 `configure.py` imports `pipes` (removed in Py 3.13) -> patched to `import shlex as pipes` in `sources/ninja.tar.bz2` (original kept as `ninja.tar.bz2.orig`).
  - Risk: GCC 8.4 may not build cleanly with host GCC 14; fallback = upstream CI `build_rosbe_ci.sh`.
- Installed qemu-system-x86, qemu-utils, socat (apt). `/dev/kvm` exists but box user isn't in its group, so TCG only; not changing that without owner OK.
- Drafts (local only): `qemu-boot.sh` (M0-5), `ci/vettv-build.yml` (M0-9), `diff-size.sh` (M0-11).
- Answers for teammates: Job Objects implemented in ntoskrnl/ps/job.c (only NtCreateJobSet unimplemented); explorer only shows taskbar when Winlogon\Shell contains "explorer.exe" (StrStrIW substring) and no shell window exists; i386 kernel32 sends Win16 via BaseCheckVDM to basesrv/NTVDM, no NtVdm64 lookup.
- 15:10 PT M0-2 done: RosBE-Unix 2.2.1 built at `toolchain/RosBE` (i686-w64-mingw32-gcc 8.4.0 RosBE-Unix, cmake 3.17.2-ReactOS). GCC 8.4 built fine on host GCC 14.
- 15:11 PT M0-4 started: `configure.sh` + `ninja bootcd bootcdregtest` in `build/` (log `lanes/build-kernel/m0-4-build.log`, 11,655 steps).
- A1 (fork) on hold pending Vet's direct confirmation to Build & Kernel; Lead relayed approval at 15:07 PT.
- 15:11 PT A1 run after Vet confirmed directly ("go A1"): fork https://github.com/Vet-TV/reactos created (22:11:27Z = 15:11 PT), parent reactos/reactos, default branch master (unchanged). `vettv/main` created at d004b2c119ef54a8cd292801d35654cec49195ca (upstream "[TCPIP] Avoid double acquire of spin lock (#9602)", 14:07 PT). Both verify commands pass. Nothing pushed, Actions untouched. Local clone is still at 8a5b9300c1 (older); will fetch fork before M0-7.
- 16:16 PT M0-4 done: `ninja bootcd bootcdregtest` exit 0 (Debug, gcc 8.4.0, upstream 8a5b9300c1). Artifacts in `build/` + `build/SHA256SUMS.txt`:
  - bootcd.iso 369,098,752 B sha256 51dab49e5cc1d4e608eb388d92a00ba6808808a66a4974f6ec777d57fcdce67b
  - bootcdregtest.iso 126,877,696 B sha256 92c90c09a55ce2e57484c184c185944146186894e440355891461ea2e2d146ff
- 16:20 PT M0-5 first run started (`runs/m0-5-first/`). Fixed qemu flag (`-accel tcg,thread=multi`). Stage 1 reached CHECKPOINT:HALF_COPIED within ~40 s. Only miss so far: kdvbox.dll not on CD (harmless). TODO after run: `-audiodev none` to silence ALSA noise.
- 16:36 PT M0-5 first run PASS: bootcdregtest.iso (sha256 92c90c09…) unattended install on QEMU TCG reached the desktop with taskbar in ~16 min total (stage 1 ~1 min, stage 2 screendump at +15 min). Evidence: `runs/m0-5-first/{serial-stage1.log,serial.log,desktop.png}`. Desktop shows regtest.cmd console ("AHK Application testing suite not present, skipping" + a missing-file error from the regtest harness) and a New Hardware Wizard; both are expected regtest/driver noise, not install failures.
- Follow-ups: pass marker currently matches "winlogon" (too weak); switch to screenshot check + explorer start marker. Add `-audiodev none`. Stage 2 should detach the CD so regtest.cmd doesn't run, or use plain bootcd + our own unattend for the M0-8 branding check.

## 2026-10-02 M0-7/M0-8 branding rebuild
- Branch `vettv/feat/branding` @ 1504b59c1f (3 files, +5/-5 vs fork/vettv/main). Reconfigured + rebuilt, NINJA_EXIT=0; build string 20261002-0.4.17-dev-973-g1504b59.
- bootcd.iso e4c311be3849762fb3564309bf4c1c20eff0a4e8c45d17e00eb304db4c7d1323; bootcdregtest.iso f895b6a91559a5287b91ae33e53ea19472bd6eab56e18e65b0bcfc482f4734d7 (build/SHA256SUMS-m0-8.txt). "VetTV Retro OS" string present in both ISOs.
- qemu-boot.sh: added -audiodev none for AC97. Boot run started into runs/m0-8-branding/ (~16 min TCG); screenshot check of desktop string pending.
- Installed gcc-mingw-w64-x86-64-posix + g++ (GCC 14-posix) for DOSBox-X I-7 x64 core (needed apt-get update first).
- M0-9 workflow ready locally: RosBE pinned f94ba214bd, gist pinned e583d55a + sha256 ea42b032…263c check, permissions contents:read, YAML validated. No push.
- M0-8 run PASS (marker found): kdinit serial shows Build 20261002-0.4.17-dev-973-g1504b59, Commit 1504b59c1f. desktop.png: version text mostly covered by regtest cmd window; visible tail 'ev)' + 'Reporting NT 5.2' matches new format. Clean recheck (Win+D, runs/m0-8-branding/recheck.sh) started ~17:12 PT.
- A3 prep: local commit 517dfe7b99 on vettv/feat/branding (workflow + .github/vettv/diff-size.sh, which the diff job needed and was missing). diff vs fork/vettv/main: 5 files +103/-5; 112 lines outside modules/vettv vs upstream. FF from d004b2c119 OK. Not pushed.
- M0-8 clean recheck (Win+D): desktop-clean.png shows 'VetTV Retro OS 0.0.1 (ReactOS 0.4.17-x86-dev)' / Build 20261002-0.4.17-dev-973-g1504b59.GNU_8.4.0. M0-8 evidence complete.
- A3 (Vet 'Go A3' in DM 7:01 PM PT): step1 Actions enabled OK; step2 build.yml+labeler.yml disabled_manually, stale.yml already disabled_fork (403 not active). Step3 push approval card interrupted; vettv/main still d004b2c119, no runs. Stopped; awaiting Vet re-approval in DM.
- A3 step3 pushed 7:08 PM PT (Vet 'Push A3' in DM): vettv/main d004b2c119..517dfe7b99. Run https://github.com/Vet-TV/reactos/actions/runs/37088729334 (VetTV Build, push) in progress; only our workflow triggered.
- M0-11: CI downstream-diff job green in run 37088729334 (108 lines vs live upstream master, limit 500). Row set to review.
- CI fix (run 37088729334 false positive on ReactOS-built d3dx9_24..43): .github/vettv/forbid-ms.sh hash-allows only ninja-built DLLs; local positive (20 ALLOW) + negative (fake msvcr71 FORBID, rc=1) tests pass. Local commit ecb62fe5a8, not pushed.
- Amended CI fix to e59ac077a5: hard-forbid inpout32/inpoutx64.dll + UserFiles paths (IMP-5 part). Tests: inpout rc=1, UserFiles rc=1, fake msvcr71 rc=1, bootcd rc=0. Not pushed. IMP-5 remaining: deps.lock hash allowlist when M1 payload lands; artifact (not just ISO) scan.
- 2026-10-02 22:27 PT: Vet said 'Push CI fix' in DM. Pushed e59ac077a5 to fork vettv/main (FF from 517dfe7b99). Run 37093271193.

## 2026-10-02 M1-USB raw USB image (local only, no push)
- ~8:30–9:40 PM PT. `build/m1-usb/vettv-retroos-usb-i386.img` (8 GiB raw, MBR) sha256 d680573e…3ba9b; `.img.xz` 46,153,372 B sha256 dd13d678…57304 (SHA256SUMS, xz round trip OK). Image unchanged by verification (qcow2 overlays).
- Layout: p1 active FAT32 7.5 GiB ReactOS (C:\ReactOS + C:\VetTV\dos\vettvdos.vhd); p2 0x0C FAT32 510.9 MiB = byte copy of vettvdos.vhd partition (hidden sectors patched in primary + backup BS).
- Unattended install via local ISO remaster using the build's own mkisofs (`/i386/unattend.inf`, Setup_Debug default, isobtrt). Zero source changes, no commits. Stage 1 62 s, stage 2 60 s (TCG). Stage 1 needs a blank disk: isoboot hangs on SeaBIOS when HDD 0 has a valid MBR.
- freeldr.ini: default `ReactOS`, 10 s, + `VetTV DOS (R-T04 DOS path)` (BootType=BootSector, partition(2); BootType=Partition is deprecated and pops a warning box).
- Verify: IDE ReactOS/Debug/DOS PASS; EHCI usb-storage ReactOS/Debug/DOS PASS (also DOS removable=on). xHCI: ReactOS STOP 0x7B (0xC0000034, IopMarkBootPartition), because ReactOS has no xHCI driver, so expect it on the xHCI-only GL552VW. xHCI DOS via FreeLoader hangs at "Loading FreeDOS" (standalone VHD on SeaBIOS xHCI boots fine).
- FreeDOS gives C: to the active partition (ReactOS), so p2 runs as D: (boot-sector BL patch + C:\->D:\ in CONFIG/AUTOEXEC/WIN, CD letter E). FAT32 BS takes the drive from DL (`mov [bp+0x40],dl`), confirmed by booting with BPB byte 0x40=0x00.
- Scripts: `lanes/build-kernel/m1-usb/`. README: `build/m1-usb/README.md`.
- 23:42 PT: CI run 37093271193 on e59ac077a5 GREEN (Debug, Release, downstream-diff). Artifacts vettv-retroos-i386-{Debug,Release}-e59ac077a5... (254 MB / 235 MB). M0-10 pending QA boot of Release artifact.
- 23:58 PT: Regenerated vettvdos-usb.img from any-letter VHD eac03e85...07f2 -> sha256 66e1cca3...e6b1 (matches DOSBox-X). Two-entry image p2 regen in progress.

## 2026-10-02 M1-USB rev 2: any-letter DOS payload
- ~10:00–10:20 PM PT. New vettvdos.vhd eac03e85…07f2 (NOTE-anyletter.md). `update-dos-payload.sh` (on a working copy): p2 @15724544 overwritten with the exact partition bytes + hidden sectors; only the boot-sector BL patch (0x10F B3 03 90, sectors 0+6), no text edits (removed from `dos-as-drive-d.sh`). p2 vs VHD partition: 10 bytes differ, all expected. C:\VetTV\dos\vettvdos.vhd replaced (read back eac03e85…). fsck.fat -n: p2 clean; p1 no errors (pre-existing BS/backup byte 0x41 00/01 note, harmless).
- QEMU (2 VMs max, `-name bk-m1usb-*`): IDE + EHCI ReactOS desktop PASS; IDE + EHCI DOS `D:\>` + "Type WIN to restart into VetTV Retro OS.", no "Windows", PASS. Evidence `build/m1-usb/verify-v2/`.
- New .img 1a7dc402dbf42ef4db4fcac7aa08182ab435548a9e09320bde39fceb6c7b7387, .img.xz 5a1583f731df40bc710035df9c393feaef6cd2bc8bb30eb56c83442a3ca18032 (46,473,800 B; xz round trip OK). SHA256SUMS updated; vettvdos-usb.img line kept as is.

## Fri Oct 2, 2026 10:24 PM PT
- Regenerated build/m1-usb/vettvdos-usb.img from REM-fix VHD b5aa4f35…92e64ff; raw sha256 70626d35…014a05bc matches DOSBox-X lane. SHA256SUMS updated. Two-entry image rebuild queued.

## 2026-10-02 ~10:23–10:40 PM PT — M1-USB rev3: N-file fix DOS payload
- Why: the rev2 payload eac03e85's AUTOEXEC.BAT has `REM ... >= n`, and FreeCOM redirects it, creating an empty file `N` in the p2 root on every boot. DOSBox-X Integration delivered the fixed `vettvdos.vhd` b5aa4f3510d2cac2bd20aeb54b8ceb643011a1ba15e8dc768c23176cc92e64ff (536,871,424 B).
- Scripts:
  - VHD_SHA default updated in `add-dos-partition.sh` and `post-install.sh`.
  - The `dos-as-drive-d.sh` comment now describes the payload generally (drive-letter independent).
  - `verify.py` gained a root check (mdir of the p1 and p2 roots before boot vs after boot on a converted overlay; fails on any change or a file `N`; `--no-rootcheck` turns it off).
- `update-dos-payload.sh build/m1-usb/vettv-retroos-usb-i386.img build/m1-usb/work/v3.img`:
  - Boot-sector patch only (hidden sectors 15724544; 0x10F 8A 5E 40→B3 03 90; sectors 0 and 6). The compare vs the VHD partition shows exactly 10 bytes differ.
  - p1 `C:\VetTV\dos\vettvdos.vhd` reads back as b5aa4f35…. The mtype check of AUTOEXEC.BAT finds no `>= n` and no redirect characters.
  - fsck.fat -n: p2 clean (71 files). p1 has no errors (only the pre-existing offset-0x41 volume-state note).
- QEMU on overlays (verify-v3/), all PASS:
  - IDE and EHCI ReactOS reach the branded desktop.
  - IDE and EHCI DOS reach `D:\>` with the restart line and no "Windows". ROOTCHECK PASS on both, with p1 and p2 roots unchanged and no `N`.
  - Negative control: the old 1a7dc402 image's IDE DOS boot gives ROOTCHECK FAIL (`N` appears in the p2 root).
- Promoted:
  - img 523c88c66b06faae9631939bfef7301d1f2728ce2dbe2c901bb6103ea27e6674 (8,589,934,592 B, sparse ~269 MB).
  - xz 07bbd64515a9caed9875c8fe13a3b5bf8787b2ba14cf5bd737fece6c4a9afc25 (46,794,336 B).
  - The xz round trip matches. SHA256SUMS has the img and xz lines updated; the vettvdos-usb.img line was kept untouched. `sha256sum -c` passes for all 3.
  - README updated; rev2 (1a7dc402 / 5a1583f7) is marked superseded because of the N bug.
- Only my own bk-m1usb-* VMs were started and stopped; Compat QA's VMs were not touched.

- 2026-10-18 23:25 CT: the box had lost host g++, m4 and the build/ output (M1-USB images included). Reinstalled g++ (4:14.2.0-1) and m4 (1.4.19-8) via apt. The vetvdm wiring build runs in wt/vetvdm-wire/output-wire (branch vettv/vetvdm-wire, local only). The M1-USB images must be rebuilt before anyone relies on build/m1-usb hashes again.
- 2026-10-19 09:03 CT: installed socat (QMP screendump in qemu-boot.sh; was missing, so bk-hook5-smoke had no screenshot).

## 2026-10-26 12:35 CT
- Upstream issue, non-blocking: text setup logs a copy error for `kdvbox.dll` (listed in txtsetup/reactos.dff but not built/placed on the CD for i386 GCC builds). Seen since M0-5 and again in bk-default-smoke stage 1; setup continues and the install boots. Not ours to fix; track as upstream, no VetTV change.
- Box had lost qemu/mtools/socat again; reinstalled qemu-system-x86 10.0.13, qemu-utils, mtools, socat, xorriso, squashfs-tools, syslinux, zstd, busybox-static.
- Restarted QA-CHECKS item 5 (bk-default-smoke) and USB installer disk build (bk-usb-disk-*) from rev 3b output-qa bootcd aaacbcbd…4ab3e. usb-installer/unattend.inf TimeZoneIndex 4 -> 20 (Central).
- 2026-10-26 13:40 CT USB installer done. build-retroos-disk.sh now rebuilds the ISO with the tree's mkisofs + .lst (xorriso remaster gave "Could not load SYSTEM.ALT hive"); file-level diff vs aaacbcbd: only i386/unattend.inf differs. retroos-disk.img 6b71c9f3…3052, stick img c3ec6f6b…d42f, xz 51804eb5…a6e1. test-qemu EHCI + xHCI: install PASS, target boots to launcher on AHCI. Item 5 results in runs/bk-default/RESULTS-item5.md.

## 2026-10-26 14:55 CT — qa-payload-3c (local only)
- wt/qa-payload-3c on vettv/qa-payload-3c from 3097cc548f: merge 66928988a6 -> 8ff4d7f49e, merge studios aeaaf3ef02 -> 6d933b1240 (HEAD). Both clean. 5c0a2c76bc was merged (b15481793a) then dropped per Lead; kept as local tag bk/qa3c-with-5c-b15481793a.
- output-qa3c (payload lanes/build-kernel/payload): bootcd bed5321a…0cb9, regtest b85e5854…b66b. .lst: 282 VetTV entries each. Earlier identical-source build pre5c/ 95d3444c…/7e0cba7f… (ISOs aren't byte-reproducible).
- bk-qa3c-smoke: shell reaches the desktop (runs/bk-qa3c/shots/smoke-desktop.png).
- Lure from shell -> DOSBox-X 2026.10.01 starts, no 0xE0000005. shell.log appends (23 lines, 7 starts, 2 launch lines). No vetvdm.log is written while the game runs.
- "Freeze": the guest stays alive (QMP running, IRQ0 ~67/s, HLT idle, no BSOD/KDBG). The GUI stops repainting ~30-40 s after launch, at the Lure title screen; same with default audio and -audiodev none.
- Clock: regtest unattend TimeZoneIndex=85 (GMT), RealTimeIsUniversal absent; -rtc base=localtime shows correct CT.

## 2026-10-26 16:25 CT — rev 3s: qa-payload-3c + shell 0c12af82e0, USB installer rebuilt (local only)
- wt/qa-payload-3c-shell, local branch vettv/qa-payload-3c-shell from 6d933b1240; merge 0c12af82e0 (launch.c, vtshell.h; clean, no WINVER/_WIN32_WINNT) -> 471e2be8ea. No push.
- output-qa3s (-Werror, VETTV_PAYLOAD_DIR=lanes/build-kernel/payload, payload hashes verified, flock /tmp/vettv-ninja.lock): bootcd 31e10016c530098d7fd77bb54902df499d32963dcbf21f961d65c5ec3f4d9aac, regtest b269daf5c8c0aed9a4807e2bae15bd8816cab6d56a02f09f53c01e9499751dd3 (runs/bk-qa3s/).
- Smoke = build-retroos-disk stage 3 (-rtc base=localtime): VetTV launcher, clock 15:57 = CT (build/usb-installer/disk/desktop-ide.png).
- USB: old rev 3b kept as vettv-retroos-installer-r3b.img(.xz), disk-r3b/, test-*-r3b, SHA256SUMS-r3b(.txt). New: installer img 2a9f2a70…6898, xz e01bb489…40c1, retroos-disk.img 3b62a2a0…f5d2, gz fdce9025…4760 (SHA256SUMS.txt + SHA256SUMS, sha256sum -c OK). retroos-disk.img(.gz) at top level are hardlinks to disk/.
- test-qemu EHCI: ERASE sda typed, VETTV-INSTALL-OK readback 3b62a2a0…; target boots on AHCI to the launcher, clock 16:23 = CT (test-ehci/target-desktop-ahci.png). xHCI not rerun.
- -rtc base=localtime added to qemu-boot.sh, m1-usb/install.sh, runs/bk-default/vm/s3.sh+s12.sh, runs/m0-8-branding/recheck.sh, runs/bk-qa3c/run.sh (default localtime), usb-installer/test-qemu.sh, build-retroos-disk.sh (bk-stall already had it).
- README: rev 3s + Asus BIOS clock = local Central time note (no Rufus). Marked B&K note appended to lanes/compat-qa/runs/asus-m2-install/STEPS-DRAFT.md.

## 2026-10-26 16:58 CT — qa3d (pushed to studios vettv/qa-payload-3d)
- 3db4bcb009 + merge shell 0c12af82e0 -> 820ff4c498 + merge rev 3d 58d7b86a13 -> 9a64ae55ca (HEAD). output-qa3d bootcd 70d83298…44a4, regtest de19c70a…f9b8 (-Werror clean, no vettv warnings).
- bk-qa3d-smoke (localtime): launcher at 16:22 CT, runs/bk-qa3d/boot/desktop.png.
- Stick 7e890426…0fb4, xz e8632289…b165, disk 9109e1c1…f282. EHCI install PASS, target boots to launcher with clock 16:53 CT. rev 3s files renamed -r3s. xHCI not run.
- 2026-10-27 01:30 CT qa3g (vettv/hw-3g 10337654ef, local only): usb-installer/build-retroos-disk.sh now calls new cleanup-qemu-enum.sh (hivex; libhivex-bin installed) after stage 3, before gzip/hash. usb-installer/ is NOT in any git repo; pre-change copy at runs/build-retroos-disk.sh.pre-3g. 27 keys removed (runs/bk-qa3g/usb/qemu-enum-removed.log). Offline: no QEMU Enum keys, SuppressNewHWUI=1. Smoke bk-qa3g-smoke on a copy: launcher + desktop, no wizard (runs/bk-qa3g/smoke/).
