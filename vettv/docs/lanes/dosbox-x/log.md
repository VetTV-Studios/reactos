# dosbox-x — log

## 2026-10-02 ~2:15 PM PT (corrected; box clock was off) — P-1/P-2/P-3 drafted
- P-1: downloaded + hashed DOSBox-X 2026.10.01 (winXP portable/setup, lowend9x, vsbuild-win32, source tarball) and WineVDM v0.9.0 (standard, old-windows, source). DOSBox-X hashes match GitHub's published asset digests. Draft: `lanes/dosbox-x/deps.lock`, sums in `lanes/dosbox-x/SHA256SUMS.txt`.
- Finding: upstream ReactOS wires WineVDM **0.8.1** (peterooch/winevdm_installer, via rapps-db), not 0.9.0. The 0.9.0 old-windows zip is an issue attachment from CI build master-2446, so its exact source commit is unconfirmed.
- Finding: DOSBox-X XP zip ships `inpout32.dll` (separate licence) — excluded from payload pending M1-7.
- 2026-10-02 ~3:12 PM PT (corrected): added §5.1 locked defaults (hostkey=mapper, no pad hotkeys, Ctrl+F9, Job Object kill-on-close) per Lead.

## 2026-10-02 ~4:00 PM PT — build-from-source research (BUILD_RESEARCH.md)
- Wrote `lanes/dosbox-x/BUILD_RESEARCH.md`. Covers upstream CI per Windows asset (VS2022 + v141_xp for winXP, v142 for vsbuild-win32, MSYS2 MinGW for mingw32, custom MinGW 7.3 for lowend), build matrix, per-host repro plans, XP/ReactOS checklist, licences and risks. Scratch work lives in `research/` (not in git).
- Measured imports against ReactOS 0x502 specs:
  - Clean: winXP SDL1/SDL2, mingw32_lowend SDL1/SDL2, lowend9x.
  - **Fail:** vsbuild-win32 (32 Vista+ kernel32 APIs despite PE 5.1) and mingw32 standard (SRW, threadpool, inet_ntop, SHGetKnownFolderPath). Don't use either on ReactOS.
- Recommendation for deps.lock: add `dosbox-x-mingw32_lowend-2026.10.01-portable.zip` (sha256 `7c22259b…13b7`, matches GH digest) as the A/B alternative to winXP. I have not edited deps.lock.
- Box (sudo -n OK): apt-installed 15 cross-toolchain packages: gcc/g++-mingw-w64-i686-posix 14.2, mingw-w64-i686-dev 12.0, binutils-mingw-w64-i686, autoconf, automake, libtool, autotools-dev, nasm, file. Log: `research/logs/apt-install.log`. RosBE GCC 8.4 is unusable as-is (win32 thread model, no std::thread).
- Cross-build **succeeded**: in-tree SDL2, D3D9/OpenGL, no FluidSynth/slirp. Clean scripted rerun took 310 s at -j6.
  - Output `research/artifacts/dosbox-x_xmingw_i686_SDL2_clean.exe`, sha256 `20e5ff3b79059a9640dcf0ef8a4153461d647a00108ad61313d2512cefadfbe2`.
  - PE 5.1, 790 imports, 0 missing at 0x502. **Not run yet**: needs a ReactOS VM smoke test.
- M1-7 licence flag: DOSBox-X compiles in xBRZ (GPL-3.0), so the default binary is effectively GPLv3. VS builds also embed the static MS CRT. Our own build can use `--disable-xbrz`.
- Note: the box clock printed CDT for part of today, so earlier "PT" times in this log (4:15 PM, 5:12 PM) may actually be CDT, i.e. 2:15 PM / 3:12 PM PT.

## 2026-10-02 4:44 PM PT (from `TZ=America/Los_Angeles date`) — companion paper tasks I-1, I-2, I-7, V-4, V-5, R-1, R-2 → review
- I-1: pinned `dosbox-x-mingw64-2026.10.01-portable.zip` (46,509,846 B, sha256 `def2d84d…8c59`, matches GH digest). Added `[dosbox-x.instant-mode-x64]` to `deps.lock` and a line in `SHA256SUMS.txt`. No MS CRT (only OS `msvcrt.dll`), no inpout32.dll, **but it ships `inpoutx64.dll`** (embeds a kernel driver), so that file is excluded. Evidence: `companion/I-1_mingw64-pin.md`.
- I-2: `companion/instant-mode/msdos-mode.conf` + `drivez/WIN.BAT`. 34 option names checked against the zip's `dosbox-x.reference.full.conf` (`check_conf_keys.out`). **Not validated at runtime** (no Wine/Windows on box). Finding: `hostkey` lives in `[dosbox]`, not `[sdl]` as VETVDM_SPEC §5.1 says; unknown keys are silently ignored. Spec not edited; raised in the outbox.
- I-7: `companion/I-7_vetvdm-core-split-plan.md` (plan only). The box has no x86_64 MinGW (i686 only).
- V-4 / V-5: QEMU recipes (`companion/V-4_…`, `companion/V-5_…`). V-4 was smoke-tested with box QEMU 10.0.13 up to the FD 1.4 LiveCD menu (`research/qemu-smoke/`). No Doom or Microsoft files were downloaded.
- R-1 / R-2: `companion/R-1_R-2_pins-and-licences.md` + `companion/companion-deps.lock`. Downloads are in `downloads/companion/` (+ SHA256SUMS.txt, ~304 MB, not in git). All hashes match upstream digests/checksums. Pins: grub4dos needs the dated build 2026-05-28; HIMEMX latest is 3.40 (FD 1.4 ships 3.39); UDVD2 has no author site.
  - Licence results: UDVD2, Loew AHCICD and JEMM AHCICD.DLL are OUT. SHSUCDX, VSBHDA, HX/HDPMI and CSMWrap need Lead sign-off.
- R-6: **not done** (left todo). Building it needs an MBR partition table written into the image. That conflicts with the "no partitioning commands anywhere" rule I was given, and the CD/SB drivers aren't bundle-OK yet. Waiting on a Lead/Vet decision.
- Installed: Python venv `research/tools/venv7z` (py7zr 1.1.3, pip, no sudo) to read the grub4dos .7z. No apt installs this session.
- TASKS.md: set Status=review + Evidence on I-1, I-2, I-7, V-4, V-5, R-1, R-2 only.

- 6:45 PM PT: fixed VETVDM_SPEC §5.1. `hostkey` belongs in `[dosbox]`, not `[sdl]` (verified in reference.conf 2026.10.01). The companion review flagged it.

## 2026-10-02 4:49–5:09 PM PT (from `TZ=America/Los_Angeles date`): Lead decisions of 4:48 PM PT applied; R-6 built → review
- **SHSUCDX:** upstream github.com/adoxa/shsucd. Pinned commit `f905b434…cc7d` (release 3.09; it includes the zlib LICENSE.txt). Built with NASM 2.16.03.
  - `SHSUCDX.COM` sha256 `1c274fda…f4a7`.
  - It differs from the release binary in 3 bytes, all XCHG operand-order encodings, so the instructions are the same.
  - Source tarball `SHSUCD-src-f905b43.tar.gz` `c096ff23…9225`. Nothing was missing for the build.
- **VSBHDA:** tag v2.0 = `c005dff5…e01e`. Source tarball `VSBHDA-src-v2.0.tar.gz` `eaca1ab2…fd38`.
  - Surprise: the tag has **no LICENSE**. GPL-2 LICENSE was added in the next commit, 75fa4bb. We ship that text.
- **XCDROM32** replaces UDVD2. Pinned `Jemm-src-v5.86.tar.gz` `c7f6cd68…703a` as its source. GNU_GPL.TXT = standard GPL-2 text `8177f975…0643`.
- **HX:** HXRT.TXT shipped. No change to the pin.
- **CSMWrap:** made a recursive clone at 3.1.2 (7 submodules) and read every licence. Archive `CSMWrap-3.1.2-recursive-src.tar.gz` `2d9d0feb…ea92` + MANIFEST. `test_only = true` in the lock.
  - Surprise: CSMWrap's LICENSE is LGPL-2.1 with no "or later", but it embeds SeaBIOS (LGPL-3.0) blobs in the same EFI. That's only clean if CSMWrap is 2.1-or-later.
  - Every other submodule is permissive or compatible: Apache-2.0+LLVM-exception, BSD-2, 0BSD, Unlicense/0BSD, BSD/MIT, MIT.
- **R-6:** `companion/R-6/build_vettvdos.sh` produced `vettvdos.vhd` (fixed VHD, MBR + FAT32), sha256 `db8b8ad4…e98b`.
  - The script refuses /dev, PhysicalDrive, non-/workspace paths, symlinks and root (`evidence/guard-tests.txt`). It never mounts or attaches.
  - QEMU 10.0.13 TCG boots to `C:\>` with D: = IDE CD (XCDROM32 + SHSUCDX, test ISO read) and VSBHDA on AC97 and on HDA. Menu 2 (HIMEMX + JEMM386) works, and `WIN` reboots.
  - **Finding:** QEMU `sb16` at default A220 makes VSBHDA refuse ("SB found"). The passing runs move sb16 to A240. Evidence in `companion/R-6/evidence/`.
- **I-2:** `msdos-mode.conf` keeps D: read-write by default and adds a documented per-game `-ro` option. `ver=7.1` kept. Key checker re-run: OK.
- **I-7:** toolchain note updated (x86_64 posix GCC 14 installed). A trivial std::thread test compiles to PE32+ (`research/x64-thread-test/`). Not run.
- Updated: `companion/companion-deps.lock`, `R-1_R-2_pins-and-licences.md` ("Lead decisions" section), `downloads/companion/SHA256SUMS.txt` (+5 lines, all verify), new `companion/R-6/SHA256SUMS.txt`, and `companion/TASKS.md` (R-6 Status/Evidence only).
- Box installs (`sudo apt-get`): mtools 4.0.48, fdisk 2.41.5 (sfdisk), dosfstools 4.2, xorriso 1.5.6. No remote writes.

## 2026-10-02 5:14 PM PDT (from `TZ=America/Los_Angeles date`): V-4/V-5 presets updated for the R-6 sb16 finding
- **V-4:** sb16 moved to `iobase=0x240,irq=7,dma=3,dma16=7` (same as the passing R-6 runs). Added `-device intel-hda -device hda-duplex,audiodev=snd0` for VSBHDA.
  - BLASTER is now VSBHDA's emulated card: `A220 I5 D1 H5 T6 P330`. §5 rewritten to explain why, with the VSBHDA load order.
  - Board changed `isapc`/486 → `pc`/pentium3/64 MB, because isapc has no PCI bus for intel-hda (QEMU rejects it) and VSBHDA needs a Pentium. isapc is kept as a no-VSBHDA fallback.
  - Dropped `adlib`: VSBHDA has its own OPL3 at 388, and adlib's default ports sit under the emulated A220. A check boot with adlib still loaded VSBHDA (`research/v4-check/ev/`).
- **Pre-existing bug fixed in V-4 §3b:** `fat:ro:` on an IDE disk fails ("Block node is read-only", with or without `readonly=on`). It now uses `snapshot=on`: the vvfat node stays rw=false and writes go to a temp overlay.
- **V-5:** sb16 kept at A220. Added a note: Win98SE uses its own SB16 driver, not VSBHDA, and has no in-box HDA driver.
- QEMU 10.0.13 accepts all three command lines, extracted verbatim from the docs and run with `-S` (`research/v4-check/cmdline-check.txt`). TASKS.md not touched (evidence paths unchanged).
- 5:19 PM PT: VETVDM_SPEC §5 now defaults to `output=surface` on ReactOS, following QA's M0-5 result (OpenGL crashes). Instant Mode `msdos-mode.conf` keeps `opengl`, because it runs on real Windows hosts.
- 7:04 PM PT: §5.1 Job Object containment verified on M0-8 by QA; parent-watch fallback dropped.

- 2026-10-02 22:30 PT: Delivered vettvdos.vhd (sha256 db8b8ad4…e98b) + R-6 doc + vettvdos.SHA256SUMS to lanes/build-kernel/inbox/m1-usb/ for M1-USB (Lead-approved). Verified: fixed VHD (conectix footer), MBR, part1 FAT32-LBA 0x0C @LBA 2048 active, FreeDOS boot32lb (hidden=2048, drv=0x80), GAMES\ empty, no keep-out/MS files. Caveat sent: if partition moves, patch BPB hidden sectors.
- 2026-10-02 23:23 PT: Posted R-T04D/R-T04 input notes to room; added RK-XHCI note to VETVDM_SPEC.md.

- 2026-10-02 9:56 PM PT: R-6 drive-letter independent. CONFIG.SYS drive-less paths; AUTOEXEC.BAT sets %VD% from new BOOTDRV.COM (INT 21h AX=3305h, errorlevel chain); SHSUCDX takes the first free letter; WIN.BAT has no letter. Menu 3 is now neutral `Restart`; the WIN wording comes from SHELL.md, picked at runtime (\VETTV\HOSTWIN.FLG means Windows host; C: means stick; else Retro OS). Rebuilt out/vettvdos.vhd sha256 eac03e85…07f2, raw 66e1cca3…e6b1. QEMU: C: (CD D:), HOSTWIN, D: via FreeLoader on a copy of Build's image (CD E:) all pass incl. WIN/menu 3; without the BL boot-sector patch D: fails (`Bad or missing Command Interpreter`), so Build keeps only that patch + hidden sectors. Delivered vhd + doc + NOTE-anyletter.md + vettvdos.SHA256SUMS to lanes/build-kernel/inbox/m1-usb/. Incident: at ~9:47 PM PT a broad `kill` of qemu-system-i386 PIDs on the shared box also terminated 3 compat-qa QEMU runs (lanes/compat-qa/runs/m0-10-ci: stage1 install, plus two live ISO boots). compat-qa should re-run M0-10.

- 2026-10-02 10:22 PM PT: R-6 stray-file fix (Compat QA m1-usb-precheck). AUTOEXEC.BAT line 6 REM had `>= n`, and FreeCOM redirects even on REM lines, so every boot created an empty `N` in the root. Reworded the comment (no <, > or | left in any comment). build_vettvdos.sh now refuses <, > or | in REM/::/; comment lines (refusal test in evidence/guard-tests.txt). boottest_qemu.py now boots a scratch copy without snapshot and compares the FAT roots (mdir) after each run; QEMU is tagged -name dosx-r6-<run> with a pidfile, and only its own process is stopped. Rebuilt vettvdos.vhd b5aa4f35…64ff, raw 70626d35…05bc. Matrix passed with all roots unchanged: C:, C:+HOSTWIN, D: (fresh copy of Build's 10:11 PM image 1a7dc402…), WIN and menu 3. No-BL-patch D: fails as expected. Old-REM control reproduces `N`. Inbox m1-usb updated (vhd, SHA256SUMS, NOTE-anyletter.md).
2026-10-15 7:58 PM CT: exe-header-check.ps1 (61061cf1…8413, unchanged) tested under PowerShell 7.4.6 on 11 fixtures (synthetic NE/PE/LE/LX/MZ-only/ZM/bad e_lfanew/tiny/empty/ELF + real DOSBox-X setup PE). All classified correctly; non-.exe skipped; refuses OutCsv inside Root; fixture tree unchanged. Not run on Windows PowerShell 5.1 (no Windows on box).
2026-10-18 2:30 PM CT: RK-USERS: LOCALAPPDATA is not set on base d004b2c119 (source check); added the path rule to I-7. Runtime check on the image is still to do.
2026-10-18 11:15 PM CT: reviewed vettv/vetvdm-paths a75d61e771, approved (see companion/REVIEW-vetvdm-paths-a75d61e771.md).
2026-10-18 11:20 PM CT: VETVDM_SPEC §2 aligned with LAUNCH-HOOK.md (--image, raw tail after --); asked Build to add --from-hook.
2026-10-18 11:25 PM CT: spec accepts NT-style --image (strip \??\), install path System32 recorded.
2026-10-18 11:24 PM CT: post-loss check: RosBE i686 gcc 8.4.0 present; exe-header-check.ps1 hash unchanged (61061cf1…). Shell/UX direct calls without --from-hook already allowed by spec §2 (flag optional).
2026-10-18 11:26 PM CT: spec §2 --image: hook will pass Win32 path (Build review); \??\ stripping kept as defensive fallback.
2026-10-18 11:30 PM CT: built tools/vetvdm-hookstub (test-only vetvdm.exe stub for launch-hook QEMU checks), sha256 e615be4c…98f8.
2026-10-18 11:52 PM CT: wrote agent-briefs/VETVDM-V0.1.md for Lead; spec §5 NTVDM line now matches hook's image-name guard (VETTV_VDM_BYPASS dropped).
2026-10-19 12:35 AM CT: reviewed vettv/vetvdm-core 44ee247535 — CHANGES NEEDED (B1 errorlevel, B2 win32cui). Review file companion/REVIEW-vetvdm-core-44ee247535.md.
2026-10-19 12:40 AM CT: spec §6 adds 0xE0000009 row + log-file decision for Shell/UX.
- Mon Oct 19 ~1:05 AM CT: reviewed vetvdm-core rev 2 4a98e7328ea, APPROVED (companion/REVIEW-vetvdm-core-4a98e7328ea.md); RosBE 8.4.0 -Werror clean, CUI, XP imports; added [jsmn] MIT to deps.lock.
- Mon Oct 19 ~9:03 AM CT: VETVDM-V0.1.md line 5 now points at VetTV-Studios/reactos (remote studios), per Lead's repo move.
- Mon Oct 19 ~9:27 AM CT: QA H5 fail (NE routed as dos). Posted NE-check rules for Build's hook fix; queued rev 3 item 3 (header NE beats --kind=dos) in REVIEW-vetvdm-core-4a98e7328ea.md.
- Mon Oct 19 ~9:30 AM CT: told Lead hook rev 6 should reuse GetBinaryTypeW (SCS_WOW_BINARY only), since NE exetyp 5 = DOS extender; amended rev 3 item 3 to match.
- Mon Oct 19 ~9:30 AM CT: queued 86a0fdf776 (WINVER 0x502 spelling) as rev 3 item 4. Hook rev 6 now via kernel32 classifier (agent); 95a58ae088 not pushed.
- Mon Oct 19 ~12:05 PM CT: staged stripped DOSBox-X payload at lanes/dosbox-x/payload/VetTV/ (23M, 130 files, SHA256SUMS.txt) for H15 a-d; queued rev 3 item 5 (log chosen WineVDM path).
- ~12:08 PM CT: added WineVDM 0.9.0-old-windows to payload/VetTV/winevdm for QA ISO only (source commit unconfirmed = release blocker).
- 12:40 PM CT: wrote LICENSE-AUDIT.md (DOSBox-X, WineVDM, jsmn).
- 12:51 PM CT: wrote a single LF into 17 zero-byte payload files (ReactOS setup 0xC000011E); SHA256SUMS regenerated.
- 12:52 PM CT: changed course: dropped the 14 empty DOSBox-X shader placeholders (13 glshaders/*.glsl + shaders/none.fx) instead of padding, since a 1-byte file could be read as a broken user shader; kept LF padding on the 3 WineVDM INI/CFG files. 267 files.
- 12:55 PM CT: per Lead, restored the 14 shader placeholders as one LF each; SYSTEM.INI/WIN.INI now a ';' comment line; MIDIMAP.CFG stays one LF. 281 files.
- 2026-10-26 12:29 rev 3c aeaaf3ef02: job breakaway/nested fallback + log append fix; -Werror i686 build clean; pushed studios vettv/vetvdm-core-3c
- 2026-10-26 14:40 disk cleanup (Vet-approved via Grok Bot): deleted superseded test images:
  - fc0120238e76837b companion/R-6/test/anyletter-D/anyletter-d-test.img
  - 6948f9a46d939845 research/qemu-smoke/freedos-doom.vhd
  - c48a9dcf4b8e22f4 research/qemu-smoke/FD14LIVE.iso
- 2026-10-26 15:41 rev 3d 58d7b86a13 on vettv/vetvdm-core-3d: NTVDM-fallback failure -> E_NOLAYER + os error in log; NE exetyp 1/3/5 -> DOS (Jazz exetyp=1). -Werror build clean (vetvdm + vetvdm_test); tests not executed (no Win32 runner on box). Pushed under Vet's OK.
- 2026-10-26 16:59 rev 3e part 1 44521c772a on vettv/vetvdm-core-3e: --caller=shell, opengl blocked, locked-hostkey only on real override. -Werror clean; tests compile-only. Spec updated.
- 2026-10-26 18:05 3e 24139dfa27: fix header read length (lfanew+4 -> +0x40) so ne_exetyp rule actually applies (Jazz, QA qa3d).
- 2026-10-26 18:06 3e c83e043c39: on-disk classify test (Jazz layout). Compile-only here.
- 2026-10-26 18:08 3e part 2 3fba176eba: bring-to-front on launch (pairs with shell 6cb51432d9). Compile-only.
- 2026-10-26 20:43 3f 318a304396 on vettv/vetvdm-core-3f: caller=shell logged, opengl from template blocked, sdl hostkey counted. Compile-only.

- 2026-10-26 9:00 PM CT: 3f CMakeLists synced with qa3e (add_cd_file, psdk deps), pushed.
- 2026-10-27: freed extracted DOSBox-X prebuilt/src trees (list in cleanup-2026-10-27.txt); archives kept.
