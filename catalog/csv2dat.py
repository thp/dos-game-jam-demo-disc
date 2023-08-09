import collections
import struct
import argparse

#
# DOS Game Jam Demo Disc Game Catalog Format
# 2023-08-09 Thomas Perl <m@thp.io>
#

parser = argparse.ArgumentParser(description='Convert CSV game list to datafile')
parser.add_argument('infile', type=str, help='Filename of CSV file')
parser.add_argument('outfile', type=str, help='Filename of DAT file')
args = parser.parse_args()

header = None
values = collections.defaultdict(set)
games = []

with open(args.infile) as fp:
    for line in fp:
        parts = line.lstrip('\ufeff').rstrip('\n').split(';')
        if header is None:
            header = parts
        else:
            d = dict(zip(header, parts))
            d['Kilobytes'] = int(d['Kilobytes'] or -1)
            d['Visible'] = (d['Visible'] == 'Y')
            d['Keyboard'] = (d['Keyboard'] == 'Y')
            d['Multiplayer'] = (d['Multiplayer'] == 'Y')
            d['Keyboard'] = (d['Keyboard'] == 'Y')
            d['EndScreen'] = (d['EndScreen'] == 'Y')
            d['Open Source'] = (d['Open Source'] == 'Y')
            d['Video'] = tuple(d['Video'].split(', '))
            d['Sound'] = tuple(x for x in d['Sound'].split(', ') if x != '-')
            d['Author'] = tuple(d['Author'].split(', '))
            d['Toolchain'] = tuple(d['Toolchain'].split(', '))
            d['CPU'] = d['Bits'] + '-bit'
            del d['Bits']

            if not d['Visible']:
                print('Skip:', d['Name'])
                continue

            #print(d)
            games.append(d)
            for k, v in d.items():
                values[k].add(v)

import pprint
#pprint.pprint(values)
#pprint.pprint(games)
#pprint.pprint(values.keys())

groupings = [
    'Genre',
    'Jam',
    'Type',
    'Author',
    'Video',
    'Multiplayer',
    'Mouse',
    'Joystick',
    'Sound',
    'CPU',
    'Toolchain',
    'Open Source',
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

def make_group(title, children, subgroups):
    return {'title': get_string_index(title), 'children': children, 'subgroups': subgroups}

root_groups = []

for group in groupings:
    children = []
    subgroups = []
    for value in sorted(split_value(group, values[group])):
        if value in ('', '?', '-'):
            continue

        print(group, '/', value)
        game_indices = []
        for game_idx, game in enumerate(games):
            if value == game[group] or (isinstance(game[group], tuple) and value in game[group]):
                print('\t', game['Name'])
                game_indices.append(game_idx)

        if value == True:
            # direct child stuff
            children.extend(game_indices)
        else:
            # submenu
            subgroup = make_group(value, game_indices, [])
            subgroups.append(subgroup)

    root_groups.append(make_group(group, children, subgroups))

root_groups.append(make_group('All Games', list(range(len(games))), []))

grouping = make_group('', [], root_groups)

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

names = []
descriptions = []
urls = []

def pack_strings(fp, strings):
    fp.write(struct.pack('<B', len(strings)))
    for string in strings:
        length = len(string.encode()) + 1
        fp.write(struct.pack('<B', length))
    for string in strings:
        fp.write(string.encode())
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

        if game['Keyboard']:
            flags |= FLAG_KEYBOARD_SUPPORTED

        kilobytes = game['Kilobytes']

        fp.write(struct.pack('<IBBBBBBBBBBBB', kilobytes, flags,
                             run_idx, loader_idx, jam_idx, genre_idx, exit_key_idx, type_idx,
                             author_list_idx, video_list_idx, sound_list_idx, toolchain_list_idx,
                             0))

    pack_strings(fp, names)
    pack_strings(fp, descriptions)
    pack_strings(fp, urls)
    pack_strings(fp, strings)

    pack_index_list(fp, string_lists)

    serialize_groups_recursive(fp, grouping)

print(f'Num games: {len(games)}')
print(f'Num strings: {len(strings)}')
print(f'Num string lists: {len(string_lists)}')
