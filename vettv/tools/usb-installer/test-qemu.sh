#!/usr/bin/env bash
# End-to-end QEMU test of the installer stick (one VM at a time, names bk-usb-*).
#   test-qemu.sh <stick.img> <ehci|xhci> <outdir>
# 1. boot the stick (throw-away overlay) as usb-storage on the given controller, SeaBIOS legacy boot,
#    with a blank 8 GiB disk on AHCI as the "internal" drive (the Asus M.2 SATA is AHCI);
#    type "ERASE <disk>" through QMP, wait for VETTV-INSTALL-OK.
# 2. boot the target disk alone (no stick) and screenshot the RetroOS desktop. Tried on AHCI first, then IDE.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
STICK="$(realpath "${1:?usage}")"; BUS="${2:?ehci|xhci}"; OUT="$(realpath -m "${3:?outdir}")"
mkdir -p "$OUT"; Q="$HERE/qmp.sh"
qemu-img create -f qcow2 -F raw -b "$STICK" "$OUT/stick.qcow2" >/dev/null
qemu-img create -f qcow2 "$OUT/target.qcow2" 8G >/dev/null
case $BUS in ehci) HC="-device usb-ehci,id=hc";; xhci) HC="-device qemu-xhci,id=hc";; esac
S=$OUT/qmp-inst.sock; : > "$OUT/serial-inst.log"
qemu-system-x86_64 -name bk-usb-inst-$BUS -machine pc -accel tcg,thread=multi -m 1024 -rtc base=localtime -display none -vga std \
  -device ahci,id=ahci -drive file="$OUT/target.qcow2",if=none,id=tgt,format=qcow2 -device ide-hd,drive=tgt,bus=ahci.0 \
  $HC -drive file="$OUT/stick.qcow2",if=none,id=stk,format=qcow2 -device usb-storage,bus=hc.0,drive=stk,bootindex=0 \
  -serial file:"$OUT/serial-inst.log" -qmp unix:$S,server,nowait -no-reboot & QP=$!
w() { local pat=$1 t=$2; for ((i=0;i<t;i++)); do grep -q "$pat" "$OUT/serial-inst.log" && return 0; kill -0 $QP 2>/dev/null || return 1; sleep 1; done; return 1; }
if ! w '!!! Everything' 900; then $Q $S shot "$OUT/inst-fail.png" || true; $Q $S quit || true; echo "FAIL: no prompt"; exit 1; fi
T=$(grep -o '/dev/[a-z0-9]* will be erased' "$OUT/serial-inst.log" | head -1 | cut -d' ' -f1 | cut -d/ -f3)
sleep 2; $Q $S shot "$OUT/inst-prompt.png"
$Q $S type "ERASE $T"; $Q $S key ret
if w 'VETTV-INSTALL-OK' 1800; then R=PASS; else R=FAIL; fi
$Q $S shot "$OUT/inst-done.png" || true; $Q $S quit || true; wait $QP || true
echo "install on $BUS -> $T: $R"; [ $R = PASS ] || exit 1
rm -f "$OUT/stick.qcow2"
for tb in ahci ide; do
  S=$OUT/qmp-boot-$tb.sock
  if [ $tb = ahci ]; then DEV=(-device ahci,id=ahci -drive file="$OUT/target.qcow2",if=none,id=tgt,format=qcow2 -device ide-hd,drive=tgt,bus=ahci.0,bootindex=0)
  else DEV=(-drive file="$OUT/target.qcow2",if=ide,index=0,format=qcow2); fi
  qemu-system-i386 -name bk-usb-boot-$BUS-$tb -machine pc -accel tcg,thread=multi -cpu pentium3 -m 2048 -display none -vga std \
    -rtc base=localtime -usb -device usb-tablet "${DEV[@]}" -serial file:"$OUT/serial-boot-$tb.log" -qmp unix:$S,server,nowait & QP=$!
  sleep "${DESKTOP_WAIT:-480}"; $Q $S shot "$OUT/target-desktop-$tb.png" || true; $Q $S quit || true; wait $QP || true
  echo "target boot $tb: see $OUT/target-desktop-$tb.png"
  [ "${BOTH:-0}" = 1 ] || break
done
