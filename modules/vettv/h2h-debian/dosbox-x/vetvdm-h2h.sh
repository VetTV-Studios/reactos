#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# VetTV Retro OS - Debian 13 head-to-head DOS launcher.
# Mirrors vetvdm's profile lookup: by-hash/<sha256>.conf first, then
# by-name/<UPPERCASE EXE>.conf (only if its "# size=" line matches, when set).
# Writes the profile line, then DOSBox-X stdout+stderr, to a per-launch log.
#
# usage: vetvdm-h2h.sh /path/to/GAME.EXE [args...]
set -u
EXE=${1:?usage: vetvdm-h2h.sh GAME.EXE [args]}; shift
ROOT=${VETTV_H2H_ROOT:-/opt/vettv-h2h}
DOSBOX=${VETTV_DOSBOX:-/opt/dosbox-x/bin/dosbox-x}
PROF=$ROOT/profiles
STATE=${XDG_STATE_HOME:-$HOME/.local/state}/vettv-h2h/launch

[ -f "$EXE" ] || { echo "no such file: $EXE" >&2; exit 2; }
DIR=$(cd "$(dirname "$EXE")" && pwd)
BASE=$(basename "$EXE")
UP=$(printf '%s' "$BASE" | tr '[:lower:]' '[:upper:]')
SHA=$(sha256sum "$EXE" | cut -d' ' -f1)
SIZE=$(stat -c %s "$EXE")

conf=; src=builtin; id=-
if [ -f "$PROF/by-hash/$SHA.conf" ]; then
    conf=$PROF/by-hash/$SHA.conf; src=by-hash
elif [ -f "$PROF/by-name/$UP.conf" ]; then
    want=$(sed -n 's/^# size=\([0-9]*\).*/\1/p' "$PROF/by-name/$UP.conf" | head -n1)
    if [ -z "$want" ] || [ "$want" = "$SIZE" ]; then
        conf=$PROF/by-name/$UP.conf; src=by-name
    fi
fi
[ -n "$conf" ] && id=$(sed -n 's/^# id=\(.*\)/\1/p' "$conf" | head -n1)

RUN=$STATE/$(date +%Y%m%d-%H%M%S)-$$
mkdir -p "$RUN"
LOG=$RUN/launch.log
{
    echo "$(date '+%F %T %Z') program=\"$EXE\" sha256=$SHA size=$SIZE"
    echo "launch profile=${id:--} ($src)"
    [ -n "$conf" ] && echo "conf=$conf"
} >> "$LOG"
sync "$LOG" 2>/dev/null || sync

set -- -conf "$ROOT/base.conf" ${conf:+-conf "$conf"} \
    -c "mount c \"$DIR\"" -c "c:" -c "$BASE $*" -c "exit"
echo "cmd: $DOSBOX $*" >> "$LOG"
# DOSBox-X writes straight to the log file (no pipe), so a kill loses at
# most what DOSBox-X itself had not yet written.
"$DOSBOX" "$@" >> "$LOG" 2>&1
rc=$?
echo "exit=$rc" >> "$LOG"
sync "$LOG" 2>/dev/null || sync
exit $rc
