#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Host build of vetvdm_test.exe (unit tests), run from this directory.
# Usage: ./build-test-host.sh [output.exe]   (CC defaults to i686-w64-mingw32-gcc)
#
# Same rules as build-host.sh: link with -s and never add .gnu_debuglink.
# objcopy puts that section at VMA 0 and ReactOS refuses the image
# ("not a valid Win32 application", PR #28 QA). The check below fails the
# build if any debug section survives.
set -eu
CC=${CC:-i686-w64-mingw32-gcc}
OBJDUMP=${OBJDUMP:-i686-w64-mingw32-objdump}
OUT=${1:-vetvdm_test.exe}
"$CC" -std=gnu99 -Wall -Wextra -Werror -O2 \
  -D_WIN32_WINNT=0x0502 -DWINVER=0x502 -DUNICODE -D_UNICODE \
  -mconsole -municode \
  -I. -Icore -Ivetvdm -Ithird_party \
  -o "$OUT" \
  core/tests/test_vetvdm.c core/tests/test_support.c \
  core/tests/test_cmdline.c core/tests/test_core.c \
  vetvdm/cmdline.c vetvdm/cmdline_norm.c vetvdm/cmdline_quote.c \
  vetvdm/image.c vetvdm/image_io.c vetvdm/route.c \
  core/buf.c core/utf8.c core/sha256.c core/quote.c \
  core/policy.c core/profile.c core/profile_json.c \
  core/conf.c core/conf_emit.c core/dryrun.c core/log.c \
  -luser32 -lkernel32 \
  -s -Wl,--image-base,0x400000 -Wl,--no-insert-timestamp
if "$OBJDUMP" -h "$OUT" | grep -qi 'debug'; then
  echo "build-test-host.sh: debug section in $OUT" >&2
  exit 1
fi
