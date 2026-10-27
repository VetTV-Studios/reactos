#!/usr/bin/env bash
# qmp.sh <socket> <command-json...>  : send QMP commands (after qmp_capabilities)
# helpers: qmp.sh <sock> shot <file.png> | key <qcode>[,<qcode>...] | type "<text>" | quit
S="$1"; shift
send() { { printf '%s\n' '{"execute":"qmp_capabilities"}'; for c in "$@"; do printf '%s\n' "$c"; sleep 0.15; done; sleep 0.3; } | socat -t2 - UNIX-CONNECT:"$S" >/dev/null 2>&1; }
keyjson() { local IFS=,; local ks=""; for k in $1; do ks="$ks{\"type\":\"qcode\",\"data\":\"$k\"},"; done; echo "{\"execute\":\"send-key\",\"arguments\":{\"keys\":[${ks%,}]}}"; }
case "$1" in
  shot) send "{\"execute\":\"screendump\",\"arguments\":{\"filename\":\"$(realpath -m "$2")\",\"format\":\"png\"}}";;
  key)  shift; cmds=(); for k in "$@"; do cmds+=("$(keyjson "$k")"); done; send "${cmds[@]}";;
  type) cmds=(); t="$2"; for ((i=0;i<${#t};i++)); do c="${t:i:1}"; case "$c" in
          [a-z0-9]) q="$c";; [A-Z]) q="shift,${c,,}";; " ") q=spc;; "-") q=minus;; "\\") q=backslash;;
          ":") q="shift,semicolon";; ".") q=dot;; "/") q=slash;; "=") q=equal;; *) continue;; esac
          cmds+=("$(keyjson "$q")"); done; send "${cmds[@]}";;
  quit) send '{"execute":"quit"}';;
  *) send "$@";;
esac
