import os
import subprocess
from PIL import Image

max_colors = 64

for dirpath, dirnames, filenames in os.walk('png'):
    for idx, filename in enumerate(sorted(filenames)):
        fn = os.path.join(dirpath, filename)
        on = os.path.join('pcx' + os.path.dirname(fn)[3:],  f'shot{idx}.pcx')
        pn = on[:-4] + '.dat'
        print(fn, '=>', on, '=>', pn)
        os.makedirs(os.path.dirname(on), exist_ok=True)
        subprocess.check_call(['convert', fn, '-resize', '320!x200!', '-colors', str(max_colors-1), on])
        img = Image.open(on)
        assert img.width == 320, img.width
        assert img.height == 200, img.height
        palette = []
        for k, v in img.palette.colors.items():
            assert v == len(palette)
            palette.append(k)
        assert len(palette) <= max_colors, (on, len(palette))
        px = img.load()
        # TODO: Could write palette + pixels to .dat
        del px
        del img
