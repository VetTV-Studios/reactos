#!/usr/bin/env bash
# VetTV 3g: remove QEMU-only device instances from the preinstalled image's SYSTEM hive (offline, hivex).
#   cleanup-qemu-enum.sh <retroos-disk.img> <logfile>
# Deletes ONLY Enum keys of QEMU's emulated hardware plus their Control\DeviceClasses interface links,
# in every ControlSet00N. Real hardware is enumerated fresh on first boot. SYSTEM.ALT is left as-is.
set -euo pipefail
IMG="${1:?img}"; LOG="${2:?log}"
command -v hivexsh >/dev/null || { echo "FATAL: hivexsh missing (apt-get install libhivex-bin)" >&2; exit 2; }
command -v mcopy  >/dev/null || { echo "FATAL: mtools missing" >&2; exit 2; }
export MTOOLS_SKIP_CHECK=1
S=$(/usr/sbin/sfdisk -d "$IMG" | grep -m1 -o 'start= *[0-9]*' | tr -dc 0-9); [ -n "$S" ] || { echo "FATAL: no partition" >&2; exit 2; }
P="$IMG@@$((S*512))"; H="::/ReactOS/System32/config/SYSTEM"
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT
mcopy -n -o -i "$P" "$H" "$W/SYSTEM"
# Enum paths (bus\device-id prefix match). QEMU i440FX/PIIX, AC97, stdvga, PCnet, QEMU IDE disk/CD, QEMU USB tablet, stdvga monitor.
PAT='^(PCI\\VEN_8086&DEV_(1237|7000|7010|7020|7111|7113|2415)&|PCI\\VEN_1234&DEV_1111&|PCI\\VEN_1022&DEV_2000&|ACPI\\QEMU0002$|IDE\\(Disk|CdRom)QEMU_|USB\\Vid_0627&Pid_0001$|HID\\Vid_0627&Pid_0001$|DISPLAY\\RHT1234$)'
DCPAT='^##\?#(PCI#VEN_8086&DEV_(1237|7000|7010|7020|7111|7113|2415)&|PCI#VEN_1234&DEV_1111&|PCI#VEN_1022&DEV_2000&|ACPI#QEMU0002#|IDE#(Disk|CdRom)QEMU_|USB#Vid_0627&Pid_0001#|HID#Vid_0627&Pid_0001#|DISPLAY#RHT1234#)'
ls_() { printf 'cd %s\nls\n' "$1" | hivexsh "$W/SYSTEM" 2>/dev/null || true; }
: > "$W/cmds"; : > "$LOG"; n=0
for cs in $(ls_ '\' | grep -E '^ControlSet[0-9]{3}$'); do
  for bus in $(ls_ "\\$cs\\Enum"); do
    while IFS= read -r dev; do [ -n "$dev" ] || continue
      if printf '%s\\%s\n' "$bus" "$dev" | grep -qiE "$PAT"; then
        printf 'cd \\%s\\Enum\\%s\\%s\ndel\n' "$cs" "$bus" "$dev" >> "$W/cmds"; echo "DEL $cs\\Enum\\$bus\\$dev" >> "$LOG"; n=$((n+1)); fi
    done < <(ls_ "\\$cs\\Enum\\$bus")
  done
  while IFS= read -r g; do [ -n "$g" ] || continue
    while IFS= read -r l; do [ -n "$l" ] || continue
      if printf '%s\n' "$l" | grep -qiE "$DCPAT"; then
        printf 'cd \\%s\\Control\\DeviceClasses\\%s\\%s\ndel\n' "$cs" "$g" "$l" >> "$W/cmds"; echo "DEL $cs\\Control\\DeviceClasses\\$g\\$l" >> "$LOG"; n=$((n+1)); fi
    done < <(ls_ "\\$cs\\Control\\DeviceClasses\\$g")
  done < <(ls_ "\\$cs\\Control\\DeviceClasses")
done
[ "$n" -gt 0 ] || { echo "FATAL: no QEMU keys found - unexpected" >&2; exit 3; }
echo commit >> "$W/cmds"
hivexsh -w "$W/SYSTEM" < "$W/cmds" >/dev/null
# verify: nothing matching remains
for cs in $(ls_ '\' | grep -E '^ControlSet[0-9]{3}$'); do for bus in $(ls_ "\\$cs\\Enum"); do
  ls_ "\\$cs\\Enum\\$bus" | sed "s|^|$bus\\\\|" | grep -iE "$PAT" && { echo "FATAL: key survived" >&2; exit 4; }; done; done || true
mcopy -n -o -i "$P" "$W/SYSTEM" "$H"
echo "removed $n keys (log: $LOG)"
