# Debian 13 head-to-head: DOS side (DOSBox-X lane)

Configs, the launcher and docs only. No game files, no binaries.

- DOSBox-X: upstream tag `dosbox-x-v2026.10.01` (commit b6abbd5980a885f5f310a4088c59a8688d1b116c),
  built natively in a Debian 13 trixie amd64 chroot on the box with `./configure --enable-sdl2 --prefix=/opt/dosbox-x`.
  Binary sha256 8714fd42b6abb043adfeb9661e6672d2106afe38a498798f453fa1f8661a787a (source tarball df023a6c…4ed4). Note: `-version`/log show "Commit f1c4414": that string comes from the include/build_timestamp.h shipped in the upstream tarball, not from the tag commit.
- `vetvdm-h2h.sh GAME.EXE [args]`: by-hash/<sha256>.conf, then by-name/<UPPER NAME>.conf
  (by-name is skipped if its `# size=` line doesn't match). Log: `~/.local/state/vettv-h2h/launch/<ts>-<pid>/launch.log`,
  with `launch profile=<id> (<source>)` first, then the DOSBox-X stdout and stderr.
- `base.conf` is loaded first, then the profile. Every profile sets mpu401=none (no GM synth on the image), so music is OPL/SB.

## Install into a QA overlay (guest, as player)
Copy `/opt/dosbox-x` (tarball from lanes/dosbox-x/runs/h2h/dosbox-x-2026.10.01-trixie-amd64.tar.gz)
and this folder into the guest. One way is a 9p share, or `virt-copy-in` with the VM off. Then:
    sudo tar -C / -xzf dosbox-x-2026.10.01-trixie-amd64.tar.gz
    sudo mkdir -p /opt/vettv-h2h && sudo cp -r base.conf profiles vetvdm-h2h.sh /opt/vettv-h2h/
    /opt/vettv-h2h/vetvdm-h2h.sh ~/games/jazz/JAZZ.EXE
Runtime libs: libsdl2-2.0-0, libpng16-16t64, libfreetype6, libasound2t64, libpcap0.8t64, libxkbfile1, libncurses6.
Check with `ldd /opt/dosbox-x/bin/dosbox-x`.
- Display: both sides run windowed (Lead ruling 2026-10-28). Debian base.conf: fullscreen=false, autolock=false, output=surface. The ReactOS profiles set fullscreen=false and [sdl] autolock=false over vetvdm's defaults.
