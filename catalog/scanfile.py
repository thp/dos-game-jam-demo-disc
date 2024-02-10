# Quickly scan for non-8.3 filenames + other undesireable filenames
# 2023-08-19 Thomas Perl <m@thp.io>

import os
import itertools

for basedir in ('DEMO2023', 'pcx', 'extras'):
    for dirname, filenames, dirnames in os.walk(basedir):
        for filename in itertools.chain(filenames, dirnames):
            if filename.lower() == 'cwsdpmi.swp':
                print('/'.join((dirname, filename)))
            if '.' in filename:
                base, ext = filename.split('.', 1)
                if len(base) > 8 or len(ext) > 3:
                    print('/'.join((dirname, filename)))
            if '-' in filename:
                print('/'.join((dirname, filename)))
