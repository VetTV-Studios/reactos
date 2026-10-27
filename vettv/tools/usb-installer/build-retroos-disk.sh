#!/usr/bin/env bash
# VetTV Retro OS USB installer, part (a): preinstalled raw disk image.
#   build-retroos-disk.sh <input.iso> <outdir> [size_mib]
# 1. rebuild the input ISO from its build tree (mkisofs + .lst) with \i386\unattend.inf replaced
# 2. stage 1: unattended text setup onto a blank raw disk (IDE, QEMU TCG, -no-reboot)
# 3. stage 2: first boot from disk, GUI setup runs and reboots (QEMU exits)
# 4. stage 3: one more boot to the desktop -> screenshot (proves the image boots on its own)
# 5. remove QEMU-only Enum keys from SYSTEM (cleanup-qemu-enum.sh, hivex)
# Output: <outdir>/retroos-disk.img (raw, MBR, one active FAT32 partition) + .img.gz + sha256.
# Never touches /dev; VM name bk-usb-disk-*. Default 4000 MiB keeps the raw image < 4 GiB (FAT32 file limit).
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ISO_IN="$(realpath "${1:?usage: build-retroos-disk.sh <input.iso> <outdir> [size_mib]}")"
OUT="$(realpath -m "${2:?outdir}")"; SIZE="${3:-4000}"
mkdir -p "$OUT/log"; ISO="$OUT/unattend.iso"; IMG="$OUT/retroos-disk.img"
rm -f "$ISO"
# Rebuild the ISO with the build tree's own mkisofs and .lst (only \i386\unattend.inf swapped).
# xorriso remastering rewrote ISO9660:1999 names and freeldr then failed with "Could not load SYSTEM.ALT hive".
B="$(dirname "$ISO_IN")"; LST="$B/boot/bootcd.Debug.lst"
[ -f "$LST" ] || { echo "need the build dir next to the ISO ($LST)"; exit 1; }
for f in $(find "$B/boot" -newer "$ISO_IN" -name '*.lst'); do echo "WARN: $f is newer than the ISO"; done
sed "s#^i386/unattend.inf=.*#i386/unattend.inf=$HERE/unattend.inf#" "$LST" > "$OUT/log/bootcd.lst"
mkdir -p "$B/boot/empty"   # empty dir the .lst uses for blank folders (lost if the tree was copied)
grep -c "^i386/unattend.inf=$HERE/unattend.inf" "$OUT/log/bootcd.lst"
( cd "$B/boot" && "$B/host-tools/bin/mkisofs" -quiet -o "$ISO" -iso-level 4 -publisher "ReactOS Project" -preparer "ReactOS Project" \
   -volid ReactOS -volset ReactOS -eltorito-platform x86 -eltorito-boot loader/isoboot.bin -no-emul-boot -boot-load-size 4 \
   -eltorito-alt-boot -eltorito-platform efi -eltorito-boot loader/efisys.bin -no-emul-boot -hide boot.catalog \
   -sort "$B/boot/bootfiles.sort" -duplicates-once -no-cache-inodes -graft-points -path-list "$OUT/log/bootcd.lst" ) 2>"$OUT/log/mkisofs.log"
sha256sum "$ISO_IN" "$ISO" | tee "$OUT/log/iso.sha256"
COMMON=(-machine pc -accel tcg,thread=multi -cpu pentium3 -smp 1 -m 2048
        -audiodev none,id=snd0 -device AC97,audiodev=snd0 -usb -device usb-tablet
        -vga std -display none -nic user,model=pcnet -rtc base=localtime)
run() { local name=$1 tmo=$2; shift 2; local t0=$(date +%s) rc=0
  echo "[$(date +%T)] $name start"
  timeout "$tmo" qemu-system-i386 -name "bk-usb-disk-$name" "${COMMON[@]}" "$@" \
     -serial file:"$OUT/log/serial-$name.log" -qmp unix:"$OUT/log/qmp-$name.sock",server,nowait || rc=$?
  echo "[$(date +%T)] $name exit rc=$rc after $(( $(date +%s)-t0 ))s"; return $rc; }
rm -f "$IMG"; qemu-img create -f raw "$IMG" "${SIZE}M" >/dev/null
run stage1 3600 -no-reboot -drive file="$IMG",if=ide,index=0,format=raw -drive file="$ISO",if=ide,index=2,media=cdrom -boot d
run stage2 3600 -no-reboot -drive file="$IMG",if=ide,index=0,format=raw -boot c
# stage 3: desktop check on a throw-away overlay, so the image stays exactly as setup left it
qemu-img create -f qcow2 -F raw -b "$IMG" "$OUT/log/stage3.qcow2" >/dev/null
run stage3 900 -drive file="$OUT/log/stage3.qcow2",if=ide,index=0,format=qcow2 -boot c &
Q=$!; sleep "${DESKTOP_WAIT:-420}"
"$HERE/qmp.sh" "$OUT/log/qmp-stage3.sock" shot "$OUT/desktop-ide.png" || true
"$HERE/qmp.sh" "$OUT/log/qmp-stage3.sock" quit || true; wait $Q || true
rm -f "$OUT/log/stage3.qcow2"
# VetTV 3g: drop QEMU-only Enum keys from the shipped SYSTEM hive (after the last QEMU stage, before gzip/hash).
"$HERE/cleanup-qemu-enum.sh" "$IMG" "$OUT/qemu-enum-removed.log"
/usr/sbin/sfdisk -d "$IMG" > "$OUT/retroos-disk.sfdisk"
gzip -6 -k -f "$IMG"
( cd "$OUT" && sha256sum retroos-disk.img retroos-disk.img.gz > SHA256SUMS ); cat "$OUT/SHA256SUMS"
