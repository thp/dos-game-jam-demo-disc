import collections
import struct
import argparse
import csv
import os
import glob

#
# DOS Game Jam Demo Disc Game Catalog Format
# 2023-08-09 Thomas Perl <m@thp.io>
#

PREFIX_STRINGS = {
    'GAME': 'DEMO2023/',
    'TOOL': 'EXTRAS/',
    'DRV': 'EXTRAS/',
    'SDK': 'SDKS/',
}

GAMES_WITHOUT_README = (
    'piadrane', 'fuelv02', 'gusano', 'tetrik',
    'dt3k', 'dsweeper', 'tdroid', 'cats10',
    'transorb', 'starlite', 'zoz', 'farm1985',
    '587sqnj', 'ow', 'allegro', 'cgacave',
    'vianav21', 'perils', 'aching', 'spmanbob',
    'termtris', 'cgarobot', 'lyfe', 'molten',
    'waterway', 'cube', 'priv5', 'cgastein',
    'batmio', 'cara', 'dialer', 'dosrocks',
    'freeroam', 'blazer', 'astro',
)

KNOWN_README_FILE_NAMES = (
    'readme.txt', 'readme.md', 'readme', 'about.txt',
    'catalog.exe', 'readme12.txt', 'orbit.txt', 'adlipt.txt',
    'unisound.txt', 'fatuma.txt', 'ossuary.txt', 'vanish.txt',
    'aboutgfs.txt', 'cutlass.txt',
    'file_id.diz',
)

parser = argparse.ArgumentParser(description='Convert CSV game list to datafile')
parser.add_argument('--verbose', action='store_true', help='Be verbose')
parser.add_argument('infile', type=str, help='Filename of CSV file')
parser.add_argument('outfile', type=str, help='Filename of DAT file')
args = parser.parse_args()

values = collections.defaultdict(set)
games = []

if not args.verbose:
    print = lambda *args: ...

def translate_toolchain(toolchain):
    # Group all OpenWatcom entries ("", " 16-bit", " 32-bit")
    if toolchain.startswith('OpenWatcom'):
        return 'OpenWatcom'
    if any(asm in toolchain.lower() for asm in ('nasm', 'fasm', 'tasm')):
        return 'Assembly'

    return toolchain

def get_path_case_insensitive(path):
    """Quick and dirty case-insensitive path resolver.

    The path must be relative to the current working directory. Each
    component is resolved by looking for all possible filename candidates
    and picking the first one that matches. In case of multiple file name
    matches (on a case sensitive filesystem, that's possible), any valid
    match is allowed to be returned.
    """
    result = []
    parts = path.split(os.sep)
    for part in parts:
        query = os.sep.join(result + ['*'])
        matches = [os.path.basename(x) for x in glob.glob(query)]
        candidate = next((x for x in matches if x.lower() == part.lower()), None)
        if candidate is None:
            return None
        result.append(candidate)
    return os.sep.join(result)

with open(args.infile) as fp:
    reader = csv.DictReader(fp)
    for parts in reader:
        print(parts)
        d = dict(parts)
        assert d['Kind'] in ('GAME', 'TOOL', 'DRV', 'SDK')
        d['Kilobytes'] = int(d['Kilobytes'] or -1)
        d['Visible'] = (d['Visible'] == 'Y')
        d['Keyboard'] = (d['Keyboard'] == 'Y')
        d['Multiplayer'] = (d['Multiplayer'] == 'Y')
        d['Keyboard'] = (d['Keyboard'] == 'Y')
        d['EndScreen'] = (d['EndScreen'] == 'Y')
        d['Open Source'] = (d['Open Source'] == 'Y')
        d['IncompatibleDOSBox'] = (d['IncompatibleDOSBox'] == 'Y')
        d['Video'] = tuple(d['Video'].split(', '))
        d['Sound'] = tuple(x for x in d['Sound'].split(', ') if x != '-')
        d['Author'] = tuple(d['Author'].split(', '))
        d['Toolchain'] = tuple(translate_toolchain(x) for x in d['Toolchain'].split(', '))
        d['CPU'] = d['Bits'] + '-bit'
        del d['Bits']
        # Normalize paths to forward slash
        d['Run'] = d['Run'].replace('\\', '/')
        d['NumScreenshots'] = int(d['NumScreenshots'])

        if d['Genre'] in ('Tetris', 'Match-3'):
            d['Genre'] = 'Tetris and Match-3'

        if not d['Visible']:
            print('Skip:', d['Name'])
            continue

        d['Readme'] = ''

        if d['Run']:
            pfx = PREFIX_STRINGS[d['Kind']]
            prefix = pfx.rstrip('/')
            gamepath = get_path_case_insensitive(os.path.join(prefix, os.path.dirname(d['Run'].split(' ', 1)[0])))
            assert gamepath is not None and os.path.exists(gamepath), gamepath
            for candidate in KNOWN_README_FILE_NAMES:
                filename = get_path_case_insensitive(os.path.join(gamepath, candidate))
                if filename is None:
                    continue
                if os.path.exists(filename):
                    d['Readme'] = filename
                    break

            if d['ID'] == 'mouse':
                d['Readme'] = get_path_case_insensitive(gamepath.replace('bin', 'doc/ctmouse/ctmouse.txt'))

            if d['ID'] not in GAMES_WITHOUT_README and not d['Readme']:
                raise ValueError(d)

            if d['Readme']:
                # Strip the prefix, as we only used it to resolve the relative path,
                # but at runtime we can just add the prefix again, which we store
                assert d['Readme'].lower().startswith(pfx.lower()), (d['Readme'], pfx)
                d['Readme'] = d['Readme'][len(pfx):]

        print(d)
        games.append(d)
        if d['Kind'] == 'GAME':
            for k, v in d.items():
                values[k].add(v)


import pprint
#pprint.pprint(values)
#pprint.pprint(games)
#pprint.pprint(values.keys())

groupings = [
    ('Genre', 'Browse by genre'),
    ('Jam', 'Browse by game jams'),
    ('Type', 'Browse by release type'),
    ('Author', 'Browse by authors'),

    ('Video', 'Browse by video modes'),
    ('Sound', 'Browse by sound card'),
    ('CPU', 'Browse by CPU type'),

    ('Toolchain', 'Browse by toolchains/SDKs'),

    ('Mouse', 'Games with mouse support'),
    ('Joystick', 'Games with joystick support'),

    ('Multiplayer', 'Multiplayer games'),
    ('Open Source', 'Open source games'),
]

def split_value(group, values):
    if group in ('Multiplayer', 'Open Source'):
        return [True]
    elif group in ('Video', 'Sound', 'Author', 'Toolchain'):
        return set(x for value in values for x in value)
    else:
        return values


# empty string has string index zero for convenience
strings = ['']

def get_string_index(s):
    global strings
    if s not in strings:
        strings.append(s)
    return strings.index(s)

# Place all prefixes early in the string list,
# so the prefix_idx always fits into an 8-bit value
for value in PREFIX_STRINGS.values():
    get_string_index(value)

def make_group(title, children, subgroups):
    return {'title': get_string_index(title), 'children': children, 'subgroups': subgroups}

root_groups = []

JAMS_ORDERING = [
    'CGA Jam 2017',
    'OG DOS Game Jam',
    'Jam #2 (Spring 2020)',
    'MS DOS Game Jam #3',
    'Fall 2020 Jam',
    'Spring 2021',
    'EOY Jam 2021',
    'August 2022 Jam',
    'EOY Jam 2022',
    'June 2023',
    'DOS COM Game Jam',
    'EOY Jam 2023',
    'Other',
]

VIDEO_ORDERING = [
    'Text',
    'CGA',
    'Hercules',
    'Tandy',
    'EGA',
    'VGA',
    'VBE1',
    'VBE2',
]

SOUND_ORDERING = [
    'PC Speaker',
    'Tandy',
    'CMS',
    'Adlib',
    'Sound Blaster',
    'MIDI',
    'GUS',
    'CDDA',
]

def special_sort(group, items):
    if group == 'Jam':
        return sorted(items, key=JAMS_ORDERING.index)
    elif group == 'Video':
        return sorted(items, key=VIDEO_ORDERING.index)
    elif group == 'Sound':
        return sorted(items, key=SOUND_ORDERING.index)
    elif group == 'Author':
        return sorted(items, key=lambda x: x.lower())
    return sorted(items)


def sort_games_list(games_idx):
    return sorted(games_idx, key=lambda idx: games[idx]['Name'].lower())

for group, label in groupings:
    children = []
    subgroups = []
    for value in special_sort(group, split_value(group, values[group])):
        if value in ('', '?', '-'):
            continue

        print(group, '/', value)
        game_indices = []
        for game_idx, game in enumerate(games):
            if game['Kind'] == 'GAME' and (value == game[group] or
                                           (isinstance(game[group], tuple) and value in game[group])):
                print('\t', game['Name'])
                game_indices.append(game_idx)

        if value == True or group == 'Joystick':
            # direct child stuff
            children.extend(game_indices)
        else:
            # submenu
            if group == 'CPU':
                if value == '16-bit':
                    value = '16-bit CPUs (8088+)'
                else:
                    value = '32-bit CPUs (386+)'
            if group == 'Video':
                if value == 'Text':
                    value = 'Text Mode'
            subgroup = make_group(value, sort_games_list(game_indices), [])
            subgroups.append(subgroup)

    root_groups.append(make_group(label, sort_games_list(children), subgroups))

all_games_idx = sort_games_list([idx for idx in range(len(games)) if games[idx]['Kind'] == 'GAME'])
root_groups.insert(0, make_group('All games (alphabetically)', all_games_idx, []))

tools_idx = sort_games_list([idx for idx in range(len(games)) if games[idx]['Kind'] == 'TOOL'])
sdks_idx = sort_games_list([idx for idx in range(len(games)) if games[idx]['Kind'] == 'SDK'])
drivers_idx = sort_games_list([idx for idx in range(len(games)) if games[idx]['Kind'] == 'DRV'])

grouping = make_group('DGJ Demo Disc 2023', [], [
    make_group('Games', [], root_groups),
    make_group('Tools', tools_idx, []),
    make_group('SDKs', sdks_idx, []),
    make_group('Drivers', drivers_idx, []),
])

# TODO: Sort games and/or sub-groups?

#pprint.pprint(grouping)

def print_groups_recursive(prefix, grouping):
    print(prefix, '== {} =='.format(strings[grouping['title']]))
    for child in grouping['children']:
        print(prefix, ' ', games[child]['Name'])
    for subgroup in grouping['subgroups']:
        print_groups_recursive(prefix + '  ', subgroup)

print_groups_recursive('', grouping)

def serialize_groups_recursive(fp, grouping):
    title_idx = grouping['title']
    fp.write(struct.pack('<BBB', title_idx, len(grouping['children']), len(grouping['subgroups'])))
    for game_idx in grouping['children']:
        fp.write(struct.pack('<B', game_idx))
    for subgroup in grouping['subgroups']:
        serialize_groups_recursive(fp, subgroup)


string_lists = []

def get_string_list_index(sl):
    global string_lists
    if sl not in string_lists:
        string_lists.append(sl)
    return string_lists.index(sl)

def make_string_list(strings):
    indices = [get_string_index(string) for string in strings]
    return get_string_list_index(indices)


FLAG_HAS_END_SCREEN = (1 << 0)
FLAG_IS_32_BITS = (1 << 1)
FLAG_IS_MULTIPLAYER = (1 << 2)
FLAG_IS_OPEN_SOURCE = (1 << 3)
FLAG_MOUSE_SUPPORTED = (1 << 4)
FLAG_MOUSE_REQUIRED = (1 << 5)
FLAG_KEYBOARD_SUPPORTED = (1 << 6)
FLAG_JOYSTICK_SUPPORTED = (1 << 7)
FLAG_REQUIRES_EGA = (1 << 8)
FLAG_REQUIRES_VGA = (1 << 9)
FLAG_REQUIRES_VBE1 = (1 << 10)
FLAG_REQUIRES_VBE2 = (1 << 11)
FLAG_DOSBOX_INCOMPATIBLE = (1 << 12)

names = []
descriptions = []
urls = []
ids = []
readmes = []

def pack_strings(fp, strings):
    fp.write(struct.pack('<H', len(strings)))
    for string in strings:
        length = len(string.encode('cp850')) + 1
        fp.write(struct.pack('<B', length))
    for string in strings:
        fp.write(string.encode('cp850'))
        fp.write(b'\0')

def pack_index_list(fp, string_lists):
    fp.write(struct.pack('<B', len(string_lists)))
    for string_list in string_lists:
        fp.write(struct.pack('<B', len(string_list)))
        for string_idx in string_list:
            fp.write(struct.pack('<B', string_idx))

with open(args.outfile, 'wb') as fp:
    fp.write(struct.pack('<B', len(games)))
    for game in games:
        names.append(game['Name'])
        descriptions.append(game['Description'])
        urls.append(game['URL'])
        ids.append(game['ID'])
        readmes.append(game['Readme'])

        genre_idx = get_string_index(game['Genre'])
        run_idx = get_string_index(game['Run'])
        loader_idx = get_string_index(game['Loader'])
        jam_idx = get_string_index(game['Jam'])
        exit_key_idx = get_string_index(game['ExitKey'])
        author_list_idx = make_string_list(game['Author'])
        video_list_idx = make_string_list(game['Video'])
        sound_list_idx = make_string_list(game['Sound'])
        toolchain_list_idx = make_string_list(game['Toolchain'])
        type_idx = get_string_index(game['Type'])
        prefix_idx = get_string_index(PREFIX_STRINGS[game['Kind']])

        cpu_bits = {'16-bit': 16, '32-bit': 32}[game['CPU']]

        flags = 0
        if cpu_bits == 32:
            flags |= FLAG_IS_32_BITS
        if game['Multiplayer']:
            flags |= FLAG_IS_MULTIPLAYER
        if game['EndScreen']:
            flags |= FLAG_HAS_END_SCREEN
        if game['Open Source']:
            flags |= FLAG_IS_OPEN_SOURCE

        if game['Mouse'] == 'Required':
            flags |= FLAG_MOUSE_SUPPORTED
            flags |= FLAG_MOUSE_REQUIRED
        elif game['Mouse'] == 'Optional':
            flags |= FLAG_MOUSE_SUPPORTED
        else:
            assert game['Mouse'] == '-'

        if game['Joystick'] == 'Optional':
            flags |= FLAG_JOYSTICK_SUPPORTED
        else:
            assert game['Joystick'] == '-'

        cga_supported = ('Text' in game['Video'] or 'CGA' in game['Video'])
        ega_supported = ('EGA' in game['Video'])
        vga_supported = ('VGA' in game['Video'])
        vbe1_supported = ('VBE1' in game['Video'])
        vbe2_supported = ('VBE2' in game['Video'])

        if ega_supported and not cga_supported:
            flags |= FLAG_REQUIRES_EGA

        if vga_supported and not ega_supported and not cga_supported:
            flags |= FLAG_REQUIRES_VGA

        if not vga_supported and not ega_supported and not cga_supported:
            if vbe1_supported:
                flags |= FLAG_REQUIRES_VBE1
            elif vbe2_supported:
                flags |= FLAG_REQUIRES_VBE2

        if game['Keyboard']:
            flags |= FLAG_KEYBOARD_SUPPORTED

        if game['IncompatibleDOSBox']:
            flags |= FLAG_DOSBOX_INCOMPATIBLE

        kilobytes = game['Kilobytes']

        fp.write(struct.pack('<IHHHHHHHBBBBBB',
                             # 32-bit file size, in kilobytes
                             kilobytes,

                             # 16-bit flags and string index values
                             flags,
                             run_idx, exit_key_idx, genre_idx, loader_idx, jam_idx, type_idx,

                             # 8-bit string index values
                             prefix_idx,

                             # 8-bit string list index values
                             author_list_idx, video_list_idx, sound_list_idx, toolchain_list_idx,

                             # 8-bit number of screenshots
                             game['NumScreenshots']))

    pack_strings(fp, names)
    pack_strings(fp, descriptions)
    pack_strings(fp, urls)
    pack_strings(fp, ids)
    pack_strings(fp, readmes)
    pack_strings(fp, strings)

    pack_index_list(fp, string_lists)

    serialize_groups_recursive(fp, grouping)

print(f'Num games: {len(games)}')
print(f'Num strings: {len(strings)}')
print(f'Num string lists: {len(string_lists)}')
