#!/usr/bin/env python3
# Generate vetvdm JSON profiles (ReactOS side) from the Debian h2h .conf
# profiles, so the sound/cycles/memory settings are identical. Only the
# file format and paths differ. Settings that come from base.conf on Debian
# (the whole [dosbox]/[cpu]/[sblaster]/[midi] part of base.conf) are written out
# explicitly so they don't depend on vetvdm's built-in defaults.
import json, os, re, sys
here = os.path.dirname(os.path.abspath(__file__))
src = os.path.join(here, '..', 'dosbox-x', 'profiles')
out = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, 'profiles')
TOP = {('dosbox','machine'):'machine', ('dosbox','memsize'):'memsize',
       ('cpu','cycles'):'cycles', ('sblaster','sbtype'):'sbtype',
       ('midi','mpu401'):'mpu401'}
def parse(p):
    meta, sec, kv = {}, None, {}
    for ln in open(p):
        ln = ln.rstrip('\n')
        m = re.match(r'# (id|size)=(.*)', ln)
        if m: meta[m.group(1)] = m.group(2); continue
        if ln.startswith('#'):
            meta.setdefault('notes', ln[2:]); continue
        m = re.match(r'\[(\w+)\]', ln)
        if m: sec = m.group(1); continue
        if '=' in ln: k, v = ln.split('=', 1); kv[(sec, k.strip())] = v.strip()
    return meta, kv
def build(meta, kv, name, sha):
    full = {('dosbox','machine'):'svga_s3', ('dosbox','memsize'):'16',
            ('cpu','cycles'):'auto', ('sblaster','sbtype'):'sb16',
            ('midi','mpu401'):'none', ('cpu','core'):'auto', ('cpu','cputype'):'auto',
            ('sblaster','sbbase'):'220', ('sblaster','irq'):'7', ('sblaster','dma'):'1',
            ('sblaster','hdma'):'5', ('sblaster','oplmode'):'auto'}
    full.update(kv)
    dos, extra = {}, {}
    for (s, k), v in sorted(full.items()):
        if (s, k) in TOP: dos[TOP[(s, k)]] = v
        else: extra.setdefault(s, {})[k] = v
    if extra: dos['extra'] = extra
    match = {}
    if sha: match['sha256'] = sha
    match['name'] = name
    if 'size' in meta: match['size'] = int(meta['size'])
    return {'id': meta['id'], 'match': match, 'layer': 'dosboxx', 'dosbox': dos,
            'notes': meta.get('notes', '') + ' (generated from h2h-debian/dosbox-x by gen.py)'}
byhash = {}
for f in os.listdir(os.path.join(src, 'by-hash')):
    m, kv = parse(os.path.join(src, 'by-hash', f)); byhash[m['id']] = f[:-5]
for d in ('by-hash', 'by-name'): os.makedirs(os.path.join(out, d), exist_ok=True)
for f in sorted(os.listdir(os.path.join(src, 'by-name'))):
    meta, kv = parse(os.path.join(src, 'by-name', f))
    name = f[:-5]; sha = byhash.get(meta['id'])
    doc = json.dumps(build(meta, kv, name, sha), indent=2) + '\n'
    open(os.path.join(out, 'by-name', name + '.json'), 'w').write(doc)
    if sha: open(os.path.join(out, 'by-hash', sha + '.json'), 'w').write(doc)
