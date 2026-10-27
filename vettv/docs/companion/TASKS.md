# Companion projects — M0 task split

Maintained by RetroOS Lead · Created Fri Oct 2, 2026, 4:25 PM PT · Priorities and sequencing: `../ROADMAP.md`
Status: `todo` · `doing` · `blocked` · `review` · `done`. **[A]** = needs Vet's approval. **[V]** = Vet runs it on his own machine, guided by the owning lane. Done = evidence in the Evidence column.

## SAFETY GATE — Restart in DOS Mode (Real Reboot)
Until **both** Lead and Vet sign off in `lanes/lead/approvals.md` (item R-GATE):
- No `bcdedit`, `bcdboot`, `SetFirmwareEnvironmentVariable`, `diskpart`, partition, format or `Suspend-BitLocker` command runs on **any real machine** (Asus, iMac, HP, or the box host itself). VM guests only.
- The app's write path stays behind a compile-time `VETTV_DRYRUN_ONLY` flag in every build that leaves the box.
- No USB writer run against a physical disk. Target image files only.
- The HP 255 G10 is never a target.
Sign-off needs: M0 + M1 VM evidence, an uninstall diff that matches exactly, written WinRE recovery steps, and Vet's BitLocker/recovery-key answer.

## §I — DOS Mode (Instant), M0 "look & feel spike"
| ID | Task | Lane | Depends on | Status | Evidence |
|---|---|---|---|---|---|
| I-1 | Pin `dosbox-x-mingw64-2026.10.01-portable.zip` (URL, SHA-256) in `deps.lock`, and confirm it has no MS CRT and no `inpout32.dll` | DOSBox-X Integration | — | review | `lanes/dosbox-x/companion/I-1_mingw64-pin.md` + `lanes/dosbox-x/deps.lock` [dosbox-x.instant-mode-x64] |
| I-2 | Hand-written `msdos-mode.conf`: no banner/menu bar, `fullresolution=desktop`, locked defaults, original VetTV banner text, Z: `WIN.BAT` = EXIT | DOSBox-X Integration | I-1 | review | `lanes/dosbox-x/companion/I-2_msdos-mode-conf.md` + `instant-mode/` (unvalidated at runtime) |
| I-3 | Banner/boot text review: original wording, no Microsoft strings, fonts or logos | Lead | I-2 | todo | |
| I-4 | Box smoke test: conf launches under Wine or a Win10 VM, `WIN` exits, `--dry-run` shows the locked defaults | Compat QA | I-2 | todo | |
| I-5 | Fade-in/out wrapper prototype (borderless topmost window + Job Object), Win32 C++ | Shell/UX | I-2 | review | lanes/shell-ux/companion/i5-fade-wrapper/ (cross-built i686+x64, not yet run) |
| I-6 | **[V]** Run on the Asus (Win10) and iMac (Win11): time from click to `C:\>` (< 3 s target), resolution restored after `WIN`, iGPU vs 960M noted, screen recording | Compat QA + Vet | I-2, I-5 | todo | |
| I-7 | `vetvdm-core` split plan: which functions move out of vetvdm, plus the x64 MinGW build target (plan only, no code move until Retro OS M1-2 lands) | DOSBox-X Integration + Build & Kernel | — | review | `lanes/dosbox-x/companion/I-7_vetvdm-core-split-plan.md` |
| I-8 | M0 sign-off | Lead | I-1…I-6 | todo | |

## §V — Portable VM Player, M0 "portable QEMU spike"
| ID | Task | Lane | Depends on | Status | Evidence |
|---|---|---|---|---|---|
| V-1 | Pin QEMU 11.1.1 source tarball (URL, SHA-256, signature check) | Build & Kernel | — | todo | |
| V-2 | Cross-build QEMU for Windows x64 with MinGW-w64 on the box into a portable folder (`qemu\` + firmware + DLLs + licences), with the build script archived | Build & Kernel | V-1, Retro OS M0-5 | todo | |
| V-3 | DLL and firmware licence inventory for the portable folder (start `THIRD_PARTY.md`) | Build & Kernel → Lead review | V-2 | todo | |
| V-4 | FreeDOS 1.4 + Doom shareware guest recipe (`isapc`/`pc`, 486, `sb16` + `adlib`, TCG) | DOSBox-X Integration | — | review | `lanes/dosbox-x/companion/V-4_freedos-doom-qemu-recipe.md` |
| V-5 | Win98SE preset command line (`pc`/`pentium2`, `cirrus-vga`, `sb16`), docs only. Vet installs from his own media | DOSBox-X Integration | — | review | `lanes/dosbox-x/companion/V-5_win98se-qemu-preset.md` |
| V-6 | **[V]** Asus run from a USB stick, no install: FreeDOS + Doom under TCG; Win98SE under TCG vs WHPX A/B; WHPX quirks logged. Check `HypervisorPlatform` state read-only first, never toggle without Vet's OK | Compat QA + Vet | V-2, V-4, V-5 | todo | |
| V-7 | Open Q1 check: does WHPX work from a standard-user process? | Compat QA | V-6 | todo | |
| V-8 | Open Q4: read QtEmu's licence text (GPL-2.0-only vs -or-later) and record it | Lead | — | todo | |
| V-9 | Qt 6 Widgets + MinGW UI skeleton plan (no code until V-2 works) | Shell/UX | — | done | lanes/shell-ux/companion/V-9_VM_PLAYER_UI_PLAN.md, Lead-reviewed 4:50 PM PT |
| V-10 | M0 sign-off | Lead | V-1…V-7 | todo | |

## §R — Restart in DOS Mode (Real Reboot), M0 "manual proof in VMs" (safety gate applies)
| ID | Task | Lane | Depends on | Status | Evidence |
|---|---|---|---|---|---|
| R-1 | Pin FreeDOS 1.4, grub4dos 0.4.6a BIOS build, CSMWrap 3.1.2, VSBHDA 2.0, JEMM, HIMEMX, CuteMouse, SHSUCDX, UDVD2 (URL + SHA-256) | DOSBox-X Integration | — | review | `lanes/dosbox-x/companion/R-1_R-2_pins-and-licences.md` + `companion-deps.lock` |
| R-2 | Licence audit for the open ones (SHSUCDX, UDVD2, AHCICD, HX/HDPMI): anything unclear stays out of the bundled image | DOSBox-X Integration → Lead sign-off | R-1 | review | `lanes/dosbox-x/companion/R-1_R-2_pins-and-licences.md` §R-2 |
| R-3 | Windows guest VM harness on the box: Win10 legacy-BIOS VM + Win11 OVMF VM (Secure Boot off), from media Vet approves (see ROADMAP), with clean snapshots | Build & Kernel | Vet's media answer, Retro OS M0-5 | todo | Unblocked 6:55 PM PT: Vet chose Microsoft evaluation ISOs. M0-5 done. |
| R-4 | **VM only:** BIOS path, bootmgr → grub4dos → mapped fixed `vettvdos.vhd` → FreeDOS; one-shot `bootsequence`; next reboot lands in Windows | Build & Kernel | R-3, R-6 | todo | |
| R-5 | **VM only:** UEFI path, CSMWrap on a second MBR disk → FreeDOS; one-shot via `{fwbootmgr}`; Secure-Boot-on refusal check | Build & Kernel | R-3, R-6 | todo | |
| R-6 | Build `vettvdos.vhd` (fixed VHD, FAT32, FreeDOS + menu CONFIG.SYS + `WIN.BAT` = FDAPM WARMBOOT) | DOSBox-X Integration | R-1 | review | `lanes/dosbox-x/companion/R-6/R-6_vettvdos-vhd.md` + `R-6/evidence/` (QEMU TCG boot to `C:\>`, IDE CD on D:, VSBHDA on AC97/HDA; sha256 `db8b8ad4…e98b`) |
| R-7 | Backup/restore drill in both VMs: `bcdedit /export`, `/enum all /v` before; restore; `/enum all` diff archived and empty | Build & Kernel | R-4, R-5 | todo | |
| R-8 | WinRE recovery steps doc (how to undo by hand if Windows won't boot) | Build & Kernel → Lead review | R-7 | todo | |
| R-9 | UI and DOS-menu wireframes (summary screen that lists every change before any write) | Shell/UX | — | done | lanes/shell-ux/companion/R-9_REBOOT_WIREFRAMES.md, Lead-reviewed 4:50 PM PT |
| R-10 | Test matrix draft (brief §6) with HP 255 G10 marked out of scope | Compat QA | — | review | `lanes/compat-qa/companion/R-10_reboot_matrix.csv` |
| R-11 | M0 sign-off (VM evidence only; this does **not** open the safety gate) | Lead | R-4…R-8 | todo | |

## Lead decisions
- 2026-10-02 4:30 PM PT: The iMac is **out of scope for Real Reboot** (it's Vet's only daily machine and boots Windows from an external USB SSD). It stays in scope for Instant Mode and the VM Player. Revisit only after Asus M3 passes.
- 2026-10-02 4:50 PM PT (R-9/V-9 review): RQ-7 dry-run "Simulate restart" never reboots. RQ-10 "Back to Windows" is OK as descriptive text, never in a product name. VQ-1 Build & Kernel owns all of `vmcore`; Shell/UX owns the Qt layer only. VQ-5 pin one Qt 6 LTS in `deps.lock` with its exact source. VQ-8 our code GPL-2.0-or-later, Qt LGPL-3.0 dynamic; borrow no QtEmu code until V-8 confirms its licence. VT323 + Inter are OFL: bundle them with notices in THIRD_PARTY.md.
- 2026-10-02 4:55 PM PT (DOSBox-X R-2 licence sign-offs): **SHSUCDX OK**, built from the upstream adoxa/shsucd repo at a pinned commit that includes its zlib `LICENSE.txt`, and shipped with that text. McCoy's pre-2006 portion is noted for the M3 audit. **VSBHDA OK** under GPL-2.0: ship the repo LICENSE and offer the source from the pinned tag. **HX/HDPMI OK**, because its licence explicitly allows redistribution: ship HXRT.TXT. **CSMWrap is OK for local and VM testing only.** Shipping it needs the remaining submodule licences read and a full recursive source archive. For the CD-ROM driver, use **XCDROM32** (GPL-2, JEMM zip, add GNU_GPL.TXT) instead of UDVD2. UDVD2, AHCICD.SYS and AHCICD.DLL stay out.
- 2026-10-02 4:55 PM PT: **`inpoutx64.dll` stays out** of every build, the same as inpout32.
- 2026-10-02 4:55 PM PT (R-6): Writing a partition table **inside a `.vhd`/`.img` file under `/workspace` on the box** is not covered by the safety gate's partition ban. The tool must refuse any block-device or `\\.\PhysicalDrive` target, never attach or loop-mount the image as a device on a real machine, and stay dry-run in builds that leave the box.
- 2026-10-02 4:55 PM PT (I-2 defaults, Vet may override): `D:\Games` is mounted **read-write**, because DOS games save into their own folders, with a per-game `readonly` option in the profile. Keep `ver=7.1`. Added to Vet's open questions.
- 2026-10-02 6:55 PM PT (R-3, Vet via Grok Bot): Win10/Win11 test guests use Microsoft's free evaluation ISOs, downloaded only from microsoft.com and hash-recorded. They stay local on the box under `toolchain/` or a VM folder. They never go into git, ISOs, CI artifacts or any release, and the forbid check must cover them. Treat the evaluation licence as test-only, never activate it, and rebuild from snapshot when it expires.
