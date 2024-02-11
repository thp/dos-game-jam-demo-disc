# Quickly scan for non-8.3 filenames + other undesireable filenames
# 2023-08-19 Thomas Perl <m@thp.io>

import os
import itertools
import string

def valid_char(ch):
    return ch in (string.digits + string.ascii_lowercase + '_')

BLOCKLISTED_FILENAMES = ('cwsdpmi.swp',)

def check_filename(basename):
    basename = basename.lower()

    if basename in BLOCKLISTED_FILENAMES:
        # Temporary/swap/... file
        return False

    if '.' in basename:
        base, ext = basename.split('.', 1)
        if '.' in ext:
            # Multiple dots in filename
            return False
    else:
        base, ext = basename, ''

    if len(base) > 8:
        # Basename longer than 8 characters
        return False

    if len(ext) > 3:
        # Extension longer than 3 characters
        return False

    if not all(valid_char(ch) for ch in (base + ext)):
        # Invalid characters
        return False

    return True




for basedir in ('DEMO2023', 'pcx', 'extras'):
    for dirname, filenames, dirnames in os.walk(basedir):
        for filename in itertools.chain(filenames, dirnames):
            if not check_filename(filename):
                print('/'.join((dirname, filename)))
