/*
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * PROJECT:  VetTV Retro OS
 * FILE:     modules/vettv/vetvdm/vetvdm/host.h
 * PURPOSE:  Shared host state for vetvdm.exe.
 *
 * Copyright (C) 2026 VetTV
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef VETVDM_HOST_H
#define VETVDM_HOST_H

#include "cmdline.h"
#include "vetvdm_paths.h"

#define VETVDM_CMDLINE_CCH 32768u

typedef struct host
{
    vetvdm_cmdline cmd;
    vcore_policy policy;
    vcore_profile profile;
    vetvdm_path_roots roots;
    vcore_expand_ctx expand;
    WCHAR system_root[MAX_PATH];
    WCHAR program_files[MAX_PATH];
    WCHAR image[MAX_PATH];
    WCHAR cwd[MAX_PATH];
    WCHAR dosbox[MAX_PATH];
    WCHAR wine[MAX_PATH];
    WCHAR win31_root[MAX_PATH];
    WCHAR wincom[64];
    WCHAR temp[MAX_PATH];
    WCHAR log_path[MAX_PATH];
    char sha[65];
    char warn[192];
    char wine_from[16];
    const char *why;
    DWORD file_size;
    DWORD started;
    DWORD log_level;
    int have_roots;
    int cls;
    int kind;
    int layer;
    int forced;
    int dosbox_ok;
    int wine_ok;
    int win31_ok;
    int have_temp;
    int skip_log;
    int dry_ran;
    int fresh_wine;      /* VETVDM_FRESH_WINEVDM / vetvdm.ini FreshWineVDM */
    int wow_ini;         /* otvdm.ini SeparateWOWVDM: 1 separate, 0 shared, -1 n/a */
    long otvdm_pre;      /* otvdm*.exe already running at launch, -1 unknown */
    DWORD otvdm_reaped;  /* own leftover otvdm processes ended after the wait */
    unsigned char *header;
} host;

void warn_add(char *buf, unsigned cap, const char *text);
int file_exists(const WCHAR *path);
int dir_exists(const WCHAR *path);
void file_part(const WCHAR *path, WCHAR *out, DWORD cch);
void parent_dir(const WCHAR *path, WCHAR *out, DWORD cch);
int join_path(WCHAR *out, DWORD cch, const WCHAR *dir, const WCHAR *tail);
int replace_file(const WCHAR *full, const WCHAR *name, WCHAR *out, DWORD cch);
DWORD full_path(const WCHAR *in, WCHAR *out, DWORD cch);
int console_attach(int alloc_ok);
void console_write(const char *text);
void show_error(int from_hook, DWORD code, const char *why);
const char *why_for(DWORD code, const char *why);
int oem_ok(const WCHAR *text);
int wappend(WCHAR *dst, DWORD cap, DWORD *used, const WCHAR *src);

DWORD load_policy(host *st);
DWORD resolve_image(host *st);
DWORD route_image(host *st);
void locate_layers(host *st);

DWORD build_conf(host *st, int win31, vcore_buf *conf,
                WCHAR *conf_path, DWORD cch_path);
DWORD write_conf(const WCHAR *path, const char *text);
DWORD read_errorlevel(const WCHAR *dir, DWORD *code);

WCHAR *host_command_dosbox(const host *st, const WCHAR *conf_path);
WCHAR *host_command_wine(const host *st);
WCHAR *host_command_ntvdm(const host *st);

int fresh_wine_enabled(void);
void fresh_wine_prepare(host *st);
struct vcore_job;
void fresh_wine_cleanup(host *st, struct vcore_job *job, DWORD child_pid);

void write_log(host *st, DWORD code);
void write_log_early(host *st, const char *line);

#endif /* VETVDM_HOST_H */
