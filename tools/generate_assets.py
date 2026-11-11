"""Generate original, palette-limited pixel art and short synthesized SFX.

Requires Pillow only when regenerating assets; checked-in BMP/WAV files build as-is.
"""
from pathlib import Path
import json
import math
import struct
import wave
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
WAVE_BOUNDARIES = [56 + i * 168 // 5 for i in range(6)]
WAVE_WIDTHS = [end - start for start, end in zip(WAVE_BOUNDARIES, WAVE_BOUNDARIES[1:])]
WAVE_FRAME_OFFSETS = [0]
for width in WAVE_WIDTHS:
    WAVE_FRAME_OFFSETS.append(WAVE_FRAME_OFFSETS[-1] + width + 1)
COLORS = [
    (17, 30, 29), (29, 54, 42), (43, 78, 46), (57, 99, 51),
    (75, 119, 56), (127, 169, 65), (186, 209, 96), (255, 211, 80),
    (255, 241, 191), (124, 76, 49), (180, 112, 54), (211, 69, 52),
    (114, 143, 126), (190, 199, 169), (246, 152, 52), (65, 125, 135),
]
FONT_ROWS = {
    'A':'010/101/111/101/101', 'B':'110/101/110/101/110',
    'C':'011/100/100/100/011', 'D':'110/101/101/101/110',
    'E':'111/100/110/100/111', 'F':'111/100/110/100/100',
    'G':'011/100/101/101/011', 'H':'101/101/111/101/101',
    'I':'111/010/010/010/111', 'J':'001/001/001/101/010',
    'K':'101/101/110/101/101', 'L':'100/100/100/100/111',
    'M':'101/111/111/101/101', 'N':'101/111/111/111/101',
    'O':'010/101/101/101/010', 'P':'110/101/110/100/100',
    'Q':'010/101/101/111/011', 'R':'110/101/110/101/101',
    'S':'011/100/010/001/110', 'T':'111/010/010/010/010',
    'U':'101/101/101/101/111', 'V':'101/101/101/101/010',
    'W':'101/101/111/111/101', 'X':'101/101/010/101/101',
    'Y':'101/101/010/010/010', 'Z':'111/001/010/100/111',
    '0':'111/101/101/101/111', '1':'010/110/010/010/111',
    '2':'110/001/010/100/111', '3':'110/001/010/001/110',
    '4':'101/101/111/001/001', '5':'111/100/110/001/110',
    '6':'011/100/111/101/111', '7':'111/001/010/010/010',
    '8':'111/101/111/101/111', '9':'111/101/111/001/110',
    ':':'000/010/000/010/000', '.':'000/000/000/000/010',
    '/':'001/001/010/100/100', '-':'000/000/111/000/000',
    '+':'000/010/111/010/000', '!':'010/010/010/000/010',
    '?':'110/001/010/000/010', '=':'000/111/000/111/000',
}


def canvas(width, height, color=0):
    image = Image.new('P', (width, height), color)
    image.putpalette([v for c in COLORS for v in c] + [0] * (768 - len(COLORS) * 3))
    return image


def text(image, x, y, value, color=8, scale=1, centered=False):
    if centered:
        x -= (len(value) * 4 - 1) * scale // 2
    draw = ImageDraw.Draw(image)
    for char in value.upper():
        for row, bits in enumerate(FONT_ROWS.get(char, '000/000/000/000/000').split('/')):
            for column, bit in enumerate(bits):
                if bit == '1':
                    px, py = x + column * scale, y + row * scale
                    draw.rectangle((px, py, px + scale - 1, py + scale - 1), fill=color)
        x += 4 * scale


def sprite(frame):
    image = canvas(16, 16)
    d = ImageDraw.Draw(image)
    # Shared palette and silhouette language keep the game readable at native resolution.
    if frame in (0, 1):
        d.line((7, 9, 7, 14), fill=5, width=2)
        d.ellipse((1, 11, 7, 14), fill=4)
        d.ellipse((8, 11, 13, 14), fill=5)
    if frame == 0:
        for x, y in [(3, 1), (7, 0), (10, 2), (11, 6), (8, 9), (3, 8), (0, 4)]:
            d.ellipse((x, y, x + 4, y + 4), fill=7)
        d.ellipse((3, 3, 11, 10), fill=9)
        d.ellipse((4, 3, 10, 8), fill=10)
        d.point((5, 5), fill=0)
        d.point((9, 5), fill=0)
        d.line((6, 8, 8, 8), fill=8)
    elif frame == 1:
        d.ellipse((2, 2, 12, 10), fill=2)
        d.ellipse((3, 1, 12, 9), fill=5)
        d.rectangle((9, 4, 14, 8), fill=5)
        d.ellipse((12, 3, 15, 9), fill=3)
        d.rectangle((14, 5, 15, 7), fill=0)
        d.rectangle((6, 3, 7, 4), fill=0)
        d.point((4, 3), fill=6)
    elif frame == 2:
        d.ellipse((2, 1, 13, 15), fill=9)
        d.ellipse((3, 1, 12, 13), fill=10)
        d.line((5, 3, 4, 10), fill=7)
        d.line((10, 3, 11, 11), fill=9)
        d.rectangle((5, 5, 5, 7), fill=0)
        d.rectangle((9, 5, 9, 7), fill=0)
        d.line((6, 10, 8, 10), fill=9)
    elif frame == 3:
        d.line((4, 7, 9, 1, 12, 7), fill=5, width=2)
        d.ellipse((1, 6, 8, 14), fill=11)
        d.ellipse((8, 6, 15, 14), fill=11)
        d.point((3, 8), fill=8)
        d.point((10, 8), fill=8)
        d.point((5, 10), fill=0)
        d.point((12, 10), fill=0)
    elif frame in (4, 5):
        d.rectangle((5, 8, 11, 12), fill=9)
        d.line((6, 12, 5, 14, 2, 14), fill=15, width=2)
        d.line((10, 12, 11, 15, 8, 15), fill=15, width=2)
        d.rectangle((3, 2, 11, 7), fill=12)
        d.rectangle((4, 1, 10, 5), fill=13)
        d.rectangle((2, 5, 6, 8), fill=12)
        d.rectangle((3, 3, 5, 4), fill=8)
        d.point((3, 4), fill=0)
        d.line((3, 7, 6, 7), fill=0)
        d.line((5, 9, 1, 10), fill=12, width=2)
        d.line((9, 8, 6, 10), fill=8)
        d.point((7, 10), fill=11)
        if frame == 5:
            d.polygon([(5, 0), (3, 4), (12, 4), (9, 0)], fill=14)
            d.line((4, 3, 11, 3), fill=8)
    elif frame == 6:
        d.ellipse((5, 5, 10, 10), fill=2)
        d.ellipse((5, 4, 9, 8), fill=6)
        d.point((6, 5), fill=8)
    elif frame == 7:
        d.line((0, 7, 15, 7), fill=7)
        d.line((7, 0, 7, 15), fill=7)
        d.line((2, 2, 13, 13), fill=7)
        d.line((2, 13, 13, 2), fill=7)
        d.ellipse((3, 3, 12, 12), fill=14)
        d.ellipse((4, 3, 11, 10), fill=7)
        d.point((6, 5), fill=8)
    elif frame == 8:
        d.line((3, 8, 2, 1, 0, 1), fill=13, width=2)
        d.rectangle((3, 7, 13, 12), fill=11)
        d.rectangle((6, 5, 11, 8), fill=12)
        d.ellipse((2, 10, 6, 14), fill=0)
        d.ellipse((10, 10, 14, 14), fill=0)
        d.point((4, 12), fill=13)
        d.point((12, 12), fill=13)
    elif frame == 9:
        d.polygon([(8, 0), (10, 4), (15, 2), (12, 7), (15, 11),
                   (10, 11), (8, 15), (5, 11), (0, 13), (3, 8), (0, 3), (5, 4)], fill=14)
        d.ellipse((3, 3, 12, 12), fill=7)
        d.ellipse((5, 5, 10, 10), fill=8)
    elif frame in (10, 11):
        image = sprite(2)
        d = ImageDraw.Draw(image)
        d.line((9, 1, 7, 4, 9, 7, 7, 10, 8, 14), fill=9)
        if frame == 11:
            d.line((3, 8, 6, 9, 4, 12), fill=0)
            d.line((8, 5, 12, 3), fill=0)
            d.rectangle((6, 10, 8, 11), fill=0)
    return image


def save(image, name, metadata):
    image.save(ROOT / 'graphics' / f'{name}.bmp')
    (ROOT / 'graphics' / f'{name}.json').write_text(json.dumps(metadata) + '\n')


def lawn():
    image = canvas(256, 256, 1)
    d = ImageDraw.Draw(image)
    d.rectangle((0, 0, 239, 55), fill=0)
    d.rectangle((WAVE_BOUNDARIES[0], 14, WAVE_BOUNDARIES[-1], 17), fill=1)
    d.line((WAVE_BOUNDARIES[0], 14, WAVE_BOUNDARIES[-1], 14), fill=9)
    d.line((WAVE_BOUNDARIES[0], 17, WAVE_BOUNDARIES[-1], 17), fill=9)
    d.line((8, 21, 231, 21), fill=2)
    d.line((0, 55, 239, 55), fill=9, width=2)
    d.rectangle((0, 57, 14, 153), fill=9)
    for y in range(58, 153, 10):
        d.line((0, y, 14, y), fill=10)
    for row in range(5):
        for column in range(9):
            x, y = 34 + column * 22, 58 + row * 19
            d.rectangle((x, y, x + 21, y + 18), fill=3 if (row + column) % 2 else 4)
            d.line((x + 3, y + 17, x + 4, y + 15), fill=2)
            d.point((x + 16, y + 5), fill=5)
    d.rectangle((15, 57, 32, 153), fill=2)
    d.rectangle((234, 57, 239, 153), fill=9)
    # Decorative soil border replaces the old text/status footer.
    d.rectangle((0, 154, 239, 159), fill=9)
    d.line((0, 154, 239, 154), fill=5)
    for x in range(6, 239, 13):
        d.line((x, 157, x + 2, 155), fill=4)
        d.point((x + 5, 159), fill=10)
    stamp(image, 7, 3, 24)
    text(image, 21, 26, 'SUN', color=7)
    for index, (name, cost) in enumerate([('SUN', '50'), ('PEA', '100'), ('NUT', '50'), ('BOMB', '150')]):
        x = 54 + index * 42
        d.rounded_rectangle((x, 24, x + 39, 52), radius=2, fill=2, outline=9)
        stamp(image, index, x + 2, 28)
        text(image, x + 20, 29, name, color=6)
        text(image, x + 20, 40, cost, color=7)
    return image


def stamp(image, frame, x, y, scale=1):
    icon = sprite(frame).resize((16 * scale, 16 * scale), Image.Resampling.NEAREST)
    # Transparency follows palette index zero, not its nonzero RGB luminance.
    image.paste(icon, (x, y), icon.point([0] + [255] * 255, 'L'))


def skull_flag(draw, pole, top, color):
    """12x10 cloth keeps a comfortable margin around the six-pixel skull."""
    draw.polygon([(pole - 12, top), (pole - 1, top), (pole - 1, top + 9),
                  (pole - 12, top + 8), (pole - 10, top + 4)], fill=color, outline=9)
    draw.line((pole - 10, top + 1, pole - 2, top + 1), fill=14 if color == 11 else 6)
    skull = [
        '.####.',
        '######',
        '#o##o#',
        '##oo##',
        '.####.',
        '.#oo#.',
    ]
    for row, pixels in enumerate(skull):
        for column, pixel in enumerate(pixels):
            if pixel != '.':
                draw.point((pole - 9 + column, top + 2 + row), fill=8 if pixel == '#' else 1)


def hourglass_icon(stage):
    image = canvas(16, 16)
    d = ImageDraw.Draw(image)
    d.rounded_rectangle((0, 0, 15, 15), radius=3, fill=1, outline=3)
    # Two distinct glass bulbs and a narrow waist inside a brass frame.
    glass = [(4, 3), (11, 3), (11, 5), (8, 8), (11, 10),
             (11, 12), (4, 12), (4, 10), (7, 8), (4, 5)]
    d.polygon(glass, fill=15, outline=8)
    d.rectangle((3, 1, 12, 2), fill=7)
    d.line((4, 1, 11, 1), fill=8)
    d.rectangle((3, 13, 12, 14), fill=14)
    d.line((4, 13, 11, 13), fill=7)
    if stage == 0:
        d.polygon([(5, 4), (10, 4), (8, 7)], fill=7)
        d.line((5, 11, 10, 11), fill=7)
    elif stage == 1:
        d.polygon([(6, 4), (9, 4), (8, 6)], fill=7)
        d.polygon([(5, 11), (8, 9), (10, 11)], fill=7)
    else:
        d.point((8, 5), fill=7)
        d.polygon([(5, 11), (5, 10), (8, 8), (10, 10), (10, 11)], fill=7)
    d.point((8, 8), fill=8)
    return image


def wave_layout_header():
    # The renderer and asset textures use these same integer boundaries/frames.
    content = '''// Generated by tools/generate_assets.py. Edit the generator to change geometry.
#ifndef GARDEN_WAVE_TRACK_H
#define GARDEN_WAVE_TRACK_H
#include <array>
namespace garden::wave_track
{
inline constexpr std::array<int, 6> boundaries{{@BOUNDARIES@}};
inline constexpr std::array<int, 6> frame_offsets{{@OFFSETS@}};
inline constexpr int sprite_width = 64;
inline constexpr int sprite_height = 32;
inline constexpr int bar_y = 14;
inline constexpr int head_y = 10;

constexpr int position(int progress)
{
    if(progress <= 0) { return boundaries.front(); }
    if(progress >= 500) { return boundaries.back(); }
    int wave = progress / 100;
    int width = boundaries[wave + 1] - boundaries[wave];
    return boundaries[wave] + width * (progress % 100) / 100;
}

constexpr int fill(int wave, int progress)
{
    int pixels = position(progress) - boundaries[wave];
    int width = boundaries[wave + 1] - boundaries[wave];
    return pixels < 0 ? 0 : (pixels > width ? width : pixels);
}
}
#endif
'''.replace('@BOUNDARIES@', ', '.join(map(str, WAVE_BOUNDARIES))).replace(
        '@OFFSETS@', ', '.join(map(str, WAVE_FRAME_OFFSETS)))
    path = ROOT / 'include' / 'wave_track.h'
    if not path.exists() or path.read_text() != content:
        path.write_text(content)


def menu_base():
    """Independent menu artwork: never derived from the lawn or gameplay HUD."""
    image = canvas(256, 256, 1)
    d = ImageDraw.Draw(image)
    for x in range(4, 240, 14):
        for y in range(5, 160, 14):
            d.point((x, y), fill=2)
    d.rectangle((5, 5, 234, 154), outline=3)
    for x, y, sx, sy in [(9, 9, 1, 1), (230, 9, -1, 1), (9, 150, 1, -1), (230, 150, -1, -1)]:
        d.line((x, y + sy * 9, x, y, x + sx * 9, y), fill=6)
    return image


def menu_heading(image, eyebrow, heading):
    text(image, 120, 16, eyebrow, color=6, centered=True)
    text(image, 120, 30, heading, color=7, scale=2, centered=True)


def title_screen():
    image = menu_base()
    d = ImageDraw.Draw(image)
    menu_heading(image, 'A POCKET LAWN DEFENSE', 'GARDEN GUARD')
    # A small illustrated garden, rather than a duplicate of the game board.
    d.rectangle((25, 78, 214, 89), fill=3)
    d.line((25, 89, 214, 89), fill=9, width=2)
    stamp(image, 0, 40, 53, 2)
    stamp(image, 1, 77, 53, 2)
    stamp(image, 6, 117, 61)
    stamp(image, 5, 165, 53, 2)
    text(image, 120, 99, 'D-PAD MOVE   A PLANT   B COLLECT SUN', centered=True)
    text(image, 120, 110, 'L/R SEED   SELECT + A SHOVEL', centered=True)
    text(image, 120, 123, 'FIVE WAVES. ONE GARDEN. NO BRAINS LOST.', color=6, centered=True)
    d.rounded_rectangle((55, 136, 184, 148), radius=3, fill=3)
    text(image, 120, 140, 'START OR A TO PLAY', color=7, centered=True)
    return image


def pause_screen():
    image = menu_base()
    menu_heading(image, 'TAKE YOUR TIME', 'GARDEN PAUSED')
    # Live run statistics and selection text are rendered by Butano, on top.
    d = ImageDraw.Draw(image)
    d.rounded_rectangle((25, 61, 214, 121), radius=4, fill=0, outline=3)
    text(image, 120, 130, 'UP/DOWN CHOOSE   A CONFIRM', color=6, centered=True)
    text(image, 120, 141, 'START OR B TO RESUME', centered=True)
    return image


def result_screen(won):
    image = menu_base()
    menu_heading(image, 'THE DAY IS YOURS' if won else 'THE GARDEN NEEDS YOU',
                 'LAWN SAVED!' if won else 'THEY GOT IN!')
    stamp(image, 0 if won else 4, 22, 54, 2)
    stamp(image, 1 if won else 5, 186, 54, 2)
    text(image, 120, 101, 'EVERY LANE. EVERY WAVE. WELL PLAYED.' if won else
         'MORE SUNFLOWERS. COVER EVERY LANE.', color=6, centered=True)
    text(image, 120, 114, 'YOUR NEXT GARDEN IS A BUTTON AWAY.', centered=True)
    text(image, 120, 135, 'START OR A: PLAY AGAIN', color=7, centered=True)
    text(image, 120, 145, 'B: TITLE SCREEN', centered=True)
    return image


def sound(name, notes):
    rate = 16000
    data = []
    for frequency, seconds in notes:
        length = int(rate * seconds)
        for i in range(length):
            envelope = min(1, i / 70) * (1 - i / length) ** 2
            sample = math.sin(2 * math.pi * frequency * i / rate)
            data.append(struct.pack('<h', int(sample * envelope * 10000)))
    with wave.open(str(ROOT / 'audio' / f'{name}.wav'), 'wb') as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(rate)
        output.writeframes(b''.join(data))


def chewing_sound():
    """A quiet pair of rounded, woody nibbles with smooth attacks and releases."""
    rate = 16000
    data = []
    phase = 0.0
    for i in range(int(rate * .105)):
        local = i / rate % .055
        duration = .037
        if local < duration:
            progress = local / duration
            phase += 2 * math.pi * (190 - 85 * progress) / rate
            envelope = math.sin(math.pi * progress) ** 2
            sample = (.8 * math.sin(phase) + .2 * math.sin(phase * 2.3)) * envelope
        else:
            sample = 0
        data.append(struct.pack('<h', int(sample * 5500)))
    with wave.open(str(ROOT / 'audio' / 'chew.wav'), 'wb') as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(rate)
        output.writeframes(b''.join(data))


def main():
    (ROOT / 'graphics').mkdir(exist_ok=True)
    (ROOT / 'audio').mkdir(exist_ok=True)
    wave_layout_header()
    atlas = canvas(16, 192)
    for index in range(12):
        atlas.paste(sprite(index), (0, index * 16))
    save(atlas, 'actors', {'type': 'sprite', 'height': 16, 'bpp_mode': 'bpp_4'})
    for name, color in [('cursor', 8), ('selection', 7)]:
        image = canvas(32 if name == 'cursor' else 64, 128 if name == 'cursor' else 96)
        d = ImageDraw.Draw(image)
        if name == 'cursor':
            for frame, tint in enumerate([8, 11, 7, 6]):
                for x, y, sx, sy in [(5, 7, 1, 1), (26, 7, -1, 1), (5, 24, 1, -1), (26, 24, -1, -1)]:
                    y += frame * 32
                    d.line((x, y + sy * 5, x, y, x + sx * 5, y), fill=tint, width=2)
        else:
            for frame, tint in enumerate([7, 11, 8]):
                d.rounded_rectangle((12, 2 + frame * 32, 51, 30 + frame * 32), radius=2, outline=tint)
        save(image, name, {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    bars = canvas(32, 8 * 18)
    d = ImageDraw.Draw(bars)
    for frame in range(18):
        y = frame * 8 + 3
        d.rectangle((0, y, 31, y + 1), fill=1)
        width = frame * 2 if frame < 17 else 32
        if width:
            d.rectangle((0, y, width - 1, y + 1), fill=11 if frame == 17 else (6 if frame == 16 else 7))
    save(bars, 'seed_bars', {'type': 'sprite', 'height': 8, 'bpp_mode': 'bpp_4'})
    wave_bars = canvas(64, 32 * WAVE_FRAME_OFFSETS[-1])
    d = ImageDraw.Draw(wave_bars)
    for wave_index, width in enumerate(WAVE_WIDTHS):
        for filled in range(width + 1):
            y = (WAVE_FRAME_OFFSETS[wave_index] + filled) * 32
            d.rectangle((0, y + 14, width - 1, y + 17), fill=1)
            d.line((0, y + 14, width - 1, y + 14), fill=9)
            d.line((0, y + 17, width - 1, y + 17), fill=9)
            if filled:
                d.rectangle((0, y + 15, filled - 1, y + 16), fill=5)
                d.line((0, y + 15, filled - 1, y + 15), fill=6)
            flag_color = 6 if filled == width else (7 if filled else 11)
            flag_y = y - 1
            if wave_index == 4:
                skull_flag(d, width, flag_y + 4, 4 if filled == width else 11)
            else:
                d.polygon([(width - 9, flag_y + 6), (width - 1, flag_y + 6), (width - 1, flag_y + 12),
                           (width - 9, flag_y + 11), (width - 7, flag_y + 9)], fill=flag_color, outline=9)
                d.line((width - 7, flag_y + 7, width - 2, flag_y + 7), fill=8 if filled else 14)
            pole_top = 5 if wave_index == 4 else 6
            d.line((width, flag_y + pole_top, width, y + 17), fill=13)
            d.point((width, flag_y + pole_top - 1), fill=8)
    save(wave_bars, 'wave_bar', {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    icons = canvas(16, 192)
    for frame in range(12):
        icon = canvas(16, 16)
        d = ImageDraw.Draw(icon)
        if frame == 0:
            d.line((4, 4, 12, 12), fill=11, width=3)
            d.line((12, 4, 4, 12), fill=11, width=3)
        elif frame == 1:
            d.line((3, 8, 6, 11, 13, 4), fill=6, width=3)
        elif frame == 2:
            d.line((11, 2, 5, 9), fill=10, width=2)
            d.polygon([(4, 7), (8, 10), (5, 14), (1, 14), (1, 11)], fill=13)
            d.line((9, 2, 12, 4), fill=8, width=2)
        elif frame == 3:
            icon = hourglass_icon(0)
        elif frame in (10, 11):
            icon = hourglass_icon(frame - 9)
        elif frame == 4:
            icon = sprite(7)
        elif frame == 5:
            d.rectangle((4, 2, 12, 10), fill=12)
            d.rectangle((5, 1, 11, 5), fill=13)
            d.rectangle((4, 5, 6, 6), fill=8)
            d.point((4, 6), fill=1)
            d.line((4, 9, 8, 9), fill=1)
            d.point((6, 9), fill=8)
            d.polygon([(6, 11), (10, 11), (8, 13)], fill=7)
        else:
            d.line((5, 0, 5, 12), fill=13, width=2)
            color = 6 if frame == 7 else 11
            offset = 1 if frame == 9 else 0
            d.polygon([(6, 0), (14, 1 + offset), (12, 3), (14, 6), (6, 5)], fill=color)
            if frame == 8:
                icon = canvas(16, 16)
                d = ImageDraw.Draw(icon)
                skull_flag(d, 15, 1, 11)
                d.line((15, 1, 15, 14), fill=13)
        icons.paste(icon, (0, frame * 16))
    save(icons, 'hud_icons', {'type': 'sprite', 'height': 16, 'bpp_mode': 'bpp_4'})
    clocks = canvas(16, 16 * 17)
    d = ImageDraw.Draw(clocks)
    for frame in range(17):
        y = frame * 16
        d.ellipse((3, y + 1, 12, y + 10), fill=1, outline=13)
        if frame:
            d.arc((3, y + 1, 12, y + 10), -90, -90 + frame * 360 / 16, fill=7, width=2)
        angle = frame * 2 * math.pi / 16 - math.pi / 2
        d.line((8, y + 6, 8 + round(math.cos(angle) * 3), y + 6 + round(math.sin(angle) * 3)), fill=8)
    save(clocks, 'phase_clock', {'type': 'sprite', 'height': 16, 'bpp_mode': 'bpp_4'})
    explosion = canvas(64, 64 * 3)
    for frame in range(3):
        effect = sprite(9).resize((64 - frame * 12, 64 - frame * 12), Image.Resampling.NEAREST)
        explosion.paste(effect, (frame * 6, frame * 64 + frame * 6))
    save(explosion, 'explosion', {'type': 'sprite', 'height': 64, 'bpp_mode': 'bpp_4'})
    base = lawn()
    save(base, 'lawn', {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})
    screens = {'title': title_screen(), 'paused': pause_screen(),
               'victory': result_screen(True), 'defeat': result_screen(False)}
    for name, image in screens.items():
        save(image, name, {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})
    sound('plant', [(280, .04), (420, .055)])
    sound('collect', [(880, .045), (1320, .065)])
    sound('navigate', [(600, .025)])
    sound('shot', [(440, .025), (260, .02)])
    sound('hit', [(150, .025)])
    chewing_sound()
    sound('wave_start', [(262, .08), (330, .08), (392, .14)])
    sound('wave_clear', [(659, .07), (784, .07), (1046, .13)])
    sound('mower', [(100, .06), (140, .06), (180, .12)])
    sound('error', [(150, .08)])
    sound('boom', [(90, .06), (55, .12)])
    sound('win', [(523, .09), (659, .09), (784, .09), (1046, .22)])
    sound('lose', [(330, .12), (262, .12), (165, .25)])


if __name__ == '__main__':
    main()
