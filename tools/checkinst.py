#!/usr/bin/env python3
"""checkinst.py SCRIPT... - static checks of Amiga Installer scripts, so they run on Commodore's
Installer 43, InstallerLG and AROS's Installer: balanced parentheses and strings, no (procedure ...),
no (select ...) used for values (AROS returns the index), no (and ...) in conditions (nest ifs),
and every relative (source "...") going through (tackon #src ...) so the script finds its files
when Workbench starts Installer in another directory. Exit status 1 on any problem.
SPDX-License-Identifier: GPL-2.0-only"""
import re, sys

def check(path):
    bad = []
    depth = 0
    for n, line in enumerate(open(path, encoding='latin-1'), 1):
        if line.lstrip().startswith(';'):
            continue
        code = re.sub(r'"(\\.|[^"\\])*"', '""', line)
        if code.count('"') % 2:
            bad.append('%d: unbalanced string' % n)
        code = code.split(';')[0]
        depth += code.count('(') - code.count(')')
        if depth < 0:
            bad.append('%d: more ) than (' % n)
            depth = 0
        for word, why in (('(procedure', 'procedure is not understood by AROS\'s Installer'),
                          ('(select', 'select returns the index on AROS: use (set) and (if (= #x n))'),
                          ('(and ', '(and ...) is not logical on AROS: nest (if ...)')):
            if word in code:
                bad.append('%d: %s' % (n, why))
        if re.search(r'\(source "[^":]+"\)', line):
            bad.append('%d: relative source without (tackon #src ...)' % n)
    if depth:
        bad.append('end: %d unclosed (' % depth)
    for b in bad:
        print('%s:%s' % (path, b))
    return not bad

ok = all([check(p) for p in sys.argv[1:]])
print('checkinst: %s' % ('ok' if ok else 'problems found'))
sys.exit(0 if ok else 1)
