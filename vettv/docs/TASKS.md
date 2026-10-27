# VetTV Retro OS — Task Board (M0–M1)

Owner: Vet · Maintained by: RetroOS Lead · Created Fri Oct 2, 2026, 2:10 PM PT
Source of truth for scope: `BRIEF.md` §7. A task is **Done** only with evidence (log path, screenshot, hash, or commit SHA) in the Evidence column.
Status: `todo` · `doing` · `blocked` · `review` · `done`. **[A]** = needs Vet's explicit approval for that exact action (see `lanes/lead/approvals.md`).

## Environment facts that change the plan (as of Oct 2, 2026)
- **Asus GL552VW runs Windows 10 again** (Vet, Oct 2, 3:07 PM PT). It can be a RosBE-Windows build host now; bare-metal ReactOS (M5) still needs a dedicated spare SSD so the Windows disk is never touched.
- **The iMac runs Windows 11 IoT Enterprise LTSC 24H2** (from the external Crucial X9 Pro SSD), not macOS. M0-6 VirtualBox testing is on Windows; RosBE-for-Windows is also an option there. Check for Hyper-V/VBS conflicts with VirtualBox before the first run.
- Box has no usable KVM (`box` user not in the kvm group), so QEMU runs in TCG. Enabling KVM is Vet's call; not requested.
- `gh` on the box is authenticated as `Vet-TV` (repo, workflow scopes). `Vet-TV/reactos` and `Vet-TV/dosbox-x` do **not** exist yet (checked 2:08 PM PT).

## M0 — Fork builds & boots in a VM

| ID | Task | Lane | Depends on | Status | Evidence |
|---|---|---|---|---|---|
| M0-1 | **[A]** Fork `reactos/reactos` → `Vet-TV/reactos` and create `vettv/main` from upstream `master` | Lead (drafts) → Vet (approves) → Build (runs) | — | done | https://github.com/Vet-TV/reactos, vettv/main @ d004b2c119 |
| M0-2 | Install RosBE-Unix 2.2.x (i386) on the box; record version + install path in `lanes/build-kernel/log.md` | Build & Kernel | — | done | log.md (RosBE 2.2.1, gcc 8.4.0) |
| M0-3 | Partial clone of upstream (`--filter=blob:none`) for local reading/building until M0-1 lands; record pinned upstream SHA | Build & Kernel | — | done | lanes/build-kernel/log.md (SHA 8a5b9300c1) |
| M0-4 | `configure.sh` + `ninja bootcd` on the box; archive build log, ISO size and SHA-256 | Build & Kernel | M0-2, M0-3 | done | build/SHA256SUMS.txt, log.md |
| M0-5 | QEMU TCG script: unattended install (bootcdregtest `unattend.inf` approach) → desktop; archive `serial.log` + QMP screendump | Build & Kernel | M0-4 | done | lanes/build-kernel/runs/m0-5-first/desktop.png + serial.log |
| M0-6 | Manual VirtualBox install of the same ISO on the iMac (Windows 11 host, legacy BIOS, AC97, 2 GB RAM); Vet runs it, QA writes the step-by-step and collects screenshots | Compat QA + Vet | M0-4 | todo | |
| M0-7 | Branding patch: boot screen / version string "VetTV Retro OS 0.0.1", on a local `vettv/feat/branding` branch; diff stat recorded | Build & Kernel | M0-4 | done | `vettv/feat/branding` @ 1504b59c1f, 1 commit ahead of vettv/main d004b2c1, 3 files +5/-5; hashes build/SHA256SUMS-m0-8.txt; Lead-verified 5:00 PM PT |
| M0-8 | Rebuild + reboot with M0-7 and screenshot proving the patch → build → boot loop | Build & Kernel | M0-5, M0-7 | done | lanes/build-kernel/runs/m0-8-branding/desktop-clean.png shows "VetTV Retro OS 0.0.1 (ReactOS 0.4.17-x86-dev)", build 20261002-0.4.17-dev-973-g1504b59; Lead-verified 6:58 PM PT |
| M0-9 | CI workflow draft (adapted upstream `build.yml`, gcc i386 Release + Debug, ISO artifact) kept **local** under `lanes/build-kernel/` | Build & Kernel | M0-3 | drafted | lanes/build-kernel/ |
| M0-10 | **[A]** Push `vettv/main` + CI workflow to the fork, enable Actions on the fork, run one build | Lead (drafts) → Vet → Build | M0-1, M0-9 | in progress | A3 done 7:09 PM PT; run https://github.com/Vet-TV/reactos/actions/runs/37088729334 (green run + QA boot of its artifact closes this) |
| M0-11 | Diff-size check script: `git diff upstream/master...vettv/main --stat` outside `modules/vettv/` (target < 500 lines) | Build & Kernel | M0-3 | done | `.github/vettv/diff-size.sh` on vettv/main; CI job downstream-diff green in run 37088729334: 108 lines (limit 500) ; Lead-verified 7:13 PM PT (run 37088729334 downstream-diff success) |
| M0-12 | M0 sign-off checklist: all five BRIEF §7 exit criteria with evidence links | Lead | M0-1…M0-10 | todo | |

**Parallel M0 planning work (no build dependency):**

| ID | Task | Lane | Status | Evidence |
|---|---|---|---|---|
| P-1 | Pin DOSBox-X 2026.10.01 (XP-compatible 32-bit asset + source tarball) and WineVDM v0.9.0 (`otvdm-for-old-windows`) with SHA-256 → draft `deps.lock` | DOSBox-X Integration | review | `lanes/dosbox-x/deps.lock` + `SHA256SUMS.txt` |
| P-2 | `vetvdm.exe` spec: CLI, registry policy keys, conf generation, exit codes → `lanes/dosbox-x/VETVDM_SPEC.md` | DOSBox-X Integration | review | `lanes/dosbox-x/VETVDM_SPEC.md` |
| P-3 | Start `LICENSES.md` (licence + source URL + notice location per component) | DOSBox-X Integration | review | `LICENSES.md` |
| P-4 | Test matrix for the 22 titles: source URL, redistribution terms checked, legal basis, "needs Vet's own copy" flag → `compat/matrix.csv` draft | Compat QA | review | `lanes/compat-qa/compat/matrix.csv` |
| P-5 | Smoke-test script skeleton (launch + 60 s + screenshot via QMP) | Compat QA | review | `lanes/compat-qa/smoke-test.py` |
| P-6 | `vettvshell.exe` screen spec + controller/keyboard navigation map + UI-stack choice (XP-era Win32 / SDL2) → `SHELL.md` draft | Shell/UX | review | `lanes/shell-ux/SHELL.md` |
| P-7 | Vet owns DOS, Win3.1 and 9x games and has Win3.1 + Win95/98SE install media (Oct 2, 3:09 PM PT). Media stays on Vet's machines only, never on the box, in git or in ISOs. Still need: exact game titles | Lead | doing | Vet replies 3:07 + 3:09 PM PT |

## M1 — DOSBox-X bundled, launches a DOS exe

| ID | Task | Lane | Depends on | Status | Evidence |
|---|---|---|---|---|---|
| M1-1 | Final `modules/vettv/deps.lock` (tag, asset name, URL, SHA-256) for DOSBox-X; decision recorded on whether `Vet-TV/dosbox-x` fork is needed (default: **no**, consume upstream artifacts + source tarball) | DOSBox-X Integration | P-1 | todo | |
| M1-2 | Setup add-on entry: `{L"DOSBox-X", ...}` in `dll/win32/syssetup/addons.c` + `boot/bootdata/packages/reactos.dff.in`, mirroring the WineVDM pattern; installs to `%SystemDrive%\VetTV\dosbox-x\` | Build & Kernel (patch) + DOSBox-X Integration (payload) | M1-1, M0-4 | todo | |
| M1-3 | "VetTV DOS" Start menu + desktop shortcut that opens DOSBox-X | DOSBox-X Integration | M1-2 | todo | |
| M1-4 | Local game disk with Doom shareware 1.9 (source URL + SHA-256 recorded; never in git/ISO) | Compat QA | P-4 | todo | |
| M1-5 | Doom shareware runs with SB16 sound from `cmd.exe` **and** Explorer via `dosbox-x.exe -c "mount c D:\Games\Doom" -c "c:" -c "doom"` in QEMU | Compat QA | M1-3, M1-4 | todo | |
| M1-6 | Same as M1-5 in VirtualBox on the iMac (Vet-run, QA-guided) | Compat QA + Vet | M1-5 | todo | |
| M1-7 | `THIRD_PARTY.md` license audit + draft source-offer page | DOSBox-X Integration | P-3, M1-1 | todo | |
| M1-8 | **[A]** Push `vettv/feat/dosbox-addon` and open a PR into `vettv/main` on the fork | Lead (drafts) → Vet → Build | M1-2…M1-5 | todo | |
| M1-9 | M1 sign-off checklist with evidence | Lead | all M1 | todo | |

## Open questions for Vet
1. Approve M0-1 (fork) now? Exact commands are in `lanes/lead/approvals.md`.
2. Which Win3.1 / Win9x media and Win16 / Win32 titles do you own (for fallback modes and matrix rows #13, #14, #19, #21)?
3. Any objection to running VirtualBox on the iMac's Windows 11 install for M0-6 / M1-6?
4. Do you own a StarCraft 1.16-era CD? (free 1.18 has spotty XP-era support)

## Lead decisions log
- 2026-10-02 3:16 PM PT: Swap BASS + Flight of the Amazon Queen (ScummVM data-only) for Jazz Jackrabbit ep.1 shareware + Rise of the Triad shareware; CD-ROM path tested with the freeware OMF 2097 CD ISO.
- 2026-10-02 3:16 PM PT: StarCraft row uses Vet's own 1.16 CD if he has one (asked); free 1.18 only as a fallback row.
- 2026-10-02 3:16 PM PT: M4 #1 "default shell" = the launcher is what you land in after logon (Run-key launch over explorer is acceptable; userinit.c patch only if the desktop flash is a problem).
- 2026-10-02 3:16 PM PT: Shelf names "DOS & 16-bit" and "32-bit PC" (not "16-bit PC", since DOS4GW games like Doom are 32-bit protected mode). No "Windows" in shelf names.
- 2026-10-02 4:30 PM PT: M0-4 built from upstream 8a5b9300c1 (local clone), so it is evidence for M0-4 only. M0-8 (branding rebuild) must be built from fork `vettv/main` (d004b2c1 or later) to close M0.
- 2026-10-02 4:30 PM PT: DOSBox-X default binaries include xBRZ (GPL-3.0). Accepted for M1 as a separately shipped program (aggregation), with a notice in THIRD_PARTY.md and its exact source tarball. Our own `--disable-xbrz` build is the target from M2. Never ship `vsbuild-*` or plain `mingw32` on Retro OS.
- 2026-10-02 4:30 PM PT: OMF 2097 fan CD ISO rejected ("All Rights Reserved"); QA builds its own ISO from the freeware v2.1 files.
- 2026-10-02 5:00 PM PT: M0-6 uses the M0-8 bootcd.iso (e4c311be…1323), not the M0-5 one. The A3 draft will include fast-forwarding vettv/main to vettv/feat/branding (no force) plus the CI workflow push.
- 2026-10-02 5:05 PM PT: vetvdm and Retro OS default is `output=surface` (QA m0-5-dbx: default DX9 warns, opengl broken on ReactOS soft GL 1.1). opengl/openglnb never default on Retro OS; Instant Mode on Win10/11 may revisit.
- 2026-10-02 6:59 PM PT: Vet approved A3. Vet confirmed he owns the StarCraft 1.16 CD; StarCraft stays in the matrix (Vet-owned, his machines only).
- 2026-10-02 8:25 PM PT: CI guard fix `ecb62fe5a8` (forbid-ms.sh, sha256 must match a ninja build output from the same run) accepted by Lead. The push and re-run wait for Vet's OK in Build & Kernel's 1:1. Follow-ups: add inpout32/inpoutx64 to the forbidden names (IMP-5); scanning inside reactos.cab is a separate task; widening the pattern later needs the deps.lock pinned-hash allowlist. Never put Win16 system file names in the pattern, because WineVDM ships its own stubs.
- 2026-10-02 8:25 PM PT: Vet asked for "Import your own Microsoft files". The spec is `IMPORT_OWN_FILES.md` (IMP-1…6), and it's on the ROADMAP. The no-Microsoft-files rule stays in force for everything we ship.
- 2026-10-02 8:26 PM PT: Vet confirmed the Asus GL552VW (now macOS Hackintosh) stays in the real-hardware plan, booting Retro OS from one of his spare USB hard drives so macOS is never touched. QA will add a fallback if USB boot fails (USB-to-SATA swap or internal spare SSD). The iMac stays primary for Instant Mode (I-4/I-6) runs.
- 2026-10-02 8:29 PM PT: New item M1-USB (Build & Kernel, local only): a prebuilt raw MBR disk image with Retro OS installed in QEMU, for Vet to write to a spare USB HDD for R-T04. Gate: QA boots the same image in QEMU as `usb-storage` first. If it hits 0x7B, use the SATA fallback. Vet does the flashing on his own machine. Nothing gets pushed.
- 2026-10-02 9:24 PM PT: Result from M1-USB. ReactOS has no xHCI driver, so it crashes with 0x7B when booting from USB 3.0. The GL552VW (Skylake) is xHCI-only, so the ReactOS entry for R-T04 is expected to need the internal SATA fallback. The DOS entry (R-T04D) stays on the USB HDD. New risk RK-XHCI: on xHCI-only real hardware (Skylake and later) ReactOS also has no USB keyboard, mouse or gamepad once the kernel takes over, unless the BIOS legacy USB emulation keeps working. That affects the M5 Asus run and the BRIEF's controller goals. Owner Build & Kernel (investigate xHCI options upstream); QA checks USB input during R-T04. Lead is asking Vet which drive bay macOS uses.
- 2026-10-02 9:43 PM PT: R-T04D is ready on the image side. `build/m1-usb/vettvdos-usb.img` (sha256 e5d2c692…a642) is written raw to the stick, and it passed the QEMU xHCI/EHCI pre-check (lanes/compat-qa/runs/m1-usb-precheck/RESULTS.md). The two-entry image is SATA-only, for R-T04. Lead approved Shell/UX's WIN.BAT wording table: never say 'Windows' when the machine has none. Waiting on Vet: is the 32 GB stick free, and will he get a 2.5" SATA SSD for R-T04?
- 2026-10-02 9:58 PM PT: M0-10 is still open. The live boot of CI Release ISO e59ac077a5 passed (QA), but the install run was invalidated: DOSBox-X Integration's cleanup killed QA's QEMU processes at about 9:47 PM PT, so stage 1 never finished. QA will re-run it cleanly. **New shared-box rule:** only kill processes you started yourself, tracked by PID file or a unique `-name` tag, and never use a blanket `pkill qemu-system-*`. The DOS payload now works from any drive letter (vhd eac03e85…07f2, stick 66e1cca3…e6b1). The stick image is being regenerated, and QA must pre-check the new hash before Vet flashes anything.
- 2026-10-02 10:07 PM PT: **M0-10 DONE.** CI run 37093271193 (e59ac077a5): the Release artifact was clean-installed and reached the full branded desktop (Lead viewed lanes/compat-qa/runs/m0-10-ci/rel2/shots/s3-b.png: 'VetTV Retro OS 0.0.1', build …-975-ge59ac07). Debug also passed. Watch item: one Release first logon hung while the box was heavily loaded (intermittent, not a blocker). The stick write is on hold until DOSBox-X posts the VHD hash with the REM '>' fix. Remaining for M0: M0-6 (Vet's VirtualBox run) and M0-12 (sign-off).
- 2026-10-02 10:23 PM PT: The DOS payload stray-N fix is in. vettvdos.vhd is b5aa4f35…92e64ff and the raw stick image is 70626d35…014a05bc, replacing eac03e85/66e1cca3. Next: Build regenerates the stick and two-entry images, QA pre-checks them, then QA DMs Vet the write steps (32 GB SanDisk Ultra only, no fixed time).
- 2026-10-02 10:29 PM PT: R-T04D stick image 70626d35…014a05bc passed QA's pre-check (xHCI/EHCI, no N file). QA is DMing Vet the write-and-boot steps for whenever he's free. The two-entry image is still being rebuilt for R-T04 (SATA-only, waiting on a 2.5" SSD).
- 2026-10-12 2:35 PM CT: The two-entry image 523c88c6…6674 passed QA's IDE/EHCI pre-check (SATA-only, R-T04). Quake 4 is scored in VMs, plus a software-render stretch on the Asus (VESA only on HD 530, 960M unusable). The Asus runs Win11 on the M.2 (Vet confirmed) and the 2.5" bay is empty. I-4/I-6 can go ahead on the Asus. QA is rewriting the R-T04D steps for a Windows host. R-T04 waits for a 2.5" SSD. Waiting on Vet's list of owned games.
- 2026-10-12 3:20 PM CT: Vet's owned-games list saved (lanes/lead/owned-games-2026-10-12.md). ReactOS playlist pulled (68 titles); overlap: Doom, Doom II, Death Rally, RTCW, Quake 4, Doom 3 BFG, Call of Duty, Disney's Hercules. Candidate matrix in lanes/lead/game-matrix-candidates.md (DOS 14 / Win16 0 / Win32-2D 11 / Win32-3D 14). Titles enter the matrix only after Vet confirms.
- 2026-10-12 3:28 PM CT: Vet owns Leisure Suit Larry 1,2,3,5,6,7 + Magna Cum Laude (Steam). LSL1-5 DOS candidates; LSL6 possible Win16 candidate (check whether the Steam copy includes the Windows 3.1 executable); LSL7 Win32 2D; Magna Cum Laude is modern, excluded.
- 2026-10-12 3:30 PM CT: Vet's Steam LSL6 is the DOS VGA (SCI) release only (SIERRA.EXE + DOS DRVs, bundled ScummVM); no Win16 exe. LSL6 moves to DOS list. Win16 coverage stays freeware/shareware.
- 2026-10-12 3:37 PM CT: Vet owns ~36 Win3.x-era games on disc (list in lanes/lead/owned-games-2026-10-12.md). Win16 gap closed pending QA's per-disc NE-header check. Suggested first Win16 set: SimCity Classic, SimAnt, Pipe Dream, Tetris Classic, Metal Marines, Indiana Jones Desktop Adventures, Oregon Trail, Star Wars Chess.
- 2026-10-18 2:40 PM CT: New risk RK-USERS. Upstream PR #9573 (merged Oct 17 2:53 PM CT) switches to NT6 profile layout (C:\Users, AppData\Roaming/Local, ProgramData, Public; %LOCALAPPDATA%/%PUBLIC%/%ProgramData% added; no Documents and Settings junction). Not in our base (8a5b9300c1, Oct 2). Our code: no hardcoded profile paths. Decision: hold sync past 95bba32e55 until Build & Kernel boot/env audit and QA save-path checks pass. Routing in lanes/lead/RK-USERS-PROFILE.md.

## Hardware driver queue (Vet, 2026-10-27 1:32 AM CT) — each starts after the previous merges
- 3h: xHCI USB 3 (Asus 8086:A12F), USB mouse/keyboard. Owner: Build & Kernel.
- 3i: HDA audio (8086:A170 + codec). Owner: Build & Kernel.
- 3j: Wired NIC RTL8168 (10EC:8168); user-supplied Realtek driver path, BSD re(4) port long-term. Owner: Build & Kernel.
- 3k: Intel chipset drivers (ME A13A, Serial IO A161, SMBus A123, etc.) + Bluetooth. Owner: Build & Kernel.
