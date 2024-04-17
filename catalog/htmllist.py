import argparse
import csv

parser = argparse.ArgumentParser(description='Convert CSV game list to HTML')
parser.add_argument('--verbose', action='store_true', help='Be verbose')
parser.add_argument('infile', type=str, help='Filename of CSV file')
parser.add_argument('outfile', type=str, help='Filename of HTML file')
args = parser.parse_args()

items = []

with open(args.infile) as fp:
    reader = csv.DictReader(fp)
    for parts in reader:
        print(parts)
        d = dict(parts)
        if d['Visible'] == 'Y':
            items.append((d['Name'].lower(), d['Name'], d['URL'], d['Author']))

with open(args.outfile, 'w') as fp:
    for _, name, url, author in sorted(items):
        print(f'<li><a href="{url}">{name}</a> by {author}</li>', file=fp)
