#!/usr/bin/env bash
# VetTV Retro OS USB installer, part (b): bootable installer stick image (raw .img, BIOS/CSM boot).
#   build-installer-stick.sh <diskdir> <out.img>
# <diskdir> = output of build-retroos-disk.sh (retroos-disk.img + .img.gz).
# Layout: MBR (SYSLINUX mbr.bin), p1 FAT32 label VETTVUSB, active:
#   /syslinux.cfg /ldlinux.* /vmlinuz (Alpine linux-lts, GPL-2.0) /initrd (busybox + modules + installer-init.sh)
#   /retroos-disk.img.gz + .sha256 files. No /dev access: built with mkfs.vfat/mtools/syslinux --offset.
set -euo pipefail
export PATH=$PATH:/usr/sbin:/sbin
HERE="$(cd "$(dirname "$0")" && pwd)"
DISK="$(realpath "${1:?usage: build-installer-stick.sh <diskdir> <out.img>}")"; OUTIMG="$(realpath -m "${2:?out.img}")"
CACHE="${CACHE:-/workspace/bk-usb-cache}"; AV=3.24.2; KV=6.18.52-0-lts
NB=alpine-netboot-$AV-x86_64.tar.gz; NB_SHA=bbdd643360fac384a7933d8bebc7132eb720002e58d1321edcde4a5a3c3b83c6
mkdir -p "$CACHE"; cd "$CACHE"
[ -f $NB ] || curl -fsSO https://dl-cdn.alpinelinux.org/alpine/v${AV%.*}/releases/x86_64/$NB
echo "$NB_SHA  $NB" | sha256sum -c -
tar xzf $NB boot/vmlinuz-lts boot/modloop-lts
[ -d ml ] || unsquashfs -q -d ml boot/modloop-lts >/dev/null
W="$(mktemp -d)"; trap 'rm -rf "$W"' EXIT
R=$W/root; mkdir -p $R/{bin,dev,proc,sys,stick,tmp,etc} $R/lib/modules/$KV
cp /usr/bin/busybox $R/bin/busybox    # Debian busybox-static (GPL-2.0)
ln -s busybox $R/bin/sh
cp "$HERE/installer-init.sh" $R/init; chmod +x $R/init
MODS="ahci ata_piix ata_generic sd_mod usb-storage uas xhci-pci ehci-pci ohci-pci uhci-hcd usbhid hid-generic vfat nls_cp437 nls_iso8859-1 nls_utf8"
M=ml/modules/$KV; : > $W/need
add() { local p; p=$(grep -E "(^|/)$1\.ko[^:]*:" $M/modules.dep | head -1) || { echo "missing module $1" >&2; exit 1; }
  grep -qxF "${p%%:*}" $W/need && return; echo "${p%%:*}" >> $W/need
  for d in ${p#*:}; do add "$(basename "${d%%.ko*}")"; done; }
for m in $MODS; do add "$(echo $m | sed 's/-/[-_]/g')"; done
while read -r f; do mkdir -p "$R/lib/modules/$KV/$(dirname $f)"
  case $f in *.gz) gzip -dc $M/$f > $R/lib/modules/$KV/${f%.gz};; *.zst) zstd -qdc $M/$f > $R/lib/modules/$KV/${f%.zst};; *) cp $M/$f $R/lib/modules/$KV/$f;; esac
done < $W/need
cp $M/modules.builtin $M/modules.order $R/lib/modules/$KV/ 2>/dev/null || true
/usr/bin/busybox depmod -b $R $KV
echo "$MODS" | tr ' ' '\n' | tr '-' '_' > $R/etc/modules.list
( cd $R && find . | cpio -o -H newc --quiet | gzip -9 ) > $W/initrd
cat > $W/syslinux.cfg <<CFG
DEFAULT install
PROMPT 0
TIMEOUT 0
SAY VetTV Retro OS USB installer - starting...
LABEL install
  KERNEL /vmlinuz
  INITRD /initrd
  APPEND rdinit=/init console=ttyS0 console=tty1 quiet loglevel=3 modprobe.blacklist=nouveau
CFG
GZ=$DISK/retroos-disk.img.gz
sha256sum "$GZ" | sed 's#  .*#  retroos-disk.img.gz#' > $W/gz.sha
sha256sum "$DISK/retroos-disk.img" | sed 's#  .*#  retroos-disk.img#' > $W/img.sha
stat -c %s "$DISK/retroos-disk.img" > $W/img.size
NEED=$(( ( $(stat -c %s $GZ) + $(stat -c %s $CACHE/boot/vmlinuz-lts) + $(stat -c %s $W/initrd) ) / 1048576 + 64 ))
PSZ=$(( NEED < 300 ? 300 : NEED ))   # MiB; FAT32 wants >= ~260 MiB of clusters
OFF=1048576
rm -f "$W/p1"; truncate -s ${PSZ}M $W/p1; mkfs.vfat -F 32 -n VETTVUSB -h 2048 $W/p1 >/dev/null
export MTOOLS_SKIP_CHECK=1
mcopy -i $W/p1 $CACHE/boot/vmlinuz-lts ::/vmlinuz; mcopy -i $W/p1 $W/initrd ::/initrd; mcopy -i $W/p1 $W/syslinux.cfg ::/syslinux.cfg
mcopy -i $W/p1 "$GZ" ::/retroos-disk.img.gz; mcopy -i $W/p1 $W/gz.sha ::/retroos-disk.img.gz.sha256
mcopy -i $W/p1 $W/img.sha ::/retroos-disk.img.sha256; mcopy -i $W/p1 $W/img.size ::/retroos-disk.img.size
mkdir -p "$(dirname "$OUTIMG")"; rm -f "$OUTIMG"; truncate -s $(( PSZ + 1 ))M "$OUTIMG"
printf 'label: dos\nstart=2048, size=%d, type=c, bootable\n' $(( PSZ*2048 )) | sfdisk -q "$OUTIMG"
dd if=$W/p1 of="$OUTIMG" bs=1M seek=1 conv=notrunc status=none
dd if=/usr/lib/syslinux/mbr/mbr.bin of="$OUTIMG" bs=440 count=1 conv=notrunc status=none
syslinux --install --offset $OFF "$OUTIMG"
mdir -i "$OUTIMG@@$OFF" ::/
ls -l "$OUTIMG"; sha256sum "$OUTIMG"
