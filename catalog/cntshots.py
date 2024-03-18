import glob
import os

lines = []

with open('gamectlg.csv', 'r', encoding='utf-8') as fp:
    for idx, line in enumerate(fp.read().splitlines()):
        if idx == 0:
            lines.append(line)
        else:
            head, dirname, count = line.rsplit(',', 2)
            count = int(count)
            files = glob.glob(os.path.join('scrnshot', dirname, 'shot*.dat'))
            count = len(files)
            for i in range(count):
                fn = f'scrnshot/{dirname}/shot{i}.dat'
                assert os.path.exists(fn), fn
            assert count == len(files), line
            lines.append(f'{head},{dirname},{count}')

with open('gamectlg.csv.new', 'w', encoding='utf-8') as fp:
    for line in lines:
        print(line, file=fp)

os.rename('gamectlg.csv.new', 'gamectlg.csv')
