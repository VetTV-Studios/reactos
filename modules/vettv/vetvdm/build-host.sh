#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Host build of vetvdm.exe (RosBE or mingw-w64 i686), run from this directory.
# Usage: ./build-host.sh [output.exe]   (CC defaults to i686-w64-mingw32-gcc)
#
# Link with -s and never add a .gnu_debuglink afterwards: objcopy places that
# section at VMA 0, below the image base, and ReactOS refuses the image with
# error 193 (seen on b2, fixed in b3). Keep .text as section 0 (objdump -h).
# --no-insert-timestamp keeps the output byte-identical across rebuilds.
set -eu
CC=${CC:-i686-w64-mingw32-gcc}
OUT=${1:-vetvdm.exe}
"$CC" -std=gnu99 -Wall -Wextra -Werror -O2 \
  -D_WIN32_WINNT=0x0502 -DWINVER=0x502 -DUNICODE -D_UNICODE \
  -mconsole -municode \
  -I. -Icore -Ivetvdm -Ithird_party \
  -o "$OUT" \
  vetvdm/main.c vetvdm/hostutil.c vetvdm/host_route.c \
  vetvdm/host_dosbox.c vetvdm/host_doscmd.c vetvdm/host_wine.c \
  vetvdm/host_ntvdm.c vetvdm/host_log.c vetvdm/host_fresh.c \
  vetvdm/cmdline.c vetvdm/cmdline_norm.c vetvdm/cmdline_quote.c \
  vetvdm/image.c vetvdm/route.c vetvdm/ask.c vetvdm/image_io.c \
  core/buf.c core/utf8.c core/sha256.c core/quote.c \
  core/policy.c core/profile.c core/profile_json.c \
  core/conf.c core/conf_emit.c core/dryrun.c core/log.c \
  core/policy_reg.c core/launch.c core/tempdir.c vetvdm_paths.c \
  -lshell32 -luser32 -ladvapi32 -lkernel32 \
  -s -Wl,--image-base,0x400000 -Wl,--no-insert-timestamp
