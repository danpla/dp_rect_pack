#!/usr/bin/env python3

from collections import namedtuple
from itertools import count
from urllib.request import pathname2url
import os
import random
import subprocess
import sys
import textwrap
import struct
import io


LIB_URL = 'https://danpla.github.io/dp_rect_pack'

OUT_DIR = 'html'
IMAGE_DIR = 'img'
DATA_DIR = os.path.join(IMAGE_DIR, 'data')

MAX_PAGES = 9999
THUMBNAIL_SIZE = 320

demo_exe_path = os.path.join('..', 'demo', 'demo')
if os.name == 'nt':
    demo_exe_path += '.exe'


RectSet = namedtuple('RectSet', 'name title count range pot')
RECT_SETS = (
    RectSet(
        'rects', 'rectangles',
        500, ((6, 100), (6, 100)), False),
    RectSet(
        'rects_tall', 'tall rectangles',
        500, ((6, 30), (30, 100)), False),
    RectSet(
        'rects_wide', 'wide rectangles',
        500, ((30, 100), (6, 30)), False),
    RectSet(
        'rects_pot', 'power of two rectangles',
        500, ((2, 7), (2, 7)), True),
    RectSet(
        'squares', 'squares',
        500, ((6, 100), None), False),
    RectSet(
        'squares_pot', 'power of two squares',
        500, ((2, 7), None), True),
    )


SIZE_LIMITS = (None, 512)


def size_limit_to_str(size_limit):
    if size_limit is None:
        return 'infinite'
    else:
        return str(size_limit)


def create_image_name(rect_set, size_limit, page_idx):
    return '{}_{}_{:04}.png'.format(
        rect_set.name, size_limit_to_str(size_limit), page_idx)


def generate_data():
    data_out_dir = os.path.join(OUT_DIR, DATA_DIR)
    os.makedirs(data_out_dir, exist_ok=True)
    for rect_set in RECT_SETS:
        file_path = os.path.join(data_out_dir, rect_set.name + '.txt')
        if os.path.isfile(file_path):
            print(file_path, 'exists; delete it to generate a new one')
            continue

        with open(file_path, 'w') as f:
            for i in range(rect_set.count):
                w_range, h_range = rect_set.range

                w = random.randint(*w_range)
                if h_range is not None:
                    # Ensure we don't generate squares
                    while True:
                        h = random.randint(*h_range)
                        if h != w:
                            break
                else:
                    h = w

                if rect_set.pot:
                    w = 2 ** w
                    h = 2 ** h

                f.write('{}x{}\n'.format(w, h))


def delete_images():
    out_image_dir = os.path.join(OUT_DIR, IMAGE_DIR)
    if not os.path.isdir(out_image_dir):
        return

    for rect_set in RECT_SETS:
        for size_limit in SIZE_LIMITS:
            for i in range(MAX_PAGES):
                image_name = create_image_name(rect_set, size_limit, i)
                out_image_path = os.path.join(out_image_dir, image_name)
                try:
                    os.remove(out_image_path)
                except FileNotFoundError:
                    pass


def render_image(name, size_limit):
    data_path = os.path.join(OUT_DIR, DATA_DIR, name + '.txt')
    if not os.path.isfile(data_path):
        sys.exit('{} does not exist'.format(data_path))
        return

    opts = [
        demo_exe_path,
        '-out-dir', os.path.join(OUT_DIR, IMAGE_DIR),
        ]

    prefix = name + '_'
    if size_limit is not None:
        prefix += str(size_limit) + '_'
        opts.extend(['-max-size', str(size_limit)])
    else:
        prefix += 'infinite_'

    opts.extend(['-image-prefix', prefix])
    opts.append(data_path)

    print(
        'Rendering', data_path,
        'with', size_limit_to_str(size_limit), 'size limit')
    exit_status = subprocess.call(opts)
    if exit_status != 0:
        sys.exit(
            '{} terminated with exit status {}'.format(
                demo_exe_path, exit_status))


def render_images():
    out_image_dir = os.path.join(OUT_DIR, IMAGE_DIR)
    os.makedirs(out_image_dir, exist_ok=True)

    for rect_set in RECT_SETS:
        for size_limit in SIZE_LIMITS:
            render_image(rect_set.name, size_limit)


def get_png_size(path):
    with open(path, 'rb') as f:
        if f.read(8) != b'\x89PNG\x0d\x0a\x1a\x0a':
            raise ValueError('Not a PNG file')
        f.seek(4, io.SEEK_CUR)  # Skip chunk length
        if f.read(4) != b'IHDR':
            raise ValueError('Corrupt PNG file')
        return struct.unpack('>2I', f.read(8))


def write_html(f):
    f.write(textwrap.dedent("""\
        <!DOCTYPE html>
        <html>
          <head>
            <title>dp_rect_pack gallery</title>
            <meta charset="UTF-8">
            <style>
                body {
                    padding: 1em;
                    color: #333333;
                    font-family: sans-serif;
                    line-height: 1.5;
                }
                p {
                    max-width: 38em;
                }
                figure {
                    display: inline-block;
                    margin: 1em;
                }
                figure figcaption {
                    text-align: center;
                }
                figure a {
                    text-decoration: none;
                }
                img {
                    box-shadow: 0.1em 0.1em 0.2em #555555;
                }
            </style>
          </head>
          <body>
            <h1>dp_rect_pack gallery</h1>

          """))
    f.write(
        '    <p>'
        'This gallery shows various sets of rectangles packed by the '
        '<a href="{}">dp_rect_pack</a> library. Click on a thumbnail to see '
        'the full-size version of the image.</p>\n\n'.format(LIB_URL))

    titles = []
    for rect_set in RECT_SETS:
        title = '{} {}'.format(rect_set.count, rect_set.title)
        if rect_set.pot:
            range_fmt = '2<sup>[{}..{}]</sup>'
        else:
            range_fmt = '[{}..{}]'

        if rect_set.range[1] is None:
            title += ' ' + range_fmt.format(*rect_set.range[0])
        else:
            title += ' {} × {}'.format(
                range_fmt.format(*rect_set.range[0]),
                range_fmt.format(*rect_set.range[1]))

        titles.append(title)

    f.write('    <h2>Contents</h2>\n')
    f.write('    <ol>\n')
    for rect_set, title in zip(RECT_SETS, titles):
        f.write(
            '      <li><a href="#{}">{}</a></li>\n'.format(
                rect_set.name, title))
    f.write('    </ol>\n\n')

    for rect_set, title in zip(RECT_SETS, titles):
        data_url = pathname2url(
            os.path.join(DATA_DIR, rect_set.name + '.txt'))
        f.write(
            '    <h2 id="{name}">{} '
            '(<a href="{}">{name}.txt</a>)</h2>\n'.format(
                title, data_url, name=rect_set.name))

        for size_limit in SIZE_LIMITS:
            size_limit_str = size_limit_to_str(size_limit)
            if size_limit is None:
                section_title = 'Infinite page mode'
            else:
                section_title = 'Multipage mode with {} px size limit'.format(
                    size_limit)

            f.write(
                '    <h3 id="{}_{}">{}</h3>\n'.format(
                    rect_set.name, size_limit_str, section_title))

            image_paths = []
            image_sizes = []
            image_max_side = 0
            for i in range(MAX_PAGES):
                image_name = create_image_name(rect_set, size_limit, i)
                image_path = os.path.join(IMAGE_DIR, image_name)
                out_image_path = os.path.join(OUT_DIR, image_path)
                if not os.path.isfile(out_image_path):
                    break

                image_paths.append(image_path)
                image_size = get_png_size(out_image_path)
                image_sizes.append(image_size)
                image_max_side = max(image_max_side, *image_size)

            scale = min(1.0, THUMBNAIL_SIZE / image_max_side)

            for num, image_path, image_size in zip(
                    count(1), image_paths, image_sizes):
                w, h = image_size
                caption = 'P. {}, {} × {} px'.format(num, w, h)
                image_url = pathname2url(image_path)

                f.write('    <figure>\n')
                f.write('      <a href="{}" target="_blank">\n'.format(
                    image_url))
                f.write(
                    '       <img src="{}" title="{}" '
                    'width="{}" height="{}">\n'.format(
                        image_url, caption,
                        round(w * scale), round(w * scale)))
                f.write('      </a>\n')
                f.write('      <figcaption>{}</figcaption>\n'.format(caption))
                f.write('    </figure>\n')

        f.write('\n')

    f.write('  </body>\n</html> \n')


def generate_html():
    html_path = os.path.join(OUT_DIR, 'gallery.html')
    with open(html_path, 'w', encoding='utf-8') as f:
        write_html(f)


def main():
    if not os.path.isfile(demo_exe_path):
        sys.exit(
            '{} does not exist. '
            'Please compile the demo program and try again.'.format(
                demo_exe_path))

    generate_data()
    delete_images()
    render_images()
    generate_html()


if __name__ == '__main__':
    main()
