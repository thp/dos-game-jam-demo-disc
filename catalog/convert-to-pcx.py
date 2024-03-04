import os
import subprocess
from PIL import Image

max_colors = 60

for dirpath, dirnames, filenames in os.walk('png'):
    for idx, filename in enumerate(sorted(filenames)):
        fn = os.path.join(dirpath, filename)
        on = os.path.join('pcx' + os.path.dirname(fn)[3:],  f'shot{idx}.pcx')
        ob = os.path.join('pcx' + os.path.dirname(fn)[3:],  f'blur{idx}.pcx')
        print(fn, '=>', on)
        os.makedirs(os.path.dirname(on), exist_ok=True)
        subprocess.check_call(['convert', fn, '-resize', '320!x200!', '-colors', str(max_colors-1), on])
        subprocess.check_call(['convert', fn,
                               '-resize', '320!x200!',
                               '-modulate', '60',
                               '-blur', '0x8',
                               '-dither', 'None',
                               '-colors', str(max_colors-1),
                               ob])

for dirpath, dirnames, filenames in os.walk('pcx'):
    for filename in filenames:
        if not filename.endswith('.pcx'):
            continue

        fn = os.path.join(dirpath, filename)
        on = os.path.join(dirpath, filename[:-4] + '.dat')
        print(fn, '=>', on)

        img = Image.open(fn)
        assert img.width == 320, img.width
        assert img.height == 200, img.height
        palette = []
        for k, v in img.palette.colors.items():
            assert v == len(palette)
            palette.append(k)
        assert len(palette) <= max_colors, (fn, len(palette))
        px = img.load()
        with open(on, 'wb') as fp:
            pixels = []
            for y in range(img.height):
                for x in range(img.width):
                    pixels.append(px[x, y])
            for i in range(max_colors):
                pixels.extend(palette[i] if len(palette) > i else (0, 0, 0))
            fp.write(bytearray(pixels))
        del px
        del img
