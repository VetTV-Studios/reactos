# DOSBox-X: building from source for ReactOS i386 and XP-era Windows (research)

*Lane: dosbox-x · Written Fri Oct 2, 2026, ~3:10–4:00 PM PT · Pinned source: tag `dosbox-x-v2026.10.01`, commit `b6abbd5980a885f5f310a4088c59a8688d1b116c`, extracted to `lanes/dosbox-x/research/src/dosbox-x-dosbox-x-v2026.10.01/` (abbreviated `SRC/` below; not in git).*
*Evidence key: `SRC/path:Lnn` = line in the pinned tarball. "Measured" = checked on the box with `objdump -p` plus `research/tools/check_imports.py`, which compares every imported symbol with the ReactOS `.spec` exports in `/workspace/vettv-retro-os/src/reactos` (commit `8a5b9300c1`, 2026-10-02 01:03 PT) at the default `DLL_EXPORT_VERSION=0x502`.*

---

## 0. Decision summary

**M1 (now): ship the pinned upstream prebuilt `winXP` build. Don't build it ourselves yet.**
- Default exe: `dosbox-x_XPx86_SDL2.exe`. Fallback: `dosbox-x_XPx86_SDL1.exe`. Both come from `dosbox-x-winXP-2026.10.01-portable.zip`, which is already in `deps.lock`.
- Measured: PE OS/subsystem version 5.1. Every one of the 632 (SDL2) and 558 (SDL1) imported symbols is exported by ReactOS at 0x502. There is no MSVC runtime DLL import and no static XInput import.
- Corresponding source = the tag tarball we already keep.
- **Add `dosbox-x-mingw32_lowend-2026.10.01-portable.zip` as the A/B alternative** (sha256 `7c22259b…13b7`, matches GitHub's digest). It's a MinGW 7.3.0 build that links against the OS `msvcrt.dll` and contains no Microsoft CRT code. Measured clean against ReactOS 0x502 for both SDL1 and SDL2. These same two exes are already inside the XP `setup.exe`.

**Never use `vsbuild-win32` or `mingw32` (standard) on ReactOS.**
- Both look XP-ish: `vsbuild-win32` even has a 5.1 header after PatchPE.
- But the measured imports include Vista+ APIs: 32 such kernel32 functions in `vsbuild-win32`, plus SRW locks, thread pools, `inet_ntop`, `SHGetKnownFolderPath` and others in `mingw32`. They will fail to load on a default (0x502) ReactOS or on XP.

**M2+ (later): build our own XP/ReactOS binary by cross-compiling on Linux with mingw-w64, first on the box and then in GitHub Actions on the fork.**
- **Proven today on the box:** Debian `g++-mingw-w64-i686-posix` 14.2 with mingw-w64 12.0 built `dosbox-x.exe` (in-tree SDL2).
  - PE 5.1.
  - 790 imports, all of them stock NT5 system DLLs including the OS `msvcrt.dll`. All are present in ReactOS 0x502.
  - No libstdc++/libgcc/winpthread DLLs and no UCRT.
- It took two small upstreamable portability patches plus header shims (see §4).
- **Why this path:**
  - No Microsoft toolchain or static MS CRT ends up inside the binary.
  - It's reproducible on Linux (box or `ubuntu`/Debian runners).
  - We can patch for ReactOS bugs.
  - We control feature flags. Example: `--disable-xbrz` gives a clean GPLv2-or-later binary (§6).
- **Gate before it replaces the upstream exe:** a runtime smoke test on ReactOS (QEMU TCG/VirtualBox) and the CI import check.

**Not recommended as our main path:**
- Upstream's own VS2022 + v141 + `PatchPE.exe` route. It needs Windows runners and the Microsoft toolchain, and its output embeds the static MS CRT plus `dxguid.lib`. `PatchPE.exe` is an opaque committed binary whose in-tree "source" doesn't match its behaviour (§1.1).
- RosBE's GCC 8.4 as the compiler. It has the win32 thread model, so `std::thread` and `std::mutex` are missing (tested), and DOSBox-X needs them.

---

## 1. How upstream produces each Windows asset

The workflows live in `SRC/.github/workflows/`. Every Windows job runs on GitHub-hosted Windows runners. None of them cross-compiles from Linux.

### 1.1 `winXP` portable: `dosbox-x_XPx86_SDL1.exe` / `dosbox-x_XPx86_SDL2.exe` (plus x64 exes)
Source: `SRC/.github/workflows/vsbuild_xp.yml`, job `MSBuild_XP_CI_build` (L10–149).
- **Runner:** `windows-2022` (L15), with `microsoft/setup-msbuild@v3` (L25).
- **Toolset:** VS 2022 Enterprise plus the components `VC.v141.x86.x64`, `VC.v141.MFC` and `WinXP` (v141_xp), added via `vs_installer.exe modify`, run twice (L26–39).
  - The project files are then **sed-patched from v142/v143 to plain `v141`** and the SDK is pinned to `10.0.22621.0` (L44). So the build uses VS2017's v141 toolset, **not** `v141_xp`.
  - Measured linker version 14.16 = VS2017 15.9.
- **Build commands:**
  - `msbuild -m vs/dosbox-x.sln -t:dosbox-x:Rebuild -p:Configuration=Release -p:Platform=Win32` (SDL1, L58–63).
  - `... -p:Configuration="Release SDL2" ...` (L64–69).
  - The same again for x64 (L70–81).
- **XP header trick:** `contrib\windows\installer\PatchPE.exe <exe>` runs after each build (L63, L69). It rewrites the OS/subsystem version to 5.1 (measured), because the v141 linker with the Win10 SDK otherwise emits 6.0.
  - BUILD.md documents this: "change the target platform to v141 … patch the PE header" (`SRC/BUILD.md:L72-75`).
  - The in-tree source `SRC/vs/pehdr-lite/pehdr-lite.c` (roytam1, per `SRC/vs/pehdr-lite/SOURCE.TXT`) does **nothing** without `-osver/-subsysver` arguments. The committed `PatchPE.exe` must therefore be a build with different defaults: **an opaque binary**.
- **Project settings** (`SRC/vs/dosbox-x.vcxproj`):
  - `WindowsTargetPlatformVersion 10.0` (L82); `Release|Win32` is v142 before the sed (L85–88).
  - `RuntimeLibrary=MultiThreaded` (`/MT`, static CRT) and `EnableEnhancedInstructionSet=StreamingSIMDExtensions` (SSE required) (L789–806 SDL1, L841–853 SDL2; SDL2 also adds `/QIfist`).
  - Link: `winmm imm32 dsound dxguid SetupAPI version Iphlpapi Ws2_32 opengl32` plus the in-tree static libs (`libpdcurses`, `zlib`, `libpng`, `freetype`, `SDL`/`SDL2`, `SDLmain`, `SDL_net`) (L803, L852).
  - `dxguid.lib` is committed in `SRC/vs/extlib/x86/` (it's a Microsoft DirectX SDK lib), and the d3dx9 headers are committed in `SRC/vs/d3dx9*.h`.
- **Feature set** (hand-written `SRC/vs/config.h`):
  - `WINVER`/`_WIN32_WINNT 0x0501` (L22–31).
  - `C_DIRECT3D`, `C_D3DSHADERS`, `C_MT32`, `C_DEBUG`/`C_HEAVY_DEBUG`, `C_PCAP`, `C_XBRZ`, `C_FREETYPE`, `C_LIBPNG` and `C_SDL_NET` are on.
  - `C_FLUIDSYNTH` is undefined (L87) and `C_SLIRP` is off (L140).
  - FluidSynth is still compiled in: `src/gui/midi.cpp:L102` enables the internal synth for any `WIN32 && !HX_DOS && !_WIN32_WINDOWS` build, and the sources come from `SRC/src/libs/fluidsynth` (version `1.1.6-noglib`, `SRC/include/fluidsynth/version.h:L34`).
- **Bundled and statically linked:**
  - In-tree **SDL 1.2.15, heavily modified** (`SRC/vs/sdl`). BUILD.md says it's required and stock SDL1 won't do (`SRC/BUILD.md:L37-51`).
  - In-tree **SDL2 2.32.10** (`SRC/vs/sdl2/include/SDL_version.h`), SDL_net / SDL2_net.
  - zlib 1.3.1, libpng 1.6.47, FreeType 2.13.1, PDCurses.
  - mt32emu 2.8.0 (`SRC/src/libs/mt32/globals.h:L71`), FluidSynth 1.1.6-noglib, xBRZ, physfs, libchdr, Opus/Ogg/SpeexDSP, dr_libs/stb decoders, tinyfiledialogs.
- **Loaded at runtime only (no static import):**
  - `d3d9.dll` (`SRC/src/output/direct3d/direct3d.cpp:L128`) and `D3DX9_43.DLL` (`SRC/src/output/direct3d/d3d_components.h:L6997`).
  - `WPCAP.DLL` (`SRC/src/misc/ethernet_pcap.cpp:L62`).
  - XInput: SDL2 tries `XInput1_4` → `XInput1_3` → `bin\XInput1_3` → `XInput9_1_0` (`SRC/vs/sdl2/src/core/windows/SDL_xinput.c:L88-98`). SDL1 tries `xinput1_4` → `xinput1_3` → `xinput9_1_0` (`SRC/vs/sdl/src/joystick/win32/SDL_xinputjoystick.c:L41-43`).
  - `dwmapi.dll`, `d3d11.dll`, `glide2x.dll`.
  - OpenGL is a normal `opengl32.dll` import.
- **Packaging** (L82–127):
  - The exes are renamed `dosbox-x_XPx86_SDL1/SDL2.exe` / `XPx64_*`.
  - `dosbox-x.conf` is generated from `dosbox-x.reference.conf` with `output=ttf`, among other changes (L102).
  - **`inpout32.dll`** is added (L107). Fonts, shaders, languages and COPYING go in too.
  - The zip is made with the committed `vs/tool/zip.exe` (L122).

### 1.2 `winXP` setup: `dosbox-x-winXP-<ver>-setup.exe`
- Source: same workflow, job `build-XP-installer` (L222–314) on `windows-2022`. It depends on `MSBuild_XP_CI_build` **and** `MinGW32_lowend_CI_build` (L227).
- The version is stamped into `contrib/windows/installer/DOSBox-X-setupXP.iss` (L268). All exes are copied to `contrib/windows/installer/WinXP/` and then `./ISCC.exe ./DOSBox-X-setupXP.iss` runs (L276–281).
- The **Inno Setup compiler is committed to the repo** (`SRC/contrib/windows/installer/ISCC.exe`, `ISCmplr.dll`, `ISPP.dll`, `Setup.e32`, `SetupLdr.e32`, `islzma.dll`). Its version resource says "Inno Setup" but carries no number; `deps.lock` notes the output is 5.5.x.
- **The `.iss` script:**
  - `OutputBaseFilename=dosbox-x-winXP-{ver}-setup` (L27), `lzma` solid (L29–30), `ArchitecturesInstallIn64BitMode=x64` (L40), `PrivilegesRequired=lowest` (L41), `MinVersion=5.1` (L46).
  - It bundles **six exes**: VS XP x64/x86 × SDL1/SDL2 **plus MinGW lowend SDL1/SDL2**, together with `inpout32.dll` and `inpoutx64.dll` (L135–148).
  - It picks one exe as `dosbox-x.exe` through `InstallVersion(n)` checks.
- The XP lowend job inside `vsbuild_xp.yml` (L150–221) is the MinGW 7.3.0 recipe from §1.4. It runs `./build-mingw-sdl2` then `./build-mingw` with the lowend toolchain, and the outputs are `dosbox-x_mingw_lowend_SDL{1,2}.exe`.

### 1.3 `mingw32_lowend9x` (Win9x/NT4/2000, SDL1 only)
- Source: `SRC/.github/workflows/mingw32.yml`, job `MinGW32_CI_lowend9x_build` (L275–378), on `windows-latest` with an MSYS2 `MINGW32` shell (`git make mingw-w64-i686-libtool autoconf automake p7zip`, L290–294).
- **Toolchain:** `/mingw32` is deleted and replaced by `https://github.com/crazii/MINGW-toolchains-w9x/releases/download/v1.0.1-w95nt/mingw32.7z`, fetched with `wget` at CI time **without a hash pin** (L318–328). BUILD.md calls it a toolchain "from the original MinGW (not the MinGW-w64 project)" (`SRC/BUILD.md:L135-146`).
- **Script:** `SRC/build-mingw-lowend9x`.
  - Flags: `-D_WIN32_WINDOWS=0x0400 -D_FILE_OFFSET_BITS=32 -mwin32 -march=pentium` (L15).
  - Builds in-tree SDL1, libpng and FreeType, and **opencow** (Unicode→ANSI shim for 9x, L68–74). No SDL_net (no IPHLPAPI on Win95, L26–29).
  - Configure line (L85): `--enable-debug --disable-libfluidsynth --disable-libslirp --disable-d3d9 --disable-d3d-shaders --disable-xbrz --disable-printer --disable-largefile --enable-opencow --disable-sdlnet`.
- Packaged as `dosbox-x-mingw32_lowend9x-<ver>-portable.zip` (L362).
- **Measured:** PE 4.0, GNU ld 2.41. Imports `msvcrt.dll`, `WSOCK32.DLL`, `OPENGL32`, `WINMM` and the other stock DLLs; all present in ReactOS 0x502. Fewest features, no D3D, no networking.

### 1.4 `mingw32` (standard) and `mingw32_lowend` (XP)
**Standard `mingw32`:** `SRC/.github/workflows/mingw32.yml`, job `MinGW32_CI_build` (L10–140).
- **Toolchain:** current MSYS2 `MINGW32`: `mingw-w64-i686-toolchain`, plus libtool, nasm, ncurses and binutils (L25–29).
- libslirp is **rebuilt from `msys2/MINGW-packages`** because MSYS2 is dropping 32-bit packages (L30–42).
- **Scripts:** `./build-mingw` (L64) / `./build-mingw-sdl2` (L94), then `strip -s`.
  - Both build in-tree SDL1 (or SDL2) and SDL_net, libpng and FreeType. zlib comes from MSYS2 (`SRC/build-mingw:L29-69`).
  - Configure: `--enable-debug --disable-avcodec --enable-d3d9 --enable-d3d-shaders --disable-libfluidsynth --prefix=/usr` (+`--enable-sdl2`) (`SRC/build-mingw:L78`, `SRC/build-mingw-sdl2:L87`). With MSYS2's libslirp present, slirp ends up enabled.
- **Not XP/ReactOS-capable (measured):** kernel32 SRW/condition-variable/thread-pool APIs, `GetFileInformationByHandleEx`, `SHGetKnownFolderPath`, `inet_ntop/pton`, and the msvcrt `_s` functions are all missing at 0x502. INSTALL.md agrees that the standard MinGW builds "won't run on Windows XP/Vista" (`SRC/INSTALL.md` §Windows).

**`mingw32_lowend`:** job `MinGW32_CI_lowend_build` (L142–273). It is not in our pins, but it is a release asset.
- `/mingw32` is replaced by the **committed** archive `SRC/build-scripts/mingw/lowend-bin/i686-7.3.0-release-posix-dwarf-rt_v5-rev0+nasm.7z` (47 MB). That's a mingw-builds GCC 7.3.0 with the mingw-w64 v5 runtime, posix threads and DWARF EH (L186–195).
- Then `./build-mingw` / `./build-mingw-sdl2` run. They detect the marker file `${MSYSTEM_PREFIX}/i686-7.3.0-release-posix-dwarf-rt_v5-rev0.txt` and switch PATH and flags, plus a `sed` on `src/hardware/serialport/misc_util.cpp` (`SRC/build-mingw:L15-24`).
- The upstream `build-mingw-lowend*` scripts (`--disable-libfluidsynth --disable-libslirp --enable-d3d9 --enable-d3d-shaders`) are what BUILD.md documents (`SRC/BUILD.md:L121-133`). CI actually uses the plain `build-mingw*` scripts in lowend mode.
- **Measured:** PE 4.0, ld 2.30, both SDL1 and SDL2 clean at 0x502.

### 1.5 `vsbuild-win32`
- Source: `SRC/.github/workflows/vsbuild32.yml`, job `MSBuild32_CI_build`, on `windows-latest` (L15). There's **no toolset sed**, so the project default v142 applies (measured linker 14.29 = VS2019 16.11 toolset).
- `msbuild … Release|Win32` and `Release SDL2|Win32` (L55, L93), each followed by `PatchPE.exe` (L57, L95). Unit tests run with `dosbox-x.exe -tests` (L58–64, L96–102). The zip is `dosbox-x-vsbuild-win32-<ver>-portable.zip` (L132).
- **Measured trap:** the header reads 5.1 because of PatchPE, but the static v142 CRT/STL imports 32 Vista+ kernel32 functions (`InitializeSRWLock`, `*ConditionVariable*`, `CreateThreadpool*`, `GetTickCount64`, `InitOnceExecuteOnce`, `CompareStringEx`, `LCMapStringEx`, `GetLocaleInfoEx`, `CreateSymbolicLinkW`, …).
  - **Fails at 0x502.** All of them resolve at `DLL_EXPORT_VERSION=0x600` (`research/logs/prebuilt-imports-vsbuild-vs-reactos-0x600.txt`).

### 1.6 Other
- `windows-installers.yml` builds the non-XP `dosbox-x-windows-*-setup.exe`: windows-2022, VS x86/x64/ARM with v142/v143, no XP.
- `hxdos.yml` builds the HX-DOS build (not relevant).

---

## 2. Per-asset build matrix

| Asset (2026.10.01) | Workflow / job | Host toolchain | SDL | Notable configure / flags | Static / bundled | PE OS·subsys (measured) | ReactOS 0x502 imports (measured) | Use for VetTV |
|---|---|---|---|---|---|---|---|---|
| `winXP` portable → `dosbox-x_XPx86_SDL2.exe` | `vsbuild_xp.yml` `MSBuild_XP_CI_build` L58–69 | windows-2022, VS2022 + **v141** (14.16), SDK 10.0.22621, `/MT`, `/arch:SSE`, PatchPE | SDL2 2.32.10 (in-tree) | `vs/config.h`: D3D9+shaders, OpenGL, MT32, internal FluidSynth, pcap, xBRZ, debugger; no slirp | static MS CRT, dxguid.lib, SDL2, SDL2_net, zlib, libpng, FreeType, PDCurses, mt32emu, FluidSynth | 5.1 · 5.1 | **clean** (632) | **M1 default** |
| `winXP` portable → `dosbox-x_XPx86_SDL1.exe` | same, L58–63 | same | SDL 1.2.15-mod (in-tree) | same | same (SDL1) | 5.1 · 5.1 | **clean** (558) | **M1 fallback** |
| `winXP` setup.exe | `vsbuild_xp.yml` `build-XP-installer` L222–292 | windows-2022 + committed `ISCC.exe` (Inno 5.5.x) | — | `.iss` MinVersion 5.1, lowest privileges, picks 1 of 6 exes | 6 exes + inpout32/inpoutx64 | n/a | n/a | optional (addons.c `/VERYSILENT`); portable zip simpler |
| `mingw32_lowend` portable (SDL1 + SDL2) | `mingw32.yml` `MinGW32_CI_lowend_build` | MSYS2 shell + committed GCC 7.3.0 / mingw-w64 v5 posix-dwarf | both | `build-mingw*` in lowend mode (debug on, d3d9, no fluidsynth-lib, no slirp) | libstdc++/winpthreads static, OS msvcrt | 4.0 · 4.0 | **clean** | **A/B candidate, no MS CRT code** |
| `mingw32_lowend9x` portable | `mingw32.yml` `MinGW32_CI_lowend9x_build` | MSYS2 shell + crazii w9x MinGW (unpinned wget) | SDL1 only | `-march=pentium`, opencow, no d3d9/sdlnet/xbrz/printer | OS msvcrt, WSOCK32 | 4.0 · 4.0 | clean | last-resort QA only (feature-poor) |
| `mingw32` portable (standard) | `mingw32.yml` `MinGW32_CI_build` | MSYS2 MINGW32 current mingw-w64 GCC | both | `--enable-debug --enable-d3d9 --enable-d3d-shaders --disable-avcodec --disable-libfluidsynth` (+libslirp) | libslirp, OS msvcrt | 4.0 · 4.0 | **fails** (SRW, threadpool, `inet_ntop`, `SHGetKnownFolderPath`, msvcrt `_s`) | **do not use** |
| `vsbuild-win32` portable | `vsbuild32.yml` | windows-latest, VS **v142** (14.29), `/MT`, PatchPE | both | as `vs/config.h` | static v142 CRT | 5.1 · 5.1 (patched) | **fails** (32 Vista+ kernel32) | **do not use** (only with a 0x600 ReactOS build) |
| **ours:** cross `dosbox-x.exe` SDL2 | `research/tools/cross-build-sdl2.sh` | Linux, Debian mingw-w64 12 / GCC 14.2 posix | SDL2 2.32.10 (in-tree) | `--host=i686-w64-mingw32 --enable-sdl2 --disable-debug --disable-avcodec --enable-d3d9 --enable-d3d-shaders --disable-libfluidsynth --disable-libslirp` | libstdc++/libgcc/winpthreads static, in-tree zlib/libpng/FreeType/SDL2/SDL2_net, OS msvcrt | 5.1 · 5.1 | **clean** (790) | **M2+ path** (needs runtime QA) |

---

## 3. What can be reproduced where

### 3.1 Shared Linux box: cross-compile (mingw-w64 i686)
**Status: works.** The SDL2 build was done today; full log in §4.
- `sudo -n true` succeeded (rc 0). Only apt cross-toolchain and autotools packages were installed (listed in §4.1). No system settings were changed.
- **RosBE 2.2.1** (`/workspace/vettv-retro-os/toolchain/RosBE/i386`):
  - What it is: a complete `i686-w64-mingw32` GCC 8.4.0 with mingw-w64 6.0, msvcrt, static libstdc++, `--enable-sjlj-exceptions`, **thread model `win32`**.
  - Test result: `#include <thread>/<mutex>; std::mutex m; std::thread t(...)` fails with "'mutex' in namespace 'std' does not name a type". The Debian posix compiler compiles the same file.
  - Why that matters: DOSBox-X uses `std::thread` whenever `__MINGW64_VERSION_MAJOR` is defined (`SRC/src/dos/cdrom.h:L31-33,L344-347`, `SRC/src/gui/sdl_mapper.cpp:L734-766`) and `<mutex>` unconditionally (`SRC/src/hardware/mic_input_win32.h:L28`).
  - Verdict: **not usable without source patches** or a `mingw-std-threads` shim. It's also busy serving the ReactOS lane's builds. Don't use it for DOSBox-X.
- **Exact commands:**
```bash
# one-time (done 2026-10-02 ~3:15 PM PT; sudo -n works on the box)
sudo -n DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
  g++-mingw-w64-i686-posix gcc-mingw-w64-i686-posix autoconf automake libtool nasm
# build (~10 min wall at -j3 while the ReactOS lane was compiling)
cd /workspace/vettv-retro-os/lanes/dosbox-x/research
nice -n 10 ./tools/cross-build-sdl2.sh "$PWD/src/dosbox-x-dosbox-x-v2026.10.01" "$PWD/build/xsdl2"
# verify
objdump -p build/xsdl2/src/dosbox-x-stripped.exe | grep -E 'Subsystem|DLL Name'
python3 tools/check_imports.py /workspace/vettv-retro-os/src/reactos 0x502 build/xsdl2/src/dosbox-x-stripped.exe
```
- **What the script does** (`research/tools/cross-build-sdl2.sh`):
  1. Copies the pristine tree.
  2. Applies `research/patches/0001-xp-cross-portability.sh`.
  3. Runs `./autogen.sh`.
  4. Builds in-tree **zlib** (`win32/Makefile.gcc PREFIX=i686-w64-mingw32-`, because Debian's mingw-w64 has no zlib), then **SDL2**, **SDL2_net**, **libpng** and **FreeType**, each with `--host=i686-w64-mingw32 --enable-static --disable-shared`. These mirror `SRC/vs/*/build-dosbox.sh`, which don't pass `--host`.
  5. Creates the header shims.
  6. Configures and makes DOSBox-X.
  - **XP flags:** `CFLAGS/CXXFLAGS=-O2 -D_WIN32_WINNT=0x0501 -DWINVER=0x0501 -march=pentium4 -msse2 -mfpmath=sse` and `LDFLAGS=-static -static-libgcc -static-libstdc++ -Wl,--major-os-version,5,--minor-os-version,1,--major-subsystem-version,5,--minor-subsystem-version,1`.
    - configure.ac already adds `-static -static-libgcc -static-libstdc++` for mingw (`SRC/configure.ac:L496-499`).
    - *Recommended tweak for CI:* use `-march=i686 -msse -mfpmath=387` (or `-march=pentium3`). That matches upstream's SSE-only baseline and avoids problems on SSE2-less VM CPU models.
- **SDL1 cross-build:** not done in the time box. The same pattern applies (`vs/sdl` is autotools; add `--host`). The SDL1 tree's own `build-dosbox.sh` toggles `ENABLE_IM_EVENT` and builds twice (`SRC/build-mingw:L27-38`), so mirror that.

### 3.2 GitHub Actions on a future fork (`Vet-TV/dosbox-x`)
*Nothing was forked or pushed. Enabling Actions on a fork needs owner approval.*
- **(a) Recommended: Linux cross job.** Use an `ubuntu-24.04` runner with container `debian:trixie` so the toolchain matches the box exactly.
```yaml
jobs:
  xp-cross-sdl2:
    runs-on: ubuntu-24.04
    container: debian:trixie
    steps:
      - uses: actions/checkout@v4            # at tag dosbox-x-v2026.10.01 (+ our patches dir)
      - run: apt-get update && apt-get install -y --no-install-recommends g++-mingw-w64-i686-posix gcc-mingw-w64-i686-posix autoconf automake libtool nasm make file python3 binutils git ca-certificates
      - run: ./vettv/cross-build-sdl2.sh "$PWD" "$RUNNER_TEMP/b"
      - run: git clone --filter=blob:none --sparse https://github.com/reactos/reactos ros && (cd ros && git sparse-checkout set dll win32ss && git checkout <pinned ReactOS SHA>)
      - run: python3 ./vettv/check_imports.py ros 0x502 "$RUNNER_TEMP/b/src/dosbox-x-stripped.exe" | tee imports.txt && ! grep -q "higher-version: \[" imports.txt
      - uses: actions/upload-artifact@v4
        with: { name: dosbox-x-vettv-xp-sdl2, path: "${{ runner.temp }}/b/src/dosbox-x-stripped.exe" }
```
  The import gate fails the job if any import isn't exported by ReactOS at 0x502.
- **(b) Upstream-identical XP build:** copy `vsbuild_xp.yml`'s `MSBuild_XP_CI_build` and drop the x64 steps.
  - Steps: `windows-2022`, the v141+WinXP component install (L26–39), the toolset sed (L44), `msbuild` (L58–69), `PatchPE.exe` (L63/L69).
  - It works without secrets (`GITHUB_TOKEN` only). It depends on `windows-2022` image availability and the VS installer accepting v141 components, which is a future deprecation risk.
- **(c) Upstream MinGW lowend:** copy `mingw32.yml` `MinGW32_CI_lowend_build` (L142–228). It's fully self-contained thanks to the committed 7.3.0 toolchain archive.

### 3.3 Windows 10 laptop (Asus)
- **(a) WSL2 Debian trixie (easiest):** run the same commands as §3.1. This gives an identical toolchain and output.
- **(b) MSYS2, upstream lowend recipe.** It produces upstream-equivalent `mingw32_lowend` exes with no Microsoft toolchain.
```bash
# MSYS2 "MINGW32" shell
pacman -Syu
pacman -S --needed git make mingw-w64-i686-libtool autoconf automake p7zip
cd /mingw32/.. && rm -rf mingw32 && mkdir mingw32
7z x <dosbox-x>/build-scripts/mingw/lowend-bin/i686-7.3.0-release-posix-dwarf-rt_v5-rev0+nasm.7z -o/mingw32
find /mingw32 -type f -iname '*.exe' -exec chmod +x {} \;
cd <dosbox-x> && export MSYSTEM_PREFIX=/mingw32
./build-mingw-sdl2 && strip -s src/dosbox-x.exe && cp src/dosbox-x.exe dosbox-x_mingw_lowend_SDL2.exe
./build-mingw      && strip -s src/dosbox-x.exe && cp src/dosbox-x.exe dosbox-x_mingw_lowend_SDL1.exe
```
- **(c) Visual Studio (upstream XP route).**
  - Setup: install VS 2022 Community or Build Tools with "MSVC v141 – VS 2017 C++ x64/x86 build tools" and "C++ Windows XP Support for VS 2017 (v141) tools", plus a Windows 10/11 SDK.
  - Build: sed or edit the `vs/*.vcxproj` files listed at `vsbuild_xp.yml:L44` from `v142` to `v141` and set the SDK, then `msbuild vs\dosbox-x.sln -t:dosbox-x:Rebuild -p:Configuration="Release SDL2" -p:Platform=Win32`, then `contrib\windows\installer\PatchPE.exe bin\Win32\"Release SDL2"\dosbox-x.exe`.
  - **Caveats:** this installs the Microsoft toolchain on the laptop. That's fine to *use*, but the output embeds the static MS CRT, and v142/v143 output is **not** NT5-safe (see the vsbuild-win32 measurement). Don't ship anything from this path without the import check.

---

## 4. Cross-build attempt log (box, Fri Oct 2, 2026)
Times are PT. During most of this session the box clock printed CDT (UTC-5; PT = box time − 2h). By 3:58 PM PT it printed PDT directly. All times below are converted to PT.

### 4.1 Packages installed
**3:15–3:17 PM PT.** The first attempt failed with a transient HTTP 500 on `libmagic-mgc`. The retry succeeded.

15 new packages:
- `autoconf 2.72-3.1`, `automake 1:1.17-4`, `autotools-dev`, `libtool 2.5.4-4`
- `binutils-mingw-w64-i686 2.44-3+12+b1`
- `g++-mingw-w64-i686-posix` / `gcc-mingw-w64-i686-posix` / `gcc-mingw-w64-i686-posix-runtime 14.2.0-19+27+b1`, `gcc-mingw-w64-base`
- `mingw-w64-common` / `mingw-w64-i686-dev 12.0.0-5`
- `nasm 2.16.03-1`
- `file`, `libmagic1t64`, `libmagic-mgc`

Toolchain properties: posix thread model, default CRT msvcrt (`__MSVCRT_VERSION__=0x700`, no `_UCRT`). Log: `research/logs/apt-install.log`.

### 4.2 Iterations
**Started 3:17 PM PT; first successful link 3:38 PM PT.** Logs are in `research/logs/xbuild-sdl2*.log`.

| # | Blocker | Fix (all in the build copy; pristine `research/src` untouched) |
|---|---|---|
| 1 | `gamelink.h:11: fatal error: Windows.h: No such file` | The Linux host is case-sensitive. A shim dir `xcase-shim/` holds symlinks `Windows.h, Winsock2.h, Shellapi.h, ShlObj.h, Shlobj.h, Commdlg.h, Shlwapi.h, TCHAR.h, Unknwn.h, D3dx9shader.h` → the lower-case mingw headers. Includes come from both `<…>` and `"…"`, e.g. `SRC/src/gui/sdlmain.cpp:L268 "Commdlg.h"`. |
| 2 | `iconvpp.hpp:226: fatal error: iconv.h` | On WIN32, `SRC/src/dos/dos_programs.cpp:L92-95` defines `C_ICONV` and `#include`s `src/misc/winiconv.c`, but `include/iconvpp.hpp:L224-226` still wants `<iconv.h>`; MSYS2 gets one from libiconv. A declaration-only `iconv.h` shim matches `winiconv.c:L67-71`. |
| 3 | `enet.h:5346: 'inet_ntop' was not declared` | `SRC/include/enet.h:L1040` only uses its own `inet_ntop/pton` fallback for non-mingw-w64. mingw-w64 hides the (Vista+) prototypes at `_WIN32_WINNT<0x600`. **Patch:** the guard becomes `#if !defined(__MINGW64_VERSION_MAJOR) \|\| (_WIN32_WINNT < 0x0600)`. This is upstreamable, and it also explains why upstream's standard mingw32 exe imports `inet_ntop`. |
| 4 | `output_direct3d11.cpp:15: fatal error: ..\ints\int10.h` | Backslash include path. **Patch:** `"../ints/int10.h"` (upstreamable). |
| — | **Linked OK** with `make rc=0` | |
| 5 | Clean rerun 1, 3:41–3:50 PM PT: `ld: cannot find -liconv` | With the shim present at configure time, `configure.ac:L1513-1522` sees `<iconv.h>`, fails the link probe and appends `-liconv`. The iterative build didn't hit this because the shim was added after configure. **Fix (in the script):** run configure with `ac_cv_header_iconv_h=no`. `winiconv.c` is still compiled in via `dos_programs.cpp`. Failed log: `research/logs/xbuild-sdl2.clean-rerun1-FAILED-liconv.log`. |

### 4.3 Result
**Iterative artifact:** `research/artifacts/dosbox-x_xmingw_i686_SDL2_iter.exe`, 22.4 MB stripped, sha256 `412d7f0997ac631e285589495d75dc3f7ae8ed610d9cfcaa58a186220fa49161`.
- PE i386 GUI, OS 5.1 / subsystem 5.1.
- Imports: ADVAPI32 COMDLG32 dsound GDI32 IMM32 IPHLPAPI KERNEL32 msvcrt ole32 OLEAUT32 OPENGL32 SETUPAPI SHELL32 SHLWAPI USER32 VERSION WINMM WINSPOOL WS2_32.
- **790 imports, 0 missing at ReactOS 0x502** (`research/logs/xbuild-sdl2-imports-vs-reactos-0x502.txt`).
- `config.h`: `C_SDL2 C_DIRECT3D C_D3DSHADERS C_OPENGL C_MT32 C_FREETYPE C_LIBPNG C_LIBZ C_SDL2_NET C_PCAP C_IPX C_MODEM C_PRINTER C_GAMELINK C_XBRZ C_DYNAMIC_X86 C_DYNREC C_FPU_X86`. No `C_DEBUG`, `C_SLIRP` or `C_FLUIDSYNTH` (the internal WIN32 synth is still compiled via `midi.cpp:L102`).

**Clean rerun of the final script (reproducibility check): PASSED.**
- Command: `JOBS=6 tools/cross-build-sdl2.sh "$PWD/src/dosbox-x-dosbox-x-v2026.10.01" "$PWD/build/xsdl2-clean"`, run from a fresh copy of the pristine tarball tree, after fix #5.
- Started ~3:51 PM PT; DONE in **310 s** wall (deps 71 s, DOSBox-X make -j6 ~240 s); exit 0. Log: `research/logs/xbuild-sdl2.clean-rerun2.log`.
- **Artifact:** `research/artifacts/dosbox-x_xmingw_i686_SDL2_clean.exe`, 22,364,174 bytes stripped, sha256 `20e5ff3b79059a9640dcf0ef8a4153461d647a00108ad61313d2512cefadfbe2`.
- Same 19 DLLs. OS/subsystem 5.1. **790 imports, 0 missing at 0x502** (`research/logs/xbuild-sdl2-clean-imports-0x502.txt`).
- `config.h` includes C_SDL2, C_DIRECT3D, C_D3DSHADERS, C_OPENGL, C_MT32, C_FREETYPE, C_LIBPNG, C_SDL2_NET and C_XBRZ.
- The byte content differs from the iterative exe. The build isn't bit-reproducible: it embeds timestamps and paths. Add `SOURCE_DATE_EPOCH` and `-ffile-prefix-map` later if we need reproducible builds.
- **Prefer the `_clean` artifact** as the reference output of the script.

**Not yet done:**
- Running the exe. There's no Wine on the box, and a ReactOS VM test is needed.
- SDL1 cross-build.
- XP's own `msvcrt.dll` symbol coverage. mingw-w64 12 may reference msvcrt symbols newer than XP's, so check against an XP export list if real XP matters.

---

## 5. XP / ReactOS compatibility checklist (gate for any exe we ship)
1. **PE header:** `MajorOperatingSystemVersion/MajorSubsystemVersion` ≤ 5.1. Our mingw route sets this with ld flags; upstream VS uses PatchPE.
   - **The header is necessary but not sufficient:** vsbuild-win32 has a 5.1 header and still fails.
2. **Import check:** every import is exported by ReactOS at the shipped `DLL_EXPORT_VERSION` (0x502 by default). Use `research/tools/check_imports.py <reactos-src> 0x502 <exe>` and wire it into CI. Avoid known Vista+ APIs: SRW locks, condition variables, `InitOnce*`, thread pool, `GetTickCount64`, `*Ex` locale APIs, `GetFileInformationByHandleEx`, `SHGetKnownFolderPath`, `inet_ntop/pton`, `CreateSymbolicLinkW`.
3. **No Microsoft runtime DLLs:** no `VCRUNTIME140*.dll`, `MSVCP140*.dll`, `ucrtbase.dll` or `api-ms-win-crt-*`.
   - The OS `msvcrt.dll` is fine; it's a ReactOS (Wine-derived) system DLL.
   - Never ship `d3dx9_43.dll`, `xinput*.dll` or DirectX redistributables. ReactOS master already has Wine `d3dx9_24…43`, `d3d9`, `xinput1_1/1_2/1_3/9_1_0`, and `ucrtbase` (`src/reactos/dll/directx/wine/`, `dll/win32/`).
4. **No toolchain DLLs:** no `libstdc++-6.dll`, `libgcc_s_*.dll` or `libwinpthread-1.dll`. Link statically (configure does this for mingw).
5. **CPU baseline:** upstream VS XP requires SSE (`/arch:SSE`, BUILD.md L88–89). Pick `-march=i686 -msse` or `pentium3` for broad VM/real-hardware coverage.
6. **XInput:** loaded dynamically only. No static import, and the stale INSTALL.md note about `XInput9_1_0.dll` (`SRC/INSTALL.md:L43`) doesn't apply. ReactOS's Wine `xinput1_3` is picked first.
7. **Exclude `inpout32.dll`/`inpoutx64.dll`** (separate closed licence; parallel-port passthrough only) and the x64 exes.
8. **Runtime smoke test on ReactOS i386** (QEMU TCG / VirtualBox): start-up; `output=surface`; Sound Blaster audio on AC97 and HDA; `mount`; Alt+Enter round-trip (see CORE-15658); exit code back to vetvdm.

---

## 6. ReactOS-specific findings (upstream issues, backends, audio, SDL1 vs SDL2)
- **Upstream history:**
  - #1269 (2019, closed, by joncampbell123): "ReactOS won't run DOSBox-X, even the MinGW builds" (2019-10-01). He suspected unknown version numbers or missing import symbols, which is exactly what the import check now catches. https://github.com/joncampbell123/dosbox-x/issues/1269
  - #3577 (open): Torinde, 2022-06-16: "ReactOS 32-bit: DOSBox-X works, but sound very low" (0.84.0). Torinde, 2023-11-21, on **ReactOS 0.4.14, 2023.10.06**: "MinGW-lowend, XP, 9x – **works**, sound is not low … HX – doesn't work … Vista+ – doesn't work". The same tester found ReactOS 64-bit 0.4.15-dev "OS unstable" on 2023-11-30. crazii, 2023-11-19, for missing sound on old Windows: SDL tries dsound → waveout, so "try `set SDL_AUDIODRIVER=waveout`". https://github.com/joncampbell123/dosbox-x/issues/3577
  - PR #4623 (merged 2023-11-27): website `_includes/version.html` "XP build works under ReactOS". PR #4627 (merged 2023-11-27): adds "Those work in ReactOS as well, but support is considered experimental" to INSTALL.md.
  - `gh search` for "reactos" in issues and PRs (2026-10-02) found **nothing newer than #3577's 2026-08/09 comments**, and those are about NT3.x / Win32s ports, not ReactOS.
- **In-tree ReactOS mentions:**
  - `SRC/src/misc/ethernet_slirp.cpp:L570-572`: peeking a closed socket breaks the next `recv()` on ReactOS (JIRA CORE-17425). This only matters for slirp builds, which the XP builds don't have.
  - `SRC/contrib/windows/installer/readme.txt:L17`: "May work on ReactOS but support is considered experimental."
- **ReactOS-side DOSBox (0.74) bugs to expect** (same SDL1/windib/dsound paths):
  - CORE-15658: no drawing after a fullscreen → windowed switch.
  - CORE-16130: taskbar stays visible in fullscreen.
  - CORE-13490: portcls audio hiccups in demanding games.
  - CORE-9981: scrambled audio, fixed in 0.4.14/0.4.15 by commit 57516ed.
  - The 0.4.16 HDA bus rewrite (sklhdaudbus + KMDF) is new and untested with DOSBox-X.
- **Output backend:**
  - Defaults: VS/XP builds default to `direct3d` when `output=default` (`SRC/src/output/output_tools.cpp:L75-98`). MinGW SDL1 builds without D3D default to `opengl`, or `surface` in VirtualBox. The packaged `dosbox-x.conf` forces `output=ttf` (`vsbuild_xp.yml:L102`).
  - On ReactOS, D3D9 is WineD3D → OpenGL → (on the Asus) Mesa software: two translation layers.
  - **Recommendation for vetvdm's generated conf:** `output=surface` by default (SDL windib/GDI blit, the fewest layers). `opengl`/`openglnb` is an opt-in per profile for VMs with working GL. Avoid `direct3d` until QA proves it. Use `ttf` only for text-mode apps.
  - Always write `output=` explicitly, so the packaged `output=ttf` / `default` never applies.
- **Audio:**
  - The SDL2 build **forces `SDL_AUDIODRIVER=directsound`** unless the variable is already set (`SRC/src/gui/sdlmain.cpp:L9510-9515`). SDL1 tries dsound, then waveout (`SRC/vs/sdl/src/audio/SDL_audio.c:L70-74`). ReactOS's dsound is the Wine implementation over winmm/wdmaud.
  - **vetvdm knob:** set `SDL_AUDIODRIVER=winmm` (SDL2) or `waveout` (SDL1) per profile if dsound crackles. Tune `[mixer] blocksize/prebuffer`.
- **SDL1 vs SDL2:**
  - Upstream's default is SDL1, with a heavily modified in-tree 1.2.15 that runs emulation independently of the window (`SRC/BUILD.md:L37-51`). SDL1 forces `SDL_VIDEODRIVER=windib`; SDL2 forces `windows` (`SRC/src/gui/sdlmain.cpp:L9496-9505`).
  - SDL2 gives better international keyboard handling (INSTALL.md) and gamelink.
  - Torinde's 0.4.14 test passed with the XP pack and with lowend (both SDL variants exist there). No source reports SDL1-vs-SDL2 differences on ReactOS.
  - **Keep both:** SDL2 is the default; SDL1 is the per-profile fallback.
- **XInput:** dynamic loading, so a missing DLL just disables gamepads. ReactOS ships Wine `xinput1_3` and `xinput9_1_0`. No need to ship anything.

---

## 7. Licence notes on what ends up in the binary (input for M1-7)
| Component (in pinned tree) | Licence (verified in tree) | Notes |
|---|---|---|
| DOSBox-X core | GPL-2.0-or-later headers, COPYING = GPLv2 | — |
| **xBRZ** `SRC/src/libs/xBRZ/xbrz.h:L1-15` | **GPL-3.0** (+ linking exception for MAME, FreeFileSync, Snes9x, ePSXe) | Enabled in VS (`vs/config.h C_XBRZ`) and default autotools builds. **This makes the shipped DOSBox-X binary distributable only under GPLv3 terms.** It's still mere aggregation on our ISO, so ReactOS isn't affected, but it contradicts BRIEF §2.4 rule (3) in spirit. Our own build can use `--disable-xbrz` for a pure GPLv2-or-later binary. Decide in M1-7. |
| SDL 1.2.15 (modified) `SRC/vs/sdl/COPYING` | LGPL-2.1 | Statically linked into a GPL program: fine. The source offer covers it (it's in the tarball). |
| SDL2 2.32.10, SDL_net, SDL2_net | zlib | — |
| zlib 1.3.1 / libpng 1.6.47 | zlib / PNG Reference Library License v2 | Keep notices. |
| FreeType 2.13.1 `SRC/vs/freetype/LICENSE.TXT` | FTL **or** GPLv2 (dual) | Choose GPLv2 or keep the FTL credit. |
| FluidSynth 1.1.6-noglib (internal) | LGPL-2.0-or-later (`fluid_synth.c` header) | Statically linked; source in the tarball. |
| mt32emu 2.8.0 | LGPL-2.1-or-later | MT-32 ROMs are **not** included and must never be shipped. |
| PDCurses | public domain (core) | Debugger builds only. |
| physfs, tinyfiledialogs | zlib | — |
| libchdr, Opus/Ogg/SpeexDSP | BSD-3-style | Keep notices. |
| dr_libs / stb_vorbis | public domain / MIT-0 | — |
| Fonts: `Nouveau_IBM.ttf`, `SarasaGothicFixed.ttf`, `wqy_1?pt.bdf`, `FREECG98.BMP` | various (Sarasa = OFL-1.1; WenQuanYi = GPLv2 w/ font exception) | Ship only what the conf uses. Audit in M1-7. |
| `inpout32.dll` / `inpoutx64.dll` | closed binary, no licence file in tree | **Exclude.** |
| Upstream VS XP exe: static **MSVC CRT** (v141 `/MT`) + `dxguid.lib` (DirectX SDK) + d3dx9 headers in `vs/` | Microsoft licences | Embedded MS code, not a redistributable DLL. GPL compatibility rests on the "system library" exception. Flag for M1-7. **The mingw route avoids it entirely.** |
| Our mingw build: libstdc++/libgcc (static) | GPLv3 + GCC Runtime Library Exception | Fine for any licence. |
| winpthreads, mingw-w64 CRT startup | MIT/ZPL-style | Keep notices. |
| Committed build tools (`ISCC.exe` Inno Setup, `PatchPE.exe`, `vs/tool/zip.exe`, `7za.exe`, lowend 7.3.0 toolchain `.7z`) | various | Not shipped, but they're inside the source tarball we redistribute as the GPL source offer. They're fine to redistribute (Inno Setup licence is permissive; mingw-builds is GPL with source elsewhere). Note them in the source bundle README. |

---

## 8. Risks
| Risk | Impact | Mitigation |
|---|---|---|
| Our cross-built exe hasn't been run yet (no Wine/ReactOS VM on the box) | High until tested | M1 ships the upstream prebuilt. Run QEMU TCG/VirtualBox smoke tests (§5 step 8) before switching. |
| GCC 14 / mingw-w64 12 output may use msvcrt symbols missing on **real XP** (ReactOS is fine per spec) | Low (XP isn't the product) | Add an XP msvcrt export list to the import gate if XP matters. Or build with the upstream 7.3.0 lowend toolchain via MSYS2/Actions. |
| Upstream VS route depends on `windows-2022` plus a VS installer still offering v141/WinXP components, and on the opaque `PatchPE.exe` | Med | Prefer the Linux cross route. If VS is needed, replace PatchPE with our own `pehdr-lite` build using explicit `-osver 5.1 -subsysver 5.1`, or a Python `pefile` step. |
| `mingw32_lowend9x` toolchain fetched unpinned (`wget` of crazii's release) | Low (we don't use it) | Pin by sha256 if ever used. |
| xBRZ GPLv3 in upstream binaries | Med (licence clarity) | M1-7 decision. `--disable-xbrz` in our build. |
| Our two portability patches + header shims drift on future tags | Low | Keep them in `patches/` and try to upstream them (owner approval required for any upstream PR). |
| SSE2 baseline in today's artifact (`-march=pentium4`) | Low | Use `-march=i686 -msse` in CI. |
| ReactOS DirectX import (PR #9352, Wine 10) changes D3D/GL behaviour | Med | Default to `output=surface`; per-profile opt-in for GL/D3D. |
| Box contention (the ReactOS lane uses RosBE and ~11 GB RAM) | Low | Build at `nice -n 10 -j3`. It took about 10 min. |

---

## 9. Sources
- Pinned tree (tag `dosbox-x-v2026.10.01`): workflows `.github/workflows/{vsbuild_xp,vsbuild32,mingw32,windows-installers}.yml`; `BUILD.md`, `INSTALL.md`, `build-mingw*`, `vs/dosbox-x.vcxproj`, `vs/config.h`, `vs/pehdr-lite/*`, `contrib/windows/installer/DOSBox-X-setupXP.iss`, `src/output/output_tools.cpp`, `src/gui/sdlmain.cpp`, `src/gui/midi.cpp`, `include/enet.h`, `src/misc/ethernet_slirp.cpp`, `vs/sdl2/src/core/windows/SDL_xinput.c`. Online: https://github.com/joncampbell123/dosbox-x/tree/dosbox-x-v2026.10.01 (e.g. https://github.com/joncampbell123/dosbox-x/blob/dosbox-x-v2026.10.01/.github/workflows/vsbuild_xp.yml)
- Release assets and digests: https://github.com/joncampbell123/dosbox-x/releases/tag/dosbox-x-v2026.10.01 (via `gh release view` / `gh api`)
- Issues/PRs (via `gh`, read-only): #1269 https://github.com/joncampbell123/dosbox-x/issues/1269 · #3577 https://github.com/joncampbell123/dosbox-x/issues/3577 (comment 1821801793 = ReactOS 0.4.14 test) · PR #4623 https://github.com/joncampbell123/dosbox-x/pull/4623 · PR #4627 https://github.com/joncampbell123/dosbox-x/pull/4627
- ReactOS JIRA (archives): CORE-15658 https://reactos.org/archives/public/ros-bugs/2019-January/160436.html · CORE-9981 https://reactos.org/archives/public/ros-bugs/2022-November/217344.html · CORE-17277 https://jira.reactos.org/browse/CORE-17277 · ReactOS 0.4.16 release notes (HDA) https://reactos.org/project-news/reactos-0416-released/ · Tests for 0.4.15 (DOSBox 0.74.2 row) https://reactos.org/wiki/Tests_for_0.4.15
- ReactOS export specs used by the import check: `/workspace/vettv-retro-os/src/reactos/{dll,win32ss}/**/*.spec` (commit 8a5b9300c1)
- Debian mingw-w64 packages (trixie): `gcc-mingw-w64-i686-posix 14.2.0-19+27+b1`, `mingw-w64-i686-dev 12.0.0-5`

### Research artifacts (box, not in git)
- `research/src/`: extracted tarball. `research/build/xsdl2/`: iterative build tree. `research/build/xsdl2-clean/`: clean rerun.
- `research/prebuilt/`: unzipped upstream assets, including `mingw32` and `mingw32_lowend` zips downloaded for analysis.
- `research/tools/check_imports.py`, `research/tools/cross-build-sdl2.sh`, `research/patches/0001-xp-cross-portability.sh`.
- `research/logs/`: apt log, build logs, import reports.
- `research/artifacts/dosbox-x_xmingw_i686_SDL2_iter.exe` (iterative) and `research/artifacts/dosbox-x_xmingw_i686_SDL2_clean.exe` (clean scripted rerun, sha256 `20e5ff3b…fbe2`).
