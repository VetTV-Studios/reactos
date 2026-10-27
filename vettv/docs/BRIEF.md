# VetTV Retro OS — Project Brief

**Owner:** Vet (VetTV / VetTV Studios), solo creator, AI-assisted ("vibe-coded") development
**Status:** Pre-M0 planning document · drafted Fri Oct 2, 2026 (PT)
**One-liner:** A ReactOS-based, open-source, *neither-Windows-nor-Linux* retro gaming OS that runs DOS, Win16 and Win9x/XP-era Win32 games from one console-style launcher, with DOSBox-X built in as the 16-bit DOS / Win3.x layer.

> Sibling projects (do not confuse): **VetTV Retro Desktop** is a browser "web OS" nostalgia SPA; **VetTV Vintage PC** is a paid native BYO-ISO emulator. VetTV Retro OS is a real bootable OS (ISO / bare metal).

---

## 1. Vision

Boot a PC (or VM) straight into a VetTV-branded game launcher. Double-click (or pad-select) any old game, whether a DOS `.EXE`, a Win3.x game, or a Win9x/XP DirectX game, and it just runs. The right compatibility layer is chosen for you:

- **DOS** → DOSBox-X (cycle-accurate-enough PC emulation: VGA/EGA/VESA, Sound Blaster/GUS/AdLib/MT-32, joystick).
- **Win16** → WineVDM (otvdm) natively on the desktop, with a DOSBox-X + user-owned Windows 3.1 guest as fallback.
- **Win32 (Win9x/XP era)** → ReactOS's NT-compatible userland, with Wine-derived DirectX (WineD3D → OpenGL, optionally Vulkan).

It is **not Windows** (no Microsoft code, branding or trademarks) and **not Linux** (ReactOS has its own NT-style kernel). It's built on top of upstream open-source work (ReactOS, DOSBox-X, WineVDM, Wine) with the smallest downstream patch set we can manage.

### 1.1 Honest compatibility targets

"100% compatible" isn't achievable. ReactOS is still officially **alpha** [R1][R3], and old games depend on hardware and API quirks. Targets are measured against the M3 test suite (§7) using the ratings in §8:

| Category | Layer | Target (by M3/M5) | Why this number |
|---|---|---|---|
| DOS real-mode / DOS4GW games | DOSBox-X | **≥ 95%** of suite Gold+ in VM; ≥ 90% on Asus | DOSBox-X is mature and runs as an ordinary 32-bit app; ReactOS only needs to host it. DOSBox-X already says its XP build runs on ReactOS, "experimental" [R12][R13] |
| Win3.x / Win16 games | WineVDM (primary) / DOSBox-X+Win3.1 (fallback) | **≥ 85%** Gold+ (combined) | WineVDM ships in ReactOS 0.4.16 images [R1] but has 340+ open issues [R16]; the DOSBox-X + real Win3.1 fallback covers most of the rest [R11] |
| Win32 2D (GDI / DirectDraw) | ReactOS + Wine ddraw | **≥ 75%** Gold+ | Basic DirectX support through WineD3D works; the Wine-10 DirectX import (Sep 24, 2026) is meant to unlock "100s" of games but is brand new [R9][R10] |
| Win32 3D (D3D 5–9, OpenGL) | WineD3D→OpenGL (HW driver or Mesa software) | **≥ 50%** Gold+ in VM; best-effort on Asus | GPU drivers are the weakest link on real hardware (§10). Software Mesa is slow |
| DX10/11, Vista+ only, anti-cheat / SecuROM / StarForce DRM | none | **Out of scope** | ReactOS targets NT5.x by default; kernel-mode DRM is a non-goal |

---

## 2. Current state of the building blocks (researched Oct 2, 2026)

### 2.1 ReactOS
- **Latest release: 0.4.16 (Aug 29, 2026).** Adds a new graphical installer, a unified boot/live CD, a new ATA storage driver (replaces UniATA, fewer `0x7B INACCESSIBLE_BOOT_DEVICE`), a new HD-audio bus driver (sklhdaudbus + KMDF), Nvidia driver slowdown fixes (system PTE layout), a Server Core install type, and a partial Wine→10.0 sync. **For the first time, release images include WineVDM** for 16-bit Windows apps [R1][R2]. GitHub's latest release tag is `0.4.16-release` [R5].
- **Previous release:** 0.4.15 (Mar 21, 2025): PnP manager rewrite, USB boot, booting on EHCI/OHCI/UHCI chipsets [R4][R6].
- **x64:** amd64 builds boot to desktop but have **no WOW64**, so 32-bit apps/games won't run on x64 ReactOS. Kernel prep for WOW64 was merged (PR #8713, Aug 2026), but WOW64 isn't done [R4][R7]. → **We target i386 (32-bit) ReactOS.**
- **UEFI:** FreeLoader UEFI boot for i386/amd64 has been merged (PR #5267) and fixes are ongoing (e.g., skipping the x86 BIOS emulator on UEFI boots) [R8]. It's still young. → **We use legacy BIOS/CSM boot on the Asus.**
- **Build system:** CMake + Ninja via the ReactOS Build Environment (**RosBE 2.2.x**: GCC 8.4.0, Binutils 2.34, Mingw-w64 6.0.0, CMake 3.17.x-ReactOS, Ninja 1.10). `configure.sh`/`configure.cmd` then `ninja bootcd` (since Apr 28, 2026 `livecd` only produces a non-bootable `liveimg.iso`; use `bootcd`). MSVC/clang builds are also supported [R14][R15]. Upstream GitHub Actions builds gcc/clang × i386/amd64 on `ubuntu-22.04` with a CI RosBE, plus MSVC on Windows runners (`.github/workflows/build.yml`) [R5].
- **NTVDM:** ReactOS has its own NTVDM (`subsystems/mvdm/`: `ntvdm`, `dos`, `wow16`, `vdmdbg`, `pifmgr`). It's **incomplete**: no VESA/EGA, no Sound Blaster, no FPU, with per-game instability; 0.4.15 only made minor DOS file-search fixes [R17][R18][R6]. `kernel32!CreateProcessInternalW` (`dll/win32/kernel32/client/proc.c`) routes `STATUS_INVALID_IMAGE_NOT_MZ / _PROTECT / _WIN_16` images through `BaseCheckVDM` (in `vdm.c`). **That's our handoff hook** (§3.2).
- **Game-compat lists:** the official wiki page **"Games ROS Testing"** (lists e.g. Far Cry 1, GTA I, SimCity 3000, Zuma Deluxe; only a few DOS entries such as Pac-Man and Prince of Persia) [R19], the **"Version Status"** page (DirectX status) [R20], and a community **ReactOS Compatibility List** [R21]. *Note: reactos.org/wiki is behind an Anubis anti-bot wall, so bots should cite search snippets or have the owner open the pages in a browser.*
- **Wine-shared DLLs:** ReactOS uses a fork of Wine that talks to an NT-style kernel instead of Unix. It was historically held at Wine 2.x/3.x for Server 2003 compatibility and is now partway to Wine 10.0. Release images still export Server 2003 APIs only; `-DDLL_EXPORT_VERSION` turns on Vista+ exports [R1]. ReactOS ships Wine `xinput1_1/1_2/1_3/9_1_0`, `dsound`, `winmm`, and Mesa software OpenGL (`dll/opengl/mesa`) [R5].
- **DirectX:** WineD3D-based D3D8/9 and DirectDraw over OpenGL [R20]. 2023: Wine-staging-4.18 import with D3D10/11 (PR #5688, merged) [R22]. **Sep 24, 2026: PR #9352 imported DirectX from Wine-10.1-dev** (≈498k lines added, 119k removed). It allows OpenGL **and Vulkan** translation, but "Vulkan only possible with a custom branch or the Mesa software renderer." This landed on `master` **after** the 0.4.16 branch point [R9][R10]. `dll/directx/wine/` now contains d3d8–d3d12, ddraw, dinput/8, dsound, dplayx, quartz, wined3d, etc.
- **DXVK:** zlib-licensed and needs a Vulkan 1.3 driver. It's not a supported ReactOS component, and ReactOS lacks working real-hardware Vulkan [R23][R10]. → **DXVK is an optional, late, VM-only experiment.** WineD3D/OpenGL is the default.

### 2.2 DOSBox-X
- **Latest release: 2026.10.01** (published Oct 1, 2026, 7:29 PM PT). Adds S3 ViRGE 3D, AWE32, GUS Max/CS4231/WSS, Pro Audio Spectrum, and a unified FPU path for Win95 task switching [R24][R25]. Previous: 2026.08.02 (assets include `winXP` setup/portable, `mingw32_lowend9x`, `vsbuild-win32/win64`, `hx-dos`) [R26].
- **Guests:** "DOS-based Windows such as Windows 3.x and Windows 9x are officially supported." Win 1.0–3.11 run straight from the integrated DOS. Win95/98/ME must be **booted** from a disk image with their own DOS [R11][R27].
- **On ReactOS:** INSTALL.md says the XP-compatible builds "work in ReactOS as well, but support is considered experimental." Lowend 9x builds are 32-bit SDL1 only. XP builds need `XInput9_1_0.dll` (ReactOS ships a Wine version) [R12][R13][R5].
- **Branches:** upstream now keeps `master` (full emulation) and `main-osfree` (built-in DOS emulation disabled, for jurisdictions with OS age-verification laws). Use `master` [R26].
- **License:** repo `COPYING` = GPLv2; source headers read "either version 2 of the License, or (at your option) any later version" (inherited from DOSBox) [R28].
- **Prior downstream packaging:** Batocera ships DOSBox-X as a package (`package/batocera/emulators/dosbox-x/dosbox-x.mk`) [R29].

### 2.3 WineVDM / otvdm
- "16-bit Windows (Windows 1.x, 2.x, 3.0, 3.1, etc.) on 64-bit Windows". GPL-2.0, latest stable **v0.9.0 (Sep 15, 2023)** with an `otvdm-for-old-windows` build "for ReactOS, and Windows versions before Vista". Still actively pushed (Sep 2026), 343 open issues [R16][R30].
- ReactOS 0.4.16 installs it as a **setup add-on**: `modules/optional/winevdm_setup.exe` listed in `boot/bootdata/packages/reactos.dff.in` and silently installed by `dll/win32/syssetup/addons.c` (i386 only) [R1][R5]. **We reuse exactly this add-on pattern for DOSBox-X.**

### 2.4 Licensing compatibility (not legal advice)
| Component | License (verified) | Notes |
|---|---|---|
| ReactOS | README: GNU GPL 2.0; per-file SPDX overwhelmingly `GPL-2.0-or-later` (GitHub code search: ~1,464 hits vs 0 for `GPL-2.0-only`); Wine-derived parts LGPL-2.1+ [R5] | GitHub's license detector reports "GPL-2.0" for all three repos; it doesn't tell "only" from "or later" |
| DOSBox-X | GPLv2 COPYING, headers "v2 or (at your option) any later" [R28] | Bundles SDL (zlib), FluidSynth/munt (LGPL), etc. Run a third-party audit at M1 |
| WineVDM | GPL-2.0 (Wine-derived code LGPL-2.1+) [R16] | Already redistributed by ReactOS |
| DXVK | zlib [R23] | Permissive, compatible |
| 86Box (reference tool only, not shipped) | GPL-2.0 [R31] | — |

**Conclusion:** GPLv2-or-later + GPLv2-or-later + LGPL + zlib can all be distributed together under **GPLv2** terms. Shipping separate programs on one ISO is mere aggregation anyway. Obligations: (1) publish **complete corresponding source** for every binary on the ISO (our ReactOS fork at the release tag, the exact DOSBox-X tag, the WineVDM tag, build scripts) alongside each release; (2) keep all copyright/license notices; (3) **don't pull in GPLv3-only or proprietary code** (it would conflict with any GPLv2-only file); (4) **never ship Microsoft binaries** (MS-DOS, Win3.x/9x, DirectX redistributables, fonts) or copyrighted games/ROMs. Users bring their own Windows 3.1/9x media for guest modes. Branding: no "Windows" in the product name, no Microsoft logos. Credit "based on ReactOS" and check ReactOS's trademark/branding expectations before public release.

### 2.5 Prior art
- **ReactOS NTVDM**: in-tree DOS/Win16 VDM. Instructive, but limited (§2.1). We keep it buildable as a fallback, not as the default [R17][R18].
- **WineVDM/otvdm**: best-in-class Win16-on-NT without a guest OS; already integrated upstream [R1][R16].
- **Batocera**: Linux console-style retro distro. DOS games are folder-per-game with a launch script, run through DOSBox / DOSBox-X / DOSBox Pure / DOSBox Staging. It's the model for our launcher UX and per-game config, not for our OS base [R29][R32].
- **86Box**: low-level, period-accurate PC emulator. We use it (with user-owned Win98) as a **reference "golden machine"** to tell game bugs from ReactOS bugs, and to try ReactOS on emulated old hardware [R31].
- **PicoGUS / PicoMEM**: Raspberry Pi Pico ISA cards for real retro PCs. PicoGUS's GUS/MPU-401 emulation is adapted from DOSBox-X [R33][R34]. That proves DOSBox-X's device code is reusable. They're out of scope for hardware, but useful as reference implementations.

---

## 3. Architecture

```
+----------------------------------------------------------------------------+
|  VetTV Launcher Shell (vettvshell.exe, replaces explorer via Winlogon\Shell)|
|  library grid · pad/keyboard nav · per-game profiles · "Desktop mode" exit |
+------------------+-----------------------+---------------------------------+
| DOS .EXE/.COM/.BAT| Win16 NE .EXE         | Win32 PE .EXE (i386)            |
|        |          |        |              |        |                        |
|        v          |        v              |        v                        |
|  vetvdm.exe  <----+-- kernel32 VDM hook --+   native ReactOS userland       |
|  (policy +        |   (BaseCheckVDM path) |   + Wine-derived DirectX        |
|   conf generator) |                       |   (ddraw/d3d8/d3d9 -> WineD3D   |
|        |          |   winevdm (otvdm)     |    -> OpenGL: HW ICD or Mesa SW)|
|        v          |   fallback: DOSBox-X  |   optional: DXVK (VM, Vulkan)   |
|   DOSBox-X (XP    |   + user-owned Win3.1 |                                 |
|   build, SDL)     |                       |                                 |
+------------------+-----------------------+---------------------------------+
|                 ReactOS i386 (fork of master or releases/0.4.16)            |
|   ntoskrnl · win32k · new ATA · HDA bus · VBE/VGA fallback · NT5.x drivers |
+----------------------------------------------------------------------------+
|  QEMU / VirtualBox / 86Box   |   Asus GL552VW (legacy CSM, SATA 2.5" SSD)  |
+----------------------------------------------------------------------------+
```

### 3.1 Base: ReactOS i386
- 32-bit only until upstream WOW64 lands. Fork from upstream `master` to get PR #9352 (Wine-10 DirectX), pinned to a known-good commit. Keep a fallback branch on `releases/0.4.16` if master regresses.
- Build the image with the regular `bootcd` target. Add-ons go in `modules/optional/` and get installed by `syssetup/addons.c` (§2.3).

### 3.2 16-bit layer: DOSBox-X invoked transparently
1. **Bundle:** pin the DOSBox-X XP-compatible 32-bit build (start with the prebuilt `winXP` portable/setup from a pinned tag, with SHA-256 recorded). Later, optionally build from source in CI. Install to `%SystemDrive%\VetTV\dosbox-x\` through a new `{L"DOSBox-X", ...}` entry in `addons.c` + `reactos.dff.in`.
2. **Handoff hook (M2):** in `dll/win32/kernel32/client/proc.c`, where `CreateProcessInternalW` handles `STATUS_INVALID_IMAGE_NOT_MZ` / `STATUS_INVALID_IMAGE_PROTECT` (DOS) and `STATUS_INVALID_IMAGE_WIN_16` (Win16) and calls `BaseCheckVDM`, insert a policy check that, when enabled, re-targets the launch to `vetvdm.exe "<original path>" <args>` (a normal Win32 CreateProcess). This keeps the patch tiny and in one place. Explorer, cmd.exe, the launcher and installers all get the same behavior for free.
3. **`vetvdm.exe` (our code, in `modules/vettv/vetvdm/`):** reads policy from `HKLM\Software\VetTV\RetroOS\VDM` (`DosMode = dosboxx|ntvdm|ask`, `Win16Mode = winevdm|dosboxx|ask`). It looks up a per-game profile (by EXE hash/path) or applies defaults, writes a temp `dosbox-x.conf` (mount the game dir as `C:`, `[autoexec]` runs the EXE then `exit`, machine/cycles/sound defaults), launches DOSBox-X, and passes the exit code back. A "Win3.1 guest" profile mounts the user-provided Win3.1 install and runs `WIN <exe>`.
4. **Win16 default = WineVDM** (already present in 0.4.16 images) because it renders Win16 windows on the native desktop. vetvdm falls back to DOSBox-X+Win3.1 for titles flagged in the compat DB.
5. **NTVDM** stays in the build as `DosMode=ntvdm` for comparison and debugging.

### 3.3 32-bit games
- Native ReactOS userland plus the in-tree Wine-derived DirectX stack (ddraw, d3d8/9, dinput, dsound, dplayx).
- Rendering: WineD3D → OpenGL, using a hardware ICD where a vendor NT5 driver works, otherwise ReactOS's Mesa software renderer. In VMs, VirtualBox/QEMU display drivers (VBE/VGA, VMSVGA where supported).
- **Optional (late): DXVK / Vulkan** only in VMs or Mesa-software experiments. Never a dependency.
- Per-game shims (registry/INI tweaks, `DLL_EXPORT_VERSION` builds, windowed mode) live in launcher profiles, not core patches.

### 3.4 VetTV launcher shell
- `vettvshell.exe` (C/C++ Win32 or a lightweight SDL2 app, XP-compatible toolchain), set as `HKLM\...\Winlogon\Shell`, with "Exit to desktop" starting `explorer.exe`.
- Library = folders under `D:\Games\<Title>\` with `vettv.game.json` (title, platform, layer, exe, args, profile, art), Batocera-style folder-per-game [R32].
- Gamepad (XInput/DirectInput) + keyboard navigation, 10-foot UI, original VetTV art only (no MS/console trademarks).
- An "Add game" wizard that scans a user folder and classifies EXEs (MZ/NE/PE) automatically.

---

## 4. Repo / fork strategy
*(Nothing has been forked or created. Every GitHub write needs the owner's explicit approval.)*

- **GitHub account:** `Vet-TV` (the `gh` CLI on the shared box is authenticated as this account, §6).
- **Repos (proposed):**
  - **Home repo (since 2026-10-19): `VetTV-Studios/reactos`** → org fork of `reactos/reactos`. `Vet-TV/reactos` (personal fork, the original home) is kept as a read-only backup; don't push new work there.
  - `Vet-TV/dosbox-x` → fork **only if** we need patches. Otherwise consume pinned upstream release artifacts plus the matching source tarball.
  - Launcher, vetvdm and compat DB live **inside** the OS fork under `modules/vettv/` to avoid multi-repo overhead.
- **Branches in `VetTV-Studios/reactos`:**
  - `master`: pristine mirror of upstream (fast-forward only, never commit).
  - `vettv/main`: default integration branch = upstream + VetTV patches.
  - `vettv/feat/<topic>`: short-lived feature branches (e.g., `vettv/feat/vdm-hook`, `vettv/feat/shell`). PR into `vettv/main`.
  - `vettv/release/0.x`: release branches, tagged `vettv-0.1.0` etc. Each tag records the upstream SHA.
  - `vettv/stable-0416`: optional fallback based on `releases/0.4.16`.
- **Upstream sync:** weekly (or before each milestone) fast-forward `master` from upstream, then **merge** (not rebase, so the history stays honest and nothing is force-pushed) into `vettv/main`. Enable `git rerere`. Rule: the downstream diff outside `modules/vettv/` must stay small (target < 500 lines: the kernel32 hook, addons.c/dff entries, branding, default hives). Track it with `git diff upstream/master...vettv/main --stat` in CI.
- **Upstreaming:** generally useful fixes (e.g., NTVDM or WineD3D bugs found by QA) go as separate PRs to `reactos/reactos` from clean branches, only with owner approval.
- **Clone hygiene:** ReactOS history is large. Use `git clone --filter=blob:none` (partial clone) on the build box, and skip `--mirror` clones.

---

## 5. Build & test setup

### 5.1 Build hosts
| Host | Role | Notes |
|---|---|---|
| **Shared box** (Linux x86_64, 8 vCPU, 15 GB RAM, ~107 GB free) | Primary build host (RosBE-Unix 2.2.x) + QEMU smoke tests | `/dev/kvm` exists but **is not accessible to the `box` user** (root:103, user not in group), so QEMU runs in slow TCG mode. Enabling KVM is an owner/admin decision. Work dir: `/workspace/vettv-retro-os/` |
| **GitHub Actions** on the fork | CI build of `bootcd` (copy upstream `build.yml`: gcc i386 Release + Debug), artifact = ISO | Public-repo minutes are free. Add a QEMU boot-smoke job (check KVM availability on hosted runners first) |
| **late-2015 iMac** (i5-6500, 32 GB) | VM host (VirtualBox/QEMU/86Box), optional RosBE on macOS | 32 GB RAM makes it the best VM box. Bare-metal ReactOS on the iMac is **not** a target |
| **Asus GL552VW** | Real-hardware target (M5) | Also usable as a RosBE-Windows build host while it still runs Windows |

### 5.2 Build commands (reference)
```bash
# inside RosBE-Unix (i386)
git clone --filter=blob:none https://github.com/VetTV-Studios/reactos.git src   # home repo since 2026-10-19 (was Vet-TV/reactos)
mkdir build && cd build
../src/configure.sh -DENABLE_ROSTESTS=1          # Ninja is default
ninja bootcd                                      # -> bootcd.iso (installer + live)
# optional Vista+ exports experiment:
../src/configure.sh -DDLL_EXPORT_VERSION=0x600
```
Windows: install RosBE for Windows, then `configure.cmd` and `ninja bootcd` [R14][R15].

### 5.3 VM test images (CI and local)
- **QEMU (headless, scripted):** `qemu-system-i386 -m 2048 -cdrom bootcd.iso -hda test.qcow2 -serial file:serial.log -display none -qmp ...`. Do unattended install with upstream's `boot/bootdata/bootcdregtest/unattend.inf` approach (the regtest bootcd). Pass/fail = serial log markers + QMP `screendump` screenshot of the desktop/launcher. A Sound Blaster 16 / AC97 device and `-usb -device usb-tablet` cover input and audio.
- **VirtualBox (local, iMac):** a golden VM with ICH AC97, PIIX4/AHCI, 2 GB RAM, legacy BIOS, plus snapshots `clean-install`, `games-installed`.
- **86Box (reference):** (a) a user-owned Win98SE "golden machine" to confirm a game works on period-correct hardware/OS; (b) optional ReactOS-on-86Box for old-hardware driver paths.
- **Game payload disk:** a separate `games.vhd/qcow2` built by script from a local, **not-redistributed** folder of freeware/shareware/user-owned games. It never goes into git or an ISO.

### 5.4 Real hardware (Asus GL552VW)
- Firmware: enable **CSM/Legacy boot**, disable Secure Boot. SATA mode AHCI (or IDE/compat if the option exists, for first boot).
- Disk: install to a **dedicated spare 2.5" SATA SSD in the bay** (or the SATA M.2), with the existing OS disk removed or unplugged to protect data (ReactOS is alpha; corruption warnings [R3]).
- Boot media: Skylake has **xHCI-only USB**, and ReactOS USB boot was added for EHCI/OHCI/UHCI chipsets [R6], so USB install may fail. Plan B: write the ISO or a pre-installed VM disk image directly to the SSD from another machine.
- Expected drivers: display → **VBE/VGA fallback** (Intel HD 530 has no NT5 driver; the GTX 960M is behind Optimus and drives no panel directly), so 3D = Mesa software. Audio → Realtek HDA via the new 0.4.16 HDA bus (to be tested). Keyboard/touchpad are usually PS/2 (i8042). Networking is optional (vendor XP driver if it works).
- The laptop has no serial port, so capture ReactOS debug output to a log file (or via a debug build's on-screen output) and record a **phone/screen-capture video** of each QA run.

---

## 6. Tooling note: GitHub CLI on the shared box
No Cursor cloud agents on this plan. Code work runs through `gh` + `git` on the shared box under `/workspace`, with owner approval for every remote write.

`gh auth status` (checked Fri Oct 2, 2026, ~1:55 PM PT): **authenticated**. Logged in to github.com as **`Vet-TV`** (active account), git protocol https, token scopes `gist`, `read:org`, `repo`, `workflow`. gh 2.46.0. (Enough to fork, push and run Actions *if and when* the owner approves. Nothing was forked, cloned, pushed or created.)

---

## 7. Milestones (M0–M5)

| # | Goal | Concrete exit criteria (all must pass) | Rough effort |
|---|---|---|---|
| **M0** | Fork builds & boots in a VM | (1) Owner-approved fork `Vet-TV/reactos` with `vettv/main` branch. (2) `ninja bootcd` succeeds on the shared box with RosBE-Unix **and** in GitHub Actions; ISO uploaded as artifact. (3) ISO installs unattended in QEMU and reaches the desktop; serial log + screenshot archived. (4) Same ISO installs manually in VirtualBox on the iMac. (5) A trivial branding change (boot screen/version string "VetTV Retro OS 0.0.1") is visible, proving the patch→build→boot loop | 1–2 weeks |
| **M1** | DOSBox-X bundled, launches a DOS exe from the shell | (1) DOSBox-X pinned build (tag + SHA-256 in `modules/vettv/deps.lock`) installed by the setup add-on mechanism. (2) Start menu / desktop shortcut "VetTV DOS" opens DOSBox-X. (3) From `cmd.exe` *and* Explorer, `dosbox-x.exe -c "mount c D:\Games\Doom" -c "c:" -c "doom"` runs **Doom shareware** with sound (SB16) in QEMU and VirtualBox. (4) License audit file `THIRD_PARTY.md` + source-offer page drafted | 1–2 weeks |
| **M2** | Transparent .exe handoff for DOS & Win16 | (1) The kernel32 VDM hook + `vetvdm.exe` are merged. Double-clicking `KEEN1.EXE` or `DOOM.EXE` in Explorer launches DOSBox-X automatically, with no prompts, and closes back to the desktop on exit. (2) Double-clicking **SkiFree (Win16)** runs it via WineVDM. A registry flip routes it to DOSBox-X+Win3.1 (user-owned) instead. (3) `DosMode=ntvdm` still works (regression check). (4) Exit codes pass back to `cmd.exe`. (5) The downstream diff outside `modules/vettv/` is < 500 lines | 2–4 weeks |
| **M3** | 32-bit game test suite | (1) ~22 titles in §7.1 installed on the game disk. (2) Full matrix (§8) filled for QEMU + VirtualBox on a tagged build. (3) Targets met: DOS ≥ 95% Gold+, Win16 ≥ 85%, Win32-2D ≥ 75%, Win32-3D ≥ 50% (VM). (4) Every Silver-or-worse title has an issue with logs and a triage label (ours / upstream ReactOS / Wine / game). (5) A re-runnable scripted smoke test covers ≥ 10 titles (launch + 60 s + screenshot) | 3–6 weeks |
| **M4** | VetTV launcher shell | (1) `vettvshell.exe` is the default shell after install. "Exit to desktop" and "Return to launcher" both work. (2) Library shows all M3 titles with art, and launches each with its profile through the correct layer. (3) Fully playable with a USB/XInput gamepad *and* keyboard only. (4) "Add game" wizard classifies MZ/NE/PE correctly for all M3 titles. (5) Cold boot → launcher in < 60 s in VirtualBox | 3–5 weeks |
| **M5** | Asus bare-metal install | (1) Installs to the dedicated SATA SSD on the GL552VW via CSM boot (USB or pre-imaged disk). (2) Boots to the launcher 10/10 cold boots with no bugcheck. (3) Internal keyboard + touchpad (or USB mouse) + audio (HDA) work, or a documented workaround exists. (4) DOS suite ≥ 90% Gold+, Win16 ≥ 80%, Win32-2D ≥ 70%. 3D results recorded (software Mesa expected). (5) A 30-minute play session each of Doom (DOSBox-X), SkiFree (WineVDM) and Half-Life demo (software renderer) without a crash, captured on video for VetTV content | 2–4 weeks (+ driver unknowns) |

Stretch (post-M5): UEFI boot on the Asus; x64 + WOW64 once upstream lands it; DXVK experiment in VMs; DOSBox-X user-owned Win98 "guest mode" button for stubborn titles; upstream contributions.

### 7.1 Target game list (freeware / shareware / legally-owned only)
| # | Title | Platform | Legal basis | Layer | Exercises |
|---|---|---|---|---|---|
| 1 | Doom (shareware v1.9, DOOM1.WAD) | DOS | Shareware | DOSBox-X | DOS4GW, VGA mode X, SB/OPL |
| 2 | Commander Keen 1 (shareware) | DOS | Shareware | DOSBox-X | EGA, PC speaker/AdLib |
| 3 | Wolfenstein 3D (shareware) | DOS | Shareware | DOSBox-X | VGA, SB digital |
| 4 | Duke Nukem 3D (shareware) | DOS | Shareware | DOSBox-X | Build engine, VESA |
| 5 | Cosmo's Cosmic Adventure ep.1 | DOS | Shareware | DOSBox-X | EGA, AdLib |
| 6 | One Must Fall 2097 | DOS | Freeware (since 1999) | DOSBox-X | SB, DOS extender, timing |
| 7 | Tyrian 2000 | DOS | Freeware | DOSBox-X | VGA, SB, joystick |
| 8 | Beneath a Steel Sky (DOS CD) | DOS | Freeware | DOSBox-X | CD-ROM image, speech |
| 9 | Flight of the Amazon Queen | DOS | Freeware | DOSBox-X | Adventure, SB |
| 10 | Lure of the Temptress | DOS | Freeware | DOSBox-X | EGA/VGA adventure |
| 11 | SkiFree (Win16, 1991) | Win16 | Author-distributed free | WineVDM | Win16 GDI |
| 12 | Castle of the Winds | Win16 | Freeware (since 1998) | WineVDM | Win16 dialogs/menus |
| 13 | Solitaire / Minesweeper from **user-owned** Win 3.1 | Win16 | User-owned media | DOSBox-X+Win3.1 & WineVDM | Fallback path, comparison |
| 14 | Microsoft Entertainment Pack title (e.g., Chip's Challenge) | Win16 | **User-owned** only | WineVDM | Win16 multimedia |
| 15 | Half-Life (Uplink demo) | Win32 | Free demo | Native | Software/OpenGL/D3D renderers |
| 16 | Unreal Tournament (UT99 demo) | Win32 | Free demo | Native | D3D/OpenGL/software |
| 17 | Quake III Arena demo | Win32 | Free demo | Native | OpenGL (Mesa/ICD) |
| 18 | Command & Conquer: Red Alert (Win95 ver.) | Win32 | Freeware (EA, 2008) | Native | DirectDraw, DirectSound |
| 19 | StarCraft (Anthology, free since 2017; use the old 1.16-era build only if user-owned) | Win32 | Free from publisher / user-owned | Native | DirectDraw 640×480×8 |
| 20 | Diablo (shareware/spawn demo) | Win32 | Shareware demo | Native | DirectDraw, palette |
| 21 | Zuma Deluxe (trial, user-owned) | Win32 | Trial / user-owned | Native | D3D7/DDraw, listed on ROS wiki [R19] |
| 22 | Max Payne demo | Win32 | Free demo | Native | D3D8 |

*Verify each title's current redistribution terms before downloading it, and store the source URL + SHA-256. Games are **never** committed to git, uploaded to releases, or baked into ISOs.*

---

## 8. Game-compat test matrix template

Rating scale: **Platinum** = flawless · **Gold** = playable start-to-finish, minor cosmetic issues · **Silver** = playable with workaround/profile tweak · **Bronze** = launches, major issues (no sound, crashes later) · **Broken** = doesn't start / crashes immediately · **N/T** = not tested.

```markdown
| ID | Title | Ver/Source (URL) | SHA-256 | Legal basis | Platform | APIs/HW | Layer | Env | Build (vettv tag / upstream sha) | Install | Launch | Menu | Play 5min | Audio | Input (kb/mouse/pad) | Save/Load | Clean exit | Rating | Issue | Tester (bot/owner) | Date (PT) | Notes |
|----|-------|------------------|---------|-------------|----------|---------|-------|-----|-----------------------------------|---------|--------|------|-----------|-------|-----------------------|-----------|------------|--------|-------|--------------------|-----------|-------|
| G001 | Doom shareware 1.9 | <url> | <hash> | Shareware | DOS | DOS4GW, VGA, SB16 | DOSBox-X 2026.10.01 | QEMU-TCG | vettv-0.0.2 / bac97c5 | ✅ | ✅ | ✅ | ✅ | ✅ | ✅/✅/– | ✅ | ✅ | Gold | – | QA bot | 2026-10-xx | cycles=max |
| G011 | SkiFree | <url> | <hash> | Free | Win16 | GDI | WineVDM 0.9.0 | VBox | … | n/a | | | | | | | | | | | | |
| G015 | Half-Life Uplink | <url> | <hash> | Demo | Win32 | SW/OGL/D3D | Native + WineD3D | Asus | … | | | | | | | | | | | | | renderer=software |
```
Env codes: `QEMU-TCG`, `QEMU-KVM`, `VBox-iMac`, `86Box-W98(ref)`, `Asus-CSM`. One row per title × env × build. Keep the matrix as `compat/matrix.csv` in the fork, and render markdown from it.

---

## 9. Bot team (one lane each)

Global boundaries for **every** bot:
- **No pushing, forking, PR creation, issue creation, releases or any other GitHub/remote write without the owner's explicit approval** for that specific action. Bots prepare branches/patches locally under `/workspace/vettv-retro-os/` and hand over a diff summary.
- **No piracy:** never download, store, share or distribute copyrighted games, ROMs, BIOS dumps, MS-DOS/Windows media or Microsoft redistributables. Test only freeware, shareware/demo and **owner-owned** titles the owner provides locally. Never put games in git, CI artifacts or ISOs.
- **No messaging or posting** (email, Slack, X, YouTube, forums, upstream trackers) on the owner's behalf without approval.
- Respect licenses: keep notices, record sources, and add no GPLv3-only or proprietary code.
- Don't touch other `/workspace` projects. Don't change box system settings (e.g., KVM group) without approval.
- Clearly label upstream-bound work vs. VetTV-only work.

| Bot | Lane | Responsibilities | Inputs | Outputs | Boundaries (extra) |
|---|---|---|---|---|---|
| **RetroOS Lead** | Lead / Planner | Owns BRIEF.md, milestone plan, task breakdown, weekly upstream-sync calendar, risk register. Routes work to lanes, reviews hand-offs, and prepares approval requests for the owner (exact git/GitHub commands + diff stat) | Owner goals, bot reports, upstream news (ReactOS/DOSBox-X releases) | `PLAN.md`, `STATUS.md` (weekly), approval request drafts, milestone sign-off checklists | Doesn't write kernel/shell code itself. Never runs remote-write commands. Only marks a milestone done when every exit criterion has evidence |
| **RetroOS Build & Kernel** | ReactOS build & kernel engineer | RosBE setup on the box, CMake/Ninja builds, CI workflow on the fork, QEMU/VBox boot automation, kernel32 VDM hook, syssetup add-on entries, branding, upstream merges and conflict resolution, x64/UEFI watch | Upstream ReactOS source, Lead tasks, QA bug reports | Local branches/patches, build logs, `bootcd.iso`, boot screenshots/serial logs, merge notes, upstream-PR drafts | Keeps the downstream diff outside `modules/vettv/` minimal. No force-push or history rewrite. No upstream PRs without approval |
| **RetroOS DOSBox-X Integration** | 16-bit layer engineer | Pin/verify DOSBox-X builds, `deps.lock`, `vetvdm.exe` (policy, conf generator, profiles), WineVDM routing, DOSBox-X+Win3.1 fallback profile, NTVDM fallback, per-game DOS profiles, `THIRD_PARTY.md` + source-offer bundle | DOSBox-X/WineVDM releases & docs, Build bot's ISO, QA results | `modules/vettv/vetvdm/`, default `dosbox-x.conf` templates, profile JSON, license audit | Uses only upstream-published binaries/sources with recorded hashes. Never bundles MS-DOS/Windows files. The Win3.1/9x guest modes need user-supplied media |
| **RetroOS Compat QA** | Compatibility QA tester | Builds the local game disk from owner-provided/legal sources, runs the matrix on QEMU/VBox (and guides owner runs on Asus/86Box), scripted smoke tests, triage (ours/ReactOS/Wine/game), regression tracking across builds | ISO builds, §7.1 list, owner-supplied game folders | `compat/matrix.csv`, screenshots/videos, bug reports with repro + logs, per-milestone pass-rate summary | Tests only freeware/shareware/demo/owner-owned games. Doesn't upload game files or saves anywhere. Doesn't file upstream bugs without approval |
| **RetroOS Shell/UX** | Launcher shell engineer | `vettvshell.exe`: library grid, game JSON schema, pad/keyboard nav, "Add game" wizard (MZ/NE/PE detection), Winlogon shell switch, VetTV visual identity (original art only), boot-to-launcher timing | Brand assets from owner, game profiles from Integration bot, QA usability notes | `modules/vettv/shell/`, UI mockups/screens, `SHELL.md` | No Microsoft/console trademarks or ripped assets. Must stay XP-API-compatible (i386, NT5 exports) |
| *(optional)* **RetroOS Release Steward** | Licensing & release compliance | Pre-release license/attribution audit, source-tarball assembly, release notes, checksum manifests, trademark/branding check | All lanes' outputs | `RELEASE_CHECKLIST.md`, draft release notes, source bundle | Prepares releases. Never publishes them. Not legal advice; flags anything uncertain to the owner |

Hand-off protocol: each bot writes to `/workspace/vettv-retro-os/lanes/<bot>/` (`inbox.md`, `outbox.md`, `log.md`). The Lead merges status weekly. Any remote write is listed in `lanes/lead/approvals.md` as an exact command for the owner to approve.

---

## 10. Risks & mitigations

| Risk | Impact | Likelihood | Mitigation |
|---|---|---|---|
| ReactOS alpha instability (bugchecks, FS corruption) [R3] | High | High | VMs first. Dedicated test SSD on the Asus. Pin known-good upstream SHAs. Keep a `stable-0416` fallback |
| **No GPU acceleration on the Asus** (HD 530 has no NT5 driver; 960M behind Optimus; Intel HD 4400/4600 drivers already fail to boot on ReactOS) [R35] | High for 3D | High | Accept VBE + Mesa software for 3D on bare metal. Prioritize DOS/Win16/2D, which DOSBox-X handles CPU-side. Choose software-renderer-friendly titles (Half-Life SW, UT99 SW). Document this honestly in VetTV content |
| xHCI-only USB on Skylake → USB boot or peripherals may fail [R6] | Med | Med-High | Pre-image the SSD from a VM. Use internal PS/2 keyboard/touchpad. Test xHCI support per upstream build |
| UEFI path immature [R8] | Med | Med | Use CSM/legacy. UEFI is a stretch goal |
| No WOW64 on x64 [R4][R7] | Med | Certain (near term) | i386-only plan |
| Upstream churn (e.g., PR #9352 DirectX rewrite regressions) [R9] | Med | Med | Merge weekly behind CI. Bisect with QA smoke suite. Pin |
| DOSBox-X on ReactOS is "experimental" (audio glitches/instability) [R12][R13] | Med | Med | Try XP VS build vs MinGW lowend builds. Tune SDL audio buffers. Report upstream with approval |
| Kernel32 hook breaks installers/legit 16-bit stubs | Med | Med | Policy registry switch + per-EXE allow/deny list. NTVDM fallback. Regression test installers |
| License/compliance mistakes (missing source, MS binaries slipping in) | High | Low-Med | Release Steward checklist, `deps.lock` with hashes, CI check that forbids known-MS filenames in the ISO |
| Legal/piracy exposure in VetTV videos or releases | High | Low | Only legal titles. Owner-owned media stays local. Watermark-free original branding. No "Windows" in the name |
| Solo-dev bandwidth / AI-generated code quality in kernel-adjacent code | Med | High | Keep the kernel diff tiny. Put most work in user-mode `modules/vettv/`. Owner review gates. Small PRs |
| Box limits (no KVM for `box` user, 15 GB RAM) | Low-Med | Certain | TCG for smoke tests, iMac VirtualBox for interactive tests, Actions for builds. Enabling KVM is the owner's call |
| **RK-USERS** Upstream #9573 (merged Oct 17 2026) moves profiles to `C:\Users` / `AppData` / `ProgramData`, no compat junctions; XP-era titles/installers with hardcoded `Documents and Settings` paths break, saves move | Med | Med | Hold sync past 95bba32e55 until Build & Kernel audit + QA save-path checks pass; our code uses env vars only (see lanes/lead/RK-USERS-PROFILE.md). Owner: Build & Kernel (gate), QA (matrix) |
| reactos.org wiki behind Anubis (bots can't read compat lists) | Low | Certain | Owner opens pages in a browser when needed. Rely on GitHub sources/JIRA archives |
| ReactOS software OpenGL 1.1 draws garbage and crashes (fault offset …a8f1, the same in upstream and our DOSBox-X builds; QA m0-5-dbx) | High for GL games (Win32 3D target) | Certain on the current build | vetvdm forces output=surface. Check that GL titles in the matrix fail at the GL check, report it upstream after Vet approves, and look at Mesa llvmpipe/softpipe as an M2 option |

---

## 11. Immediate next steps (all need owner approval where marked)
1. **[Approval]** Fork `reactos/reactos` → `Vet-TV/reactos`, create `vettv/main`.
2. Install RosBE-Unix 2.2.x on the box, do a partial clone, run `ninja bootcd` locally (M0 #2).
3. Script the QEMU TCG unattended install + screenshot (M0 #3).
4. **[Approval]** Enable GitHub Actions on the fork (copy upstream `build.yml`).
5. Download DOSBox-X 2026.10.01 XP build + source tarball, record hashes, try it on the M0 ISO in VirtualBox (M1 prep).
6. Owner: confirm which Win3.1/Win9x media and games you legally own, for the fallback and test lanes.

---

## References (accessed Oct 2, 2026)
- [R1] ReactOS Team, "Introducing ReactOS 0.4.16" (Aug 29, 2026): https://reactos.org/project-news/reactos-0416-released/
- [R2] Phoronix, "ReactOS 0.4.16 Released With New Graphical Installer, Better Hardware Support": https://www.phoronix.com/news/ReactOS-0.4.16-Released
- [R3] ReactOS Download page (alpha warning): https://reactos.org/download/ · README "Product quality warning": https://github.com/reactos/reactos
- [R4] ReactOS Wiki, 0.4.15 (x64 boots, no WOW64; RosBE 2.2.1 used): https://reactos.org/wiki/0.4.15
- [R5] reactos/reactos GitHub repo: README/COPYING, `.github/workflows/build.yml`, `dll/win32/kernel32/client/proc.c`, `subsystems/mvdm/`, `dll/directx/wine/`, `dll/win32/syssetup/addons.c`, `boot/bootdata/packages/reactos.dff.in`, releases (`0.4.16-release`): https://github.com/reactos/reactos
- [R6] ReactOS, "ReactOS 0.4.15 released" + Community Changelog-0.4.15: https://reactos.org/project-news/reactos-0415-released/ · https://reactos.org/wiki/Community_Changelog-0.4.15
- [R7] PR #8713 "[NTOSKRNL] Kernel changes in preparation for WOW64" (merged): https://github.com/reactos/reactos/pull/8713
- [R8] PR #5267 "[FREELDR] Enable UEFI boot for i386 and amd64" (merged); commit 229f789; Wiki "UEFI Freeldr": https://github.com/reactos/reactos/pull/5267 · https://github.com/reactos/reactos/commit/229f7891a1c707ca2c51a9588ca5964a9751622a · https://reactos.org/wiki/UEFI_Freeldr
- [R9] PR #9352 "[DIRECTX:WINE] Import DirectX from Wine-10.1-dev" (merged Sep 24, 2026): https://github.com/reactos/reactos/pull/9352
- [R10] Phoronix, "ReactOS Lands Significant DirectX Stack Update From Wine For Enabling More Games" (Sep 24, 2026): https://www.phoronix.com/news/ReactOS-Updated-Wine-DirectX
- [R11] DOSBox-X Wiki, Microsoft Windows Installation Guide: https://dosbox-x.com/wiki/Guide:Microsoft-Windows-in-DOSBox%E2%80%90X
- [R12] DOSBox-X INSTALL.md (XP builds work in ReactOS, experimental): https://github.com/joncampbell123/dosbox-x/blob/master/INSTALL.md
- [R13] DOSBox-X PR #4623 "XP build works under ReactOS"; issue #1269 "Begin testing/developing Windows builds of DOSBox-X on ReactOS"; issue #3577: https://github.com/joncampbell123/dosbox-x/pull/4623 · https://github.com/joncampbell123/dosbox-x/issues/1269 · https://github.com/joncampbell123/dosbox-x/issues/3577
- [R14] ReactOS Wiki, Building ReactOS: https://reactos.org/wiki/Building_ReactOS · A Layman's Guide: https://reactos.org/wiki/A_Layman%27s_Guide_-_How_to_Compile_ReactOS
- [R15] RosBE-Unix 2.2.1 README; "ReactOS Build Environment 2.2 released": https://github.com/reactos/RosBE/blob/b6a44c4e/RosBE-Unix/Base-i386/README · https://reactos.org/project-news/rosbe-22-released/
- [R16] otya128/winevdm repo + README: https://github.com/otya128/winevdm
- [R17] ReactOS blog, "NTVDM progress": https://reactos.org/blogs/ntvdm-progress/
- [R18] BTTR forum, "DOS ain't dead – ReactOS 0.4.0 alpha"; JIRA CORE-8740: https://www.bttr-software.de/forum/mix_entry.php?id=14646 · https://reactos.org/archives/public/ros-bugs/2015-September/081203.html
- [R19] ReactOS Wiki, Games ROS Testing: https://reactos.org/wiki/Games_ROS_Testing
- [R20] ReactOS Wiki, Version Status: https://reactos.org/wiki/Version_Status · ReactX/DirectX: https://reactos.org/wiki/DirectX
- [R21] Community ReactOS Compatibility List: https://voluxian.neocities.org/ReactOSCompat/Compat
- [R22] PR #5688 "[WINESYNC] Update our DirectX import to wine-staging-4.18 and Support DirectX10/11" (merged): https://github.com/reactos/reactos/pull/5688
- [R23] DXVK repo (zlib) and Driver-support wiki: https://github.com/doitsujin/dxvk · https://github.com/doitsujin/dxvk/wiki/Driver-support
- [R24] DOSBox-X homepage (release Oct 1, 2026; Win3.x/9x officially supported): https://dosbox-x.com/
- [R25] "DOSBox-X 2026.10.01 adds S3 ViRGE and AWE32 support": https://pantspantsrevolution.com/dosbox-x-2026-10-01-adds-s3-virge-and-awe32-support/
- [R26] DOSBox-X releases (2026.10.01; 2026.08.02 assets; master vs main-osfree note): https://github.com/joncampbell123/dosbox-x/releases · https://github.com/joncampbell123/dosbox-x/releases/tag/dosbox-x-v2026.08.02
- [R27] DOSBox-X homepage feature description: https://dosbox-x.com/
- [R28] DOSBox-X COPYING + source header (`src/dosbox.cpp`, GPLv2-or-later): https://github.com/joncampbell123/dosbox-x
- [R29] Batocera DOSBox-X package: https://github.com/batocera-linux/batocera.linux/blob/master/package/batocera/emulators/dosbox-x/dosbox-x.mk
- [R30] WineVDM v0.9.0 release (incl. otvdm-for-old-windows for ReactOS): https://github.com/otya128/winevdm/releases/tag/v0.9.0
- [R31] 86Box repo (GPL-2.0, v6.0 May 2026): https://github.com/86Box/86Box
- [R32] Batocera Wiki, DOS system: https://wiki.batocera.org/systems:dos
- [R33] PicoGUS: https://picogus.com/ · https://github.com/polpo/picogus
- [R34] ISA PicoMEM: https://github.com/FreddyVRetro/ISA-PicoMEM/
- [R35] ReactOS JIRA archives CORE-18026 (Intel HD 4600 driver boot failure), CORE-17594 (HD 4400): https://reactos.org/archives/public/ros-bugs/2022-July/209662.html · https://reactos.org/archives/public/ros-bugs/2021-May/197159.html

*Some reactos.org wiki pages are behind an Anubis bot challenge and were cited from search-engine snippets. Re-verify in a browser before quoting them publicly.*
