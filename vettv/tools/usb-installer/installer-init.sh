#!/bin/sh
# VetTV Retro OS USB installer: /init of the stick's initramfs (busybox only, no network, no root fs).
# Finds the stick (FAT label VETTVUSB), checks the payload hash, picks the internal disk, and writes
# retroos-disk.img.gz to it ONLY after the user types "ERASE <disk>" exactly. Then reads it back.
export PATH=/bin
/bin/busybox --install -s /bin
mount -t proc proc /proc; mount -t sysfs sys /sys; mount -t devtmpfs dev /dev
exec </dev/tty1 >/dev/tty1 2>&1
log() { echo "$*"; echo "[vettv] $*" >/dev/ttyS0 2>/dev/null; }
halt_msg() { log "$*"; log "Nothing more will happen. Hold the power button to turn the laptop off."; while :; do sleep 3600; done; }
clear; log "=== VetTV Retro OS USB installer ==="
for m in $(cat /etc/modules.list); do modprobe "$m" 2>/dev/null; done
log "Looking for the USB stick..."
STICK=""; for i in $(seq 1 30); do STICK=$(findfs LABEL=VETTVUSB 2>/dev/null) && [ -n "$STICK" ] && break; sleep 1; done
[ -n "$STICK" ] || halt_msg "ERROR: could not find the stick (label VETTVUSB). Try another USB port."
STICKDISK=$(basename "$(readlink -f /sys/class/block/$(basename $STICK)/..)")
mkdir -p /stick; mount -t vfat -o ro "$STICK" /stick 2>/tmp/m.err || halt_msg "ERROR: cannot mount $STICK: $(cat /tmp/m.err)"
log "Stick: $STICK (disk $STICKDISK)"
log "Checking the RetroOS image on the stick (about a minute)..."
WANT=$(cut -d' ' -f1 /stick/retroos-disk.img.gz.sha256)
GOT=$(sha256sum /stick/retroos-disk.img.gz | cut -d' ' -f1)
[ "$WANT" = "$GOT" ] || halt_msg "ERROR: the image on the stick is damaged (sha256 mismatch). Rewrite the stick."
IMGBYTES=$(cat /stick/retroos-disk.img.size)
log "Image OK. It needs $((IMGBYTES/1048576)) MiB on the internal drive."
CANDS=""
for b in /sys/block/*; do n=$(basename $b)
  case $n in loop*|ram*|sr*|fd*|zram*) continue;; esac
  [ "$n" = "$STICKDISK" ] && continue
  [ "$(cat $b/removable)" = 1 ] && continue
  sz=$(( $(cat $b/size) * 512 )); [ $sz -ge $IMGBYTES ] || continue
  CANDS="$CANDS $n"; done
echo; log "Internal drives found:"
for n in $CANDS; do log "  $n  $(( $(cat /sys/block/$n/size) / 2097152 )) GiB  $(cat /sys/block/$n/device/model 2>/dev/null)"; done
[ -n "$CANDS" ] || halt_msg "ERROR: no internal drive found. Check the M.2 drive (SATA mode AHCI in the BIOS)."
set -- $CANDS; T=$1
echo; log "!!! Everything on /dev/$T will be erased and replaced with VetTV Retro OS. !!!"
[ $# -gt 1 ] && log "More than one drive was found. Type the name of the one to erase instead of $T."
while :; do
  printf "Type  ERASE %s  and press Enter to install (or just Enter to cancel): " "$T"
  read -r A B
  [ -z "$A" ] && halt_msg "Cancelled. Nothing was written."
  if [ "$A" = ERASE ] && [ -n "$B" ] && echo " $CANDS " | grep -q " $B "; then T=$B; break; fi
  log "That did not match. Nothing was written."
done
log "Writing to /dev/$T ... (a few minutes; do not unplug anything)"
zcat /stick/retroos-disk.img.gz | dd of=/dev/$T bs=4M conv=fsync 2>/tmp/dd.err || halt_msg "ERROR: write failed: $(cat /tmp/dd.err)"
sync; echo 3 > /proc/sys/vm/drop_caches; blockdev --flushbufs /dev/$T 2>/dev/null
log "Verifying what was written..."
WANT2=$(cat /stick/retroos-disk.img.sha256 | cut -d' ' -f1)
GOT2=$(head -c "$IMGBYTES" /dev/$T | sha256sum | cut -d' ' -f1)
[ "$WANT2" = "$GOT2" ] || halt_msg "ERROR: verify failed (got $GOT2). Do not boot the drive; ask Build & Kernel."
log "VETTV-INSTALL-OK /dev/$T $GOT2"
umount /stick; echo
log "Done. Unplug the USB stick, then press Enter to power off."
read -r X; poweroff -f
