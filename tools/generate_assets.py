"""Generate original palette-limited pixel art and prepare game audio.

Requires Pillow only when regenerating assets; checked-in BMP/WAV files build as-is.
"""
from pathlib import Path
import json
import math
import struct
import wave
from PIL import Image, ImageDraw
from pixel_art import character_layers, draw_garden, draw_sidewalk, seed_icon, shovel_icon, zombie_head
from generate_music import generate_music
from audio_config import OUTPUT_GAIN
from menu_art import graveyard, tombstone, wooden_sign
from zombie_effect_art import generate_zombie_effects
from asset_layout import asset_path

ROOT = Path(__file__).resolve().parents[1]
WAVE_BOUNDARIES = [56 + i * 168 // 5 for i in range(6)]
WAVE_WIDTHS = [end - start for start, end in zip(WAVE_BOUNDARIES, WAVE_BOUNDARIES[1:])]
WAVE_FRAME_OFFSETS = [0]
for width in WAVE_WIDTHS:
    WAVE_FRAME_OFFSETS.append(WAVE_FRAME_OFFSETS[-1] + width + 2)
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
    '>':'100/010/001/010/100', '<':'001/010/100/010/001',
    ',':'000/000/000/010/100', ';':'000/010/000/010/100',
    "'":'010/010/000/000/000', '"':'101/101/000/000/000',
    '(':'001/010/010/010/001', ')':'100/010/010/010/100',
    '[':'011/010/010/010/011', ']':'110/010/010/010/110',
    '_':'000/000/000/000/111', '%':'101/001/010/100/101',
    '*':'000/101/010/101/000', '#':'101/111/101/111/101',
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


def small_sprite(frame):
    """Compact projectiles, sun, mower, and blast art; also used for HUD icons."""
    image = canvas(16, 16)
    d = ImageDraw.Draw(image)
    if frame == 6:
        d.ellipse((5, 5, 10, 10), fill=2)
        d.ellipse((5, 4, 9, 8), fill=6)
        d.point((6, 5), fill=8)
    elif frame == 7:
        # Mirrored rays keep the silhouette balanced around the round disc.
        for ray in ((7, 0, 8, 2), (7, 13, 8, 15),
                    (0, 7, 2, 8), (13, 7, 15, 8)):
            d.rectangle(ray, fill=7)
        for flip_x in (False, True):
            for flip_y in (False, True):
                ray = [(2, 2), (4, 3), (3, 4)]
                d.polygon([(15 - x if flip_x else x, 15 - y if flip_y else y)
                           for x, y in ray], fill=7)
        d.ellipse((3, 3, 12, 12), fill=14)
        d.ellipse((4, 4, 11, 11), fill=7)
        d.line((5, 5, 7, 5), fill=8)
        d.point((5, 6), fill=8)
    elif frame == 8:
        # Side profile: swept steel handle, beveled red deck, and raised engine.
        # A single dark pixel either side of the shaft keeps the handle slender.
        d.line((0, 1, 3, 1), fill=1)
        d.line((2, 2, 4, 2), fill=1)
        for x, y in ((3, 3), (3, 4), (4, 5), (4, 6), (5, 7), (5, 8)):
            d.line((x - 1, y, x + 1, y), fill=1)
            d.point((x, y), fill=12)
        d.polygon([(4, 8), (11, 8), (15, 10), (14, 12), (2, 12), (1, 10)],
                  fill=9, outline=1)
        d.polygon([(4, 9), (11, 9), (13, 10), (13, 11), (3, 11)], fill=11)
        d.line((4, 9, 10, 9), fill=14)
        d.rounded_rectangle((7, 5, 12, 9), radius=1, fill=12, outline=1)
        d.line((8, 6, 10, 6), fill=13)
        d.line((9, 8, 11, 8), fill=1)
        d.point((11, 4), fill=1)
        for x in (2, 10):
            d.ellipse((x, 10, x + 4, 14), fill=1)
            d.arc((x, 10, x + 4, 14), 180, 270, fill=12)
            d.point((x + 2, 12), fill=13)
    elif frame == 9:
        d.polygon([(8, 0), (10, 4), (15, 2), (12, 7), (15, 11),
                   (10, 11), (8, 15), (5, 11), (0, 13), (3, 8), (0, 3), (5, 4)], fill=14)
        d.ellipse((3, 3, 12, 12), fill=7)
        d.ellipse((5, 5, 10, 10), fill=8)
    return image


def save(image, name, metadata):
    path = asset_path(name)
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path)
    path.with_suffix('.json').write_text(json.dumps(metadata) + '\n')


SEED_NAMES = ('sunflower', 'peashooter', 'walnut', 'cherry', 'potato_mine', 'snow_pea', 'squash', 'repeater', 'tallnut', 'chomper')
SEED_COSTS = ['50', '100', '50', '150', '25', '175', '50', '200', '125', '150']


PACKET_WIDTH = 24
PACKET_HEIGHT = 29
PACKET_PITCH = 28
PACKET_LEFT = 54
PACKET_TOP = 24
PACKET_SLOTS = 6
RECHARGE_STEPS = PACKET_HEIGHT


def snow_colors(base):
    colors = base.copy()
    colors[4:7] = [(65, 139, 189), (126, 202, 239), (214, 243, 255)]
    return colors


def squash_colors(base):
    colors = base.copy()
    colors[2] = (58, 79, 32)
    colors[4:7] = [(110, 142, 53), (161, 188, 84), (211, 226, 130)]
    return colors


def squash_packet_colors(base):
    colors = squash_colors(base)
    # Slot 2 is also the card stock: keep it identical to every other seed packet.
    colors[2] = base[2]
    return colors


def chomper_colors(base):
    colors = base.copy()
    colors[4:7] = [(76, 37, 108), (125, 60, 158), (184, 111, 205)]
    colors[10] = (210, 103, 148)
    colors[14] = (34, 20, 48)
    return colors


def seed_colors(kind, base):
    """Packet and reward palettes use the same plant-specific color rules."""
    if kind == 5: return snow_colors(base)
    if kind == 6: return squash_packet_colors(base)
    if kind == 9: return chomper_colors(base)
    return base


def plant_colors(name, base):
    if name == 'squash': return squash_colors(base)
    if name == 'chompers': return chomper_colors(base)
    return base


def apply_palette(image, colors):
    image.putpalette([v for color in colors for v in color] + [0] * (768 - len(colors) * 3))


def seed_packet(kind):
    """Shared icon-and-price packet for the bank and plant rewards."""
    image = canvas(PACKET_WIDTH, PACKET_HEIGHT)
    d = ImageDraw.Draw(image)
    d.rounded_rectangle((0, 0, PACKET_WIDTH - 1, PACKET_HEIGHT - 1), radius=1, fill=2, outline=9)
    d.rectangle((3, 2, 20, 20), fill=1, outline=3)
    icon = seed_icon(kind, canvas)
    image.paste(icon, ((PACKET_WIDTH - icon.width) // 2, 3), icon.point([0] + [255] * 255, 'L'))
    text(image, PACKET_WIDTH // 2, 22, SEED_COSTS[kind], color=7, centered=True)
    return image


def seed_layout_header():
    content = """// Generated by tools/generate_assets.py. Edit the generator to change packet geometry.
#ifndef GARDEN_SEED_LAYOUT_H
#define GARDEN_SEED_LAYOUT_H
namespace garden::seed_layout
{
inline constexpr int width = @WIDTH@;
inline constexpr int height = @HEIGHT@;
inline constexpr int pitch = @PITCH@;
inline constexpr int left = @LEFT@;
inline constexpr int center_y = @TOP@ + height / 2;
inline constexpr int slots = @SLOTS@;
inline constexpr int recharge_steps = @STEPS@;
inline constexpr int frames_per_plant = (recharge_steps + 1) * 2;
constexpr int center(int slot) { return left + width / 2 + slot * pitch; }
constexpr int frame(int remaining, int recharge, bool affordable)
{
    int revealed = (recharge - remaining) * recharge_steps / recharge;
    return (affordable ? 0 : recharge_steps + 1) + revealed;
}
}
#endif
"""
    for key, value in dict(WIDTH=PACKET_WIDTH, HEIGHT=PACKET_HEIGHT, PITCH=PACKET_PITCH,
                           LEFT=PACKET_LEFT, TOP=PACKET_TOP, SLOTS=PACKET_SLOTS, STEPS=RECHARGE_STEPS).items():
        content = content.replace('@' + key + '@', str(value))
    path = ROOT / 'include' / 'seed_layout.h'
    if not path.exists() or path.read_text() != content:
        path.write_text(content)


def reward_packet(kind):
    image = canvas(64, 64)
    packet = seed_packet(kind)
    image.paste(packet, ((64 - PACKET_WIDTH) // 2, 17))
    return image


def rally_marker(image):
    # Padding on the screen-door sheet leaves the body centered in its frame.
    origin = (image.width - 32) // 2
    # Amber eyes indicate the rally buff without adding floating HUD symbols.
    for x in (10 + origin, 17 + origin):
        for y in range(10, 15):
            for px in range(x, x + 5):
                if image.getpixel((px, y)) == 8:
                    image.putpixel((px, y), 7)
    return image


def draw_unplanted_soil(image, lanes):
    if lanes == 5: return
    d = ImageDraw.Draw(image)
    grass_top = 58 + (5 - lanes) // 2 * 19
    grass_bottom = grass_top + lanes * 19 - 1
    # Author continuous soil beds, avoiding seams where unused lane rows used to meet.
    for top, bottom in ((58, grass_top - 1), (grass_bottom + 1, 152)):
        d.rectangle((33, top, 232, bottom), fill=9)
        d.line((33, top, 33, bottom), fill=1)
        for y in range(top + 2, bottom - 1, 5):
            for x in range(37, 226, 8):
                grain = (x * 37 + y * 19 + (x * y) // 11) % 31
                px = x + grain % 3
                py = y + grain // 9
                if py + 2 > bottom: continue
                if grain < 6:
                    # Rounded earth clods: a lit shoulder and a short recessed underside.
                    d.polygon([(px, py + 1), (px + 2, py), (px + 4, py + 1),
                               (px + 3, py + 2), (px + 1, py + 2)], fill=10)
                    d.line((px + 1, py + 2, px + 3, py + 2), fill=9)
                    if grain < 2: d.point((px + 2, py + 2), fill=1)
                elif grain in (10, 18, 24):
                    d.line((px, py, px + 1, py), fill=10)
                elif grain == 30:
                    # A few muted pebbles keep the earth from becoming a repeated dash pattern.
                    d.line((px, py + 1, px + 2, py + 1), fill=1)
                    d.point((px + 1, py), fill=12)
        # Short, broken furrows add depth without a grid of obvious horizontal stripes.
        for x in range(42, 226, 29):
            y = top + 4 + (x * 7) % max(1, bottom - top - 7)
            d.line((x, y + 1, x + 2, y, x + 5, y), fill=1)
            d.line((x + 1, y + 2, x + 5, y + 1), fill=9)
    # A T. rex with a broad skull, tiny forelimbs and large folded hind legs.
    x, y = 102, 76 if lanes == 1 else 66
    def bone(points, width=1):
        d.line(points, fill=13, width=width)
    bone([(x - 31, y + 4), (x - 24, y + 3), (x - 17, y), (x - 8, y - 1),
          (x + 7, y - 1), (x + 12, y - 4)], 2)
    for offset in (-6, -2, 2, 6):
        d.arc((x + offset - 3, y - 4, x + offset + 3, y + 4), 280, 100, fill=13)
    # The weight-bearing thighs and three-toed feet distinguish it from a fish.
    bone([(x - 7, y + 1), (x - 13, y + 5), (x - 10, y + 8), (x - 3, y + 8)], 2)
    bone([(x - 3, y + 1), (x - 7, y + 5), (x - 4, y + 7), (x + 2, y + 7)])
    for toe in (-3, 0, 2):
        bone([(x + toe, y + 7), (x + toe + 1, y + 9)])
    bone([(x + 7, y), (x + 10, y + 3), (x + 13, y + 2)])
    d.point((x + 13, y + 3), fill=13)
    # Deep eye socket, blunt snout, open lower jaw and a few surviving teeth.
    d.polygon([(x + 12, y - 5), (x + 16, y - 8), (x + 27, y - 7),
               (x + 29, y - 3), (x + 25, y - 2), (x + 18, y - 3),
               (x + 16, y - 1), (x + 12, y - 2)], fill=13)
    d.rectangle((x + 15, y - 6, x + 18, y - 4), fill=9)
    d.point((x + 26, y - 5), fill=9)
    bone([(x + 15, y - 1), (x + 20, y + 1), (x + 27, y)])
    for tooth in (20, 23, 26):
        d.point((x + tooth, y - 2), fill=8)
    d.line((x + 20, y - 7, x + 25, y - 7), fill=8)
    d.line((x - 18, y + 1, x - 15, y + 1), fill=10)
    # A smaller, partially buried rib fossil remains in the lower dirt bed.
    x, y = 159, 133 if lanes == 1 else 142
    bone([(x - 15, y + 1), (x + 9, y)])
    for offset in (-10, -5, 0, 5):
        d.arc((x + offset - 3, y - 5, x + offset + 4, y + 6), 270, 90, fill=13)
        d.point((x + offset + 3, y), fill=8)
    d.polygon([(x + 10, y - 3), (x + 17, y - 3), (x + 20, y),
               (x + 17, y + 3), (x + 11, y + 3)], fill=13)
    d.rectangle((x + 13, y - 1, x + 14, y), fill=9)
    d.line((x + 12, y + 3, x + 17, y + 2), fill=10)
    # Irregular turf lips project into the soil; all playable cells remain grass.
    d.line((34, grass_top - 1, 232, grass_top - 1), fill=2)
    d.line((34, grass_bottom + 1, 232, grass_bottom + 1), fill=2)
    for x in range(35, 230, 7):
        depth = 1 + (x * 5) % 3
        shade = 3 if x % 4 else 4
        d.polygon([(x, grass_top), (x + 1, grass_top - depth - 1), (x + 3, grass_top)], fill=shade)
        d.polygon([(x, grass_bottom), (x + 2, grass_bottom + depth), (x + 4, grass_bottom)], fill=shade)
        if x % 3:
            d.point((x + 1, grass_top - 1), fill=5)
            d.point((x + 1, grass_bottom + 1), fill=5)


def lawn(show_hud=True, lanes=5):
    image = canvas(256, 256, 1)
    lawn_colors = COLORS.copy()
    lawn_colors[2:6] = [(42, 80, 42), (64, 112, 53), (80, 130, 60), (123, 166, 65)]
    if lanes < 5:
        lawn_colors[9] = (121, 81, 56)
        lawn_colors[10] = (155, 106, 70)
    image.putpalette([v for color in lawn_colors for v in color] + [0] * (768 - len(lawn_colors) * 3))
    d = ImageDraw.Draw(image)
    d.rectangle((0, 0, 239, 55), fill=0)
    if show_hud:
        d.rectangle((WAVE_BOUNDARIES[0], 14, WAVE_BOUNDARIES[-1], 17), fill=1)
        d.line((WAVE_BOUNDARIES[0], 14, WAVE_BOUNDARIES[-1], 14), fill=9)
        d.line((WAVE_BOUNDARIES[0], 17, WAVE_BOUNDARIES[-1], 17), fill=9)
        d.line((8, 21, 231, 21), fill=2)
    d.line((0, 55, 239, 55), fill=9, width=2)
    draw_garden(image)
    draw_unplanted_soil(image, lanes)
    if show_hud:
        stamp(image, 7, 3, 24)
        text(image, 21, 26, 'SUN', color=7)
    return image


def stamp(image, frame, x, y, scale=1):
    source = small_sprite(frame)
    size = 16 * scale
    fit = min(size / source.width, size / source.height)
    icon = source.resize((round(source.width * fit), round(source.height * fit)), Image.Resampling.NEAREST)
    # Transparency follows palette index zero, not its nonzero RGB luminance.
    image.paste(icon, (x + (size - icon.width) // 2, y + size - icon.height),
                icon.point([0] + [255] * 255, 'L'))


def skull_flag(draw, pole, top, color):
    """12x10 cloth keeps a comfortable margin around the six-pixel skull."""
    draw.polygon([(pole - 12, top), (pole - 1, top), (pole - 1, top + 9),
                  (pole - 12, top + 8), (pole - 10, top + 4)], fill=color, outline=9)
    draw.line((pole - 10, top + 1, pole - 2, top + 1), fill=14 if color == 11 else (8 if color == 7 else 6))
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
inline constexpr int head_y = 18;

constexpr int boundary(int wave, int total = 5) { return 56 + 168 * wave / total; }
constexpr int frame_offset(int wave, int total = 5)
{ return total == 3 ? wave * 58 : frame_offsets[wave]; }

constexpr int position(int progress, int total = 5)
{
    if(progress <= 0) { return boundaries.front(); }
    if(progress >= total * 100) { return boundaries.back(); }
    int wave = progress / 100;
    int width = boundary(wave + 1, total) - boundary(wave, total);
    return boundary(wave, total) + width * (progress % 100) / 100;
}

constexpr int display_position(int progress, int total = 5)
{ return 280 - position(progress, total); }

constexpr int fill(int wave, int progress, int total = 5)
{
    int pixels = position(progress, total) - boundary(wave, total);
    int width = boundary(wave + 1, total) - boundary(wave, total);
    return pixels < 0 ? 0 : (pixels > width ? width : pixels);
}
}
#endif
'''.replace('@BOUNDARIES@', ', '.join(map(str, WAVE_BOUNDARIES))).replace(
        '@OFFSETS@', ', '.join(map(str, WAVE_FRAME_OFFSETS)))
    path = ROOT / 'include' / 'wave_track.h'
    if not path.exists() or path.read_text() != content:
        path.write_text(content)


def menu_base(bottom=151):
    """Shared graveyard scenery with an inset headstone for readable controls."""
    image = canvas(256, 256, 1)
    graveyard(image)
    tombstone(image, (27, 8, 212, bottom))
    d = ImageDraw.Draw(image)
    d.rounded_rectangle((36, 43, 203, 137), radius=3, fill=1)
    return image


def menu_heading(image, heading):
    ImageDraw.Draw(image).rounded_rectangle((40, 18, 199, 37), radius=3, fill=1)
    text(image, 120, 24, heading, color=7, scale=2, centered=True)


def title_screen():
    image = canvas(256, 256, 3)
    graveyard(image, moon_origin=(12, 46))
    wooden_sign(image, (12, 9, 227, 39))
    text(image, 120, 19, 'PLANTS VS. ZOMBIES', color=7, scale=2, centered=True)
    tombstone(image, (130, 48, 222, 122), crown_radius=32)
    d = ImageDraw.Draw(image)
    # The selected level name and progress marker are runtime sprites.
    text(image, 176, 62, 'BACKYARD', color=1, centered=True)
    text(image, 176, 72, '10 LEVELS', color=2, centered=True)
    # Plants, the level label, and start prompt are independent runtime sprites.
    wooden_sign(image, (8, 132, 232, 155))
    text(image, 120, 136, 'UP/DOWN: SELECT   A/START: OPEN', centered=True)
    text(image, 120, 146, 'D-PAD: MOVE   A: PLANT   B: SUN', centered=True)
    return image


def level_status_icon(state):
    """Matching enamel glyphs: dark contour, green body, pale top-left highlight."""
    image = canvas(16, 16)
    d = ImageDraw.Draw(image)
    if state == 0:
        d.rounded_rectangle((5, 2, 11, 9), radius=3, fill=1)
        d.rounded_rectangle((6, 3, 10, 8), radius=2, fill=13)
        d.rectangle((7, 4, 9, 7), fill=12)
        d.rounded_rectangle((4, 7, 12, 12), radius=2, fill=12, outline=1)
        d.rectangle((5, 8, 11, 10), fill=13)
        d.line((5, 8, 6, 8), fill=8)
    elif state == 2:
        d.polygon([(2, 8), (5, 7), (7, 9), (12, 3),
                   (14, 5), (7, 12)], fill=5, outline=1)
        d.line((4, 8, 7, 10, 12, 5), fill=6)
    else:
        d.line((8, 7, 8, 12), fill=1, width=3)
        d.line((8, 7, 8, 11), fill=5)
        d.polygon([(8, 9), (3, 6), (2, 3), (6, 4), (8, 7)], fill=5, outline=1)
        d.polygon([(8, 8), (10, 3), (13, 2), (13, 5), (10, 8)], fill=5, outline=1)
        d.line((4, 5, 6, 6), fill=6)
        d.line((10, 5, 12, 3), fill=6)
    return image


def level_select_screen():
    image = canvas(256, 256, 1)
    graveyard(image)
    tombstone(image, (10, 8, 229, 153))
    ImageDraw.Draw(image).rounded_rectangle((23, 48, 216, 140), radius=4, fill=1, outline=2)
    menu_heading(image, 'BACKYARD')
    text(image, 120, 41, 'WORLD 1 - SELECT A LEVEL', color=1, centered=True)
    text(image, 120, 143, 'D-PAD: SELECT   A: PLAY   B: BACK', color=1, centered=True)
    return image


def yard_road(lanes=5):
    base = lawn(show_hud=False, lanes=lanes)
    image = canvas(512, 256, 0)
    # Share the lawn scenery and palette, with an independent scouting header.
    # Only colors absent from the lawn are repurposed for the asphalt.
    palette = base.getpalette().copy()
    for index, color in [(6, (98, 106, 108)), (11, (63, 71, 75)), (15, (158, 165, 160))]:
        assert index not in set(base.crop((0, 0, 240, 160)).get_flattened_data())
        palette[index * 3:index * 3 + 3] = color
    image.putpalette(palette)
    image.paste(base, (0, 0))
    d = ImageDraw.Draw(image)
    # The same black header continues across both views; all scenery stays below it.
    d.rectangle((240, 0, 511, 55), fill=0)
    d.line((240, 55, 511, 55), fill=9, width=2)
    # The same sidewalk starts at the lawn's edge and widens into the street view.
    draw_sidewalk(image, 263)
    d.line((264, 57, 264, 159), fill=15)
    d.line((265, 57, 265, 159), fill=6)
    d.rectangle((266, 57, 511, 159), fill=11)
    d.line((267, 57, 267, 159), fill=6)
    for y in range(58, 160, 7):
        for x in range(272 + y % 9, 512, 17):
            d.line((x, y, x + 1, y), fill=6)
    for y in range(83, 160, 29):
        d.rectangle((394, y, 396, y + 13), fill=7)
        d.line((394, y + 13, 396, y + 13), fill=10)
    # Separate portable barricades leave clear gaps between the striped boards.
    for left, right in ((278, 331), (350, 403), (422, 475)):
        for x in (left + 10, right - 10):
            d.polygon([(x, 65), (x + 4, 65), (x + 10, 81), (x + 6, 81)], fill=9, outline=1)
            d.polygon([(x, 65), (x + 3, 65), (x - 3, 81), (x - 7, 81)], fill=10, outline=1)
            d.line((x - 8, 82, x + 11, 82), fill=6)
        d.rectangle((left, 60, right, 72), fill=1)
        d.rectangle((left + 2, 61, right - 2, 70), fill=8)
        for x in range(left + 2, right - 1):
            for y in range(62, 70):
                if (x + y) % 20 < 9:
                    d.point((x, y), fill=14)
        d.line((left + 2, 71, right - 2, 71), fill=9)
        for x in (left + 4, right - 4):
            d.point((x, 64), fill=6)
            d.point((x, 68), fill=6)
    return image


def almanac_paper(image):
    # Neutral warm paper contrasts with the original metal grays and mine lamp.
    palette = image.getpalette().copy()
    palette[13 * 3:14 * 3] = (152, 145, 126)
    palette[15 * 3:16 * 3] = (168, 160, 141)
    image.putpalette(palette)
    return image


def almanac_screen():
    image = canvas(256, 256, 1)
    graveyard(image)
    d = ImageDraw.Draw(image)
    d.rounded_rectangle((5, 26, 234, 139), radius=4, fill=9, outline=1)
    d.rounded_rectangle((8, 28, 231, 136), radius=3, fill=10, outline=14)
    d.rounded_rectangle((12, 31, 227, 133), radius=2, fill=13, outline=8)
    for i in range(6):
        left, top = 24 + i % 3 * 66, 43 + i // 3 * 46
        d.rounded_rectangle((left, top, left + 60, top + 42), radius=3, fill=15, outline=12)
        d.line((left + 3, top + 1, left + 57, top + 1), fill=13)
    wooden_sign(image, (37, 3, 202, 24))
    text(image, 120, 9, 'ALMANAC', color=7, scale=2, centered=True)
    wooden_sign(image, (8, 141, 231, 159))
    text(image, 120, 148, 'L/R: PAGE  D-PAD: MOVE  A: INFO  B: BACK', color=8, centered=True)
    return almanac_paper(image)


def almanac_stats():
    icons = canvas(16, 64)
    sun = small_sprite(7)
    icons.paste(sun, (0, 0))
    d = ImageDraw.Draw(icons)
    # A full heart represents all durability, including a zombie's protective gear.
    d.polygon([(2, 20), (4, 18), (6, 18), (8, 20), (10, 18), (12, 18),
               (14, 20), (14, 23), (8, 29), (2, 23)], fill=11, outline=1)
    d.line((4, 20, 6, 20), fill=8)
    d.line((4, 21, 4, 22), fill=14)
    # A clock with a returning arrow is the seed packet's recharge time.
    d.ellipse((2, 34, 13, 45), fill=13, outline=1)
    d.ellipse((4, 36, 11, 43), fill=8)
    d.line((8, 37, 8, 40, 10, 41), fill=1)
    d.arc((1, 33, 14, 46), 15, 270, fill=10, width=1)
    d.polygon([(12, 35), (15, 35), (15, 38)], fill=7, outline=1)
    # Rounded cranium, separate sockets, cheekbones and a short three-tooth jaw.
    skull = canvas(16, 16)
    sd = ImageDraw.Draw(skull)
    sd.polygon([(5, 2), (10, 2), (12, 4), (13, 7), (12, 10), (10, 11),
                (10, 13), (5, 13), (5, 11), (3, 10), (2, 7), (3, 4)], fill=8, outline=1)
    sd.polygon([(10, 4), (12, 5), (12, 9), (10, 10), (9, 12), (9, 10), (10, 8)], fill=13)
    sd.ellipse((4, 5, 6, 8), fill=1)
    sd.ellipse((9, 5, 11, 8), fill=1)
    sd.polygon([(7, 9), (8, 9), (8, 10)], fill=1)
    sd.line((7, 12, 7, 13), fill=1)
    sd.line((9, 12, 9, 13), fill=1)
    icons.paste(skull, (0, 48))
    return icons


def almanac_entry_panel():
    image = canvas(256, 256, 0)
    d = ImageDraw.Draw(image)
    d.rounded_rectangle((23, 23, 220, 137), radius=5, fill=1)
    d.rounded_rectangle((20, 20, 217, 134), radius=5, fill=9, outline=1, width=2)
    d.rounded_rectangle((23, 23, 214, 131), radius=3, fill=13, outline=8)
    d.line((31, 41, 206, 41), fill=12)
    d.line((31, 109, 206, 109), fill=12)
    wooden_sign(image, (8, 141, 231, 159))
    text(image, 120, 148, 'UP/DOWN: SCROLL  B: BACK', color=8, centered=True)
    return almanac_paper(image)


def pause_screen():
    image = menu_base(bottom=156)
    menu_heading(image, 'PAUSED')
    d = ImageDraw.Draw(image)
    d.line((52, 92, 187, 92), fill=2)
    text(image, 120, 140, 'UP/DOWN: SELECT  LEFT/RIGHT: VOLUME', color=1, centered=True)
    text(image, 120, 147, 'A: SELECT   B/START: RESUME', color=1, centered=True)
    return image


def reward_panel():
    image = canvas(256, 256, 0)
    # Keep the HUD and lawn visible; only the information below the packet is boxed.
    wooden_sign(image, (34, 120, 205, 151))
    return image


def completion_trophy():
    image = canvas(64, 64)
    d = ImageDraw.Draw(image)
    # Open handles, a curved golden cup, and a solid stepped plinth.
    d.rounded_rectangle((16, 19, 47, 32), radius=5, fill=14, outline=1)
    d.rounded_rectangle((19, 22, 44, 28), radius=3, fill=0, outline=10)
    d.polygon([(22, 17), (41, 17), (40, 29), (37, 34), (33, 36),
               (33, 39), (39, 40), (39, 42), (24, 42), (24, 40),
               (30, 39), (30, 36), (26, 34), (23, 29)], fill=7, outline=1)
    d.polygon([(35, 19), (39, 19), (38, 29), (35, 33), (32, 34)], fill=14)
    d.line((25, 20, 26, 28, 29, 31), fill=8, width=2)
    d.rectangle((21, 16, 42, 18), fill=7, outline=1)
    d.line((23, 17, 40, 17), fill=8)
    d.rectangle((23, 42, 40, 46), fill=9, outline=1)
    d.line((25, 43, 38, 43), fill=10)
    d.rectangle((29, 43, 34, 45), fill=7)
    d.line((44, 13, 44, 17), fill=8)
    d.line((42, 15, 46, 15), fill=8)
    return image


def loss_notice(prompt=False):
    image = canvas(256, 256, 0)
    # Draw an outline in the same native pixel font, over the actual lost lawn.
    for label, y in [('THE ZOMBIES', 58), ('ATE YOUR BRAINS!', 81)]:
        for dx, dy in [(-2, 0), (2, 0), (0, -2), (0, 2), (-1, -1), (1, 1)]:
            text(image, 120 + dx, y + dy, label, color=1, scale=3, centered=True)
        text(image, 120, y, label, color=11, scale=3, centered=True)
    if prompt:
        wooden_sign(image, (43, 119, 196, 148))
        text(image, 120, 126, 'A/START: RETRY', color=8, centered=True)
        text(image, 120, 138, 'B: LEVELS', color=8, centered=True)
    return image


def sound(name, notes, gain=1.0):
    rate = 16000
    data = []
    for frequency, seconds in notes:
        length = int(rate * seconds)
        for i in range(length):
            envelope = min(1, i / 70) * (1 - i / length) ** 2
            sample = math.sin(2 * math.pi * frequency * i / rate)
            data.append(struct.pack('<h', int(sample * envelope * 14000 * OUTPUT_GAIN * gain)))
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
        data.append(struct.pack('<h', int(sample * 7700 * OUTPUT_GAIN)))
    with wave.open(str(ROOT / 'audio' / 'chew.wav'), 'wb') as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(rate)
        output.writeframes(b''.join(data))


# Character bodies and shadow masks are authored separately, then packed into one
# sprite to stay within GBA OAM limits. Palette entry 15 belongs only to shadows.
SHADOW_COLOR = (43, 78, 46)
PLANT_FRAMES = (0, 1, 2, 3, 10, 11, 12, 13, 20, 21, 22, 23, 24, 25, 26, 27, 28)
ZOMBIE_FRAMES = (4, 5, 14, 15, 16, 17, 18, 19)


def character_palette(colors, clothing_slot=None):
    colors = list(colors)
    if clothing_slot is not None:
        colors[clothing_slot] = colors[15]
    colors[15] = SHADOW_COLOR
    return colors


def character_sprite(frame, colors, clothing_slot=None, buffed=False):
    body, mask = character_layers(frame, canvas)
    if buffed:
        rally_marker(body)
    if clothing_slot is not None:
        assert clothing_slot not in set(body.get_flattened_data()), (frame, clothing_slot)
        body = body.point([clothing_slot if index == 15 else index for index in range(256)])
    else:
        assert 15 not in set(body.get_flattened_data()), frame
    mask.paste(body, (0, 0), body.point([0] + [255] * 255, 'L'))
    apply_palette(mask, character_palette(colors, clothing_slot))
    return mask


def defeat_sounds():
    """Prepare checked-in recordings; provenance lives beside the source WAVs."""
    durations = {}
    for name, peak_level in [('record_scratch', 16000), ('scream', 16500)]:
        with wave.open(str(ROOT / 'tools' / 'audio_sources' / (name + '.wav')), 'rb') as source:
            assert source.getnchannels() == 1 and source.getsampwidth() == 2
            rate = source.getframerate()
            assert rate == 16000
            samples = [sample[0] for sample in struct.iter_unpack('<h', source.readframes(source.getnframes()))]
        durations[name] = len(samples) / rate
        dc = sum(samples) / len(samples)
        fade_in = round(rate * .004)
        fade_out = round(rate * .012)
        prepared = [(value - dc) * min(1, i / fade_in, (len(samples) - 1 - i) / fade_out)
                    for i, value in enumerate(samples)]
        peak = max(abs(value) for value in prepared)
        gain = min(28000, peak_level * OUTPUT_GAIN) / max(1, peak)
        with wave.open(str(ROOT / 'audio' / (name + '.wav')), 'wb') as output:
            output.setnchannels(1)
            output.setsampwidth(2)
            output.setframerate(rate)
            output.writeframes(b''.join(struct.pack('<h', round(value * gain)) for value in prepared))

    scream_frame = math.ceil(durations['record_scratch'] * 60) + 1
    end_frame = scream_frame + math.ceil(durations['scream'] * 60)
    ready_frame = max(180, math.ceil((end_frame + 12) / 10) * 10)
    timing = ('#ifndef GARDEN_DEFEAT_AUDIO_H\n#define GARDEN_DEFEAT_AUDIO_H\n'
              '// Generated from the recorded clips; keep the full scream before retry is enabled.\n'
              f'#define PVZ_DEFEAT_SCREAM_FRAME {scream_frame}\n'
              f'#define PVZ_DEFEAT_READY_FRAME {ready_frame}\n'
              '#ifdef __cplusplus\nnamespace garden::defeat_audio\n{\n'
              'inline constexpr int scream_frame = PVZ_DEFEAT_SCREAM_FRAME;\n'
              'inline constexpr int ready_frame = PVZ_DEFEAT_READY_FRAME;\n'
              '}\n#endif\n#endif\n')
    timing_path = ROOT / 'include' / 'defeat_audio.h'
    if not timing_path.exists() or timing_path.read_text() != timing:
        timing_path.write_text(timing)


def emerging_zombies():
    atlas = canvas(32, 32 * 12)
    # The dirt opens, then the head, shoulders and legs pull free.
    for buffed in (False, True):
        for stage, drop in enumerate((21, 16, 11, 7, 3, 0)):
            source = character_sprite(4 if stage % 2 == 0 else 14, COLORS, 3, buffed)
            frame = canvas(32, 32)
            frame.putpalette(source.getpalette())
            d = ImageDraw.Draw(frame)
            d.ellipse((6, 24, 26, 28), fill=15)
            d.ellipse((8, 24, 24, 27), fill=9)
            if stage:
                # Clip at the ground rather than sliding an entire sprite above the lawn.
                body = source.point([0 if index == 15 else index for index in range(256)])
                visible = body.crop((0, 0, 32, 27 - drop))
                frame.paste(visible, (0, drop), visible.point([0] + [255] * 255, 'L'))
            if stage < 5:
                d.point((10 - stage % 2, 28), fill=10)
                d.point((25, 25 + stage % 2), fill=10)
            else:
                # Last pose matches the walking silhouette and shadow for a clean handoff.
                frame = source
            atlas.paste(frame, (0, (int(buffed) * 6 + stage) * 32))
    apply_palette(atlas, character_palette(COLORS, 3))
    return atlas


def lawn_tombstones():
    atlas = canvas(32, 32 * 8)
    for stage in range(3):
        stone = canvas(32, 32)
        d = ImageDraw.Draw(stone)
        d.ellipse((6, 24, 26, 28), fill=15)
        d.polygon([(7, 25), (7, 13), (9, 9), (12, 7), (20, 7), (24, 10),
                   (25, 14), (25, 25)], fill=12, outline=1)
        d.line((8, 14, 8, 13, 10, 10, 13, 8, 20, 8), fill=13)
        d.line((24, 14, 24, 24, 9, 24), fill=2)
        # Shallow carved cross, with a light edge rather than a heavy inset box.
        d.line((16, 12, 16, 20), fill=2)
        d.line((12, 15, 20, 15), fill=2)
        d.line((17, 13, 17, 20), fill=13)
        d.line((13, 16, 20, 16), fill=13)
        d.point((10, 22), fill=13)
        d.line((18, 24, 21, 24), fill=3)
        if stage:
            d.line((20, 9, 18, 12, 20, 14, 17, 17, 18, 19), fill=1)
            d.point((19, 12), fill=13)
        if stage == 2:
            d.line((9, 19, 12, 17, 14, 20, 13, 23), fill=1)
            d.line((22, 21, 20, 23, 20, 25), fill=1)
            d.point((24, 12), fill=0)
        atlas.paste(stone, (0, stage * 32))
    cracked = atlas.crop((0, 64, 32, 96))
    for stage in range(5):
        rubble = canvas(32, 32)
        rd = ImageDraw.Draw(rubble)
        rd.ellipse((6, 25, 26, 28), fill=15)
        if stage < 3:
            for left, top, right, bottom in ((7, 7, 16, 16), (16, 7, 26, 17), (7, 16, 16, 26), (16, 17, 26, 26)):
                chunk = cracked.crop((left, top, right, bottom))
                dx = (-1 if left < 16 else 1) * stage
                dy = min(27 - bottom, stage * 3)
                rubble.paste(chunk, (left + dx, top + max(0, dy)), chunk.point([0] + [255] * 255, 'L'))
            rd.line((16, 9, 14, 16, 17, 21, 15, 25), fill=1)
        else:
            for x, y, size in ((8, 25, 4), (14, 24, 5), (21, 25, 4)):
                rd.polygon([(x, y + 2), (x + 1, y - 2), (x + size, y - 1), (x + size, y + 2)], fill=12, outline=1)
                rd.point((x + 2, y - 1), fill=13)
            if stage == 4:
                for y in range(24, 29):
                    for x in range(6, 27):
                        if (x + y) % 2: rubble.putpixel((x, y), 0)
        atlas.paste(rubble, (0, (3 + stage) * 32))
    # Match the independently colored character ground shadows.
    colors = list(COLORS)
    colors[15] = SHADOW_COLOR
    apply_palette(atlas, colors)
    return atlas


def main():
    save(emerging_zombies(), 'emerging_zombies', {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    save(lawn_tombstones(), 'tombstones', {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    (ROOT / 'graphics').mkdir(exist_ok=True)
    (ROOT / 'audio').mkdir(exist_ok=True)
    wave_layout_header()
    seed_layout_header()
    generate_music(ROOT / 'audio' / 'garden_theme.mod')
    title_plants = canvas(64, 64 * 4)
    for index, frame in enumerate((0, 12, 1, 20)):
        title_plants.paste(character_sprite(frame, COLORS).resize((64, 64), Image.Resampling.NEAREST), (0, index * 64))
    apply_palette(title_plants, character_palette(COLORS))
    save(title_plants, 'title_plants', {'type': 'sprite', 'height': 64, 'bpp_mode': 'bpp_4'})
    level_tiles = canvas(32, 32 * 30)
    d = ImageDraw.Draw(level_tiles)
    tile_bounds = (1, 1, 30, 30)
    tile_radius = 8
    for state in range(3):
        for number in range(10):
            y = (state * 10 + number) * 32
            d.rounded_rectangle((tile_bounds[0], y + tile_bounds[1], tile_bounds[2], y + tile_bounds[3]),
                                radius=tile_radius, fill=12, outline=1)
            d.rounded_rectangle((2, y + 2, 29, y + 29), radius=7, outline=13)
            d.line((6, y + 29, 25, y + 29), fill=2)
            text(level_tiles, 16, y + 6, str(number + 1), color=1, scale=2, centered=True)
            badge = level_status_icon(state)
            level_tiles.paste(badge, (8, y + 15), badge.point([0] + [255] * 255, 'L'))
    save(level_tiles, 'level_tiles', {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    level_cursor = canvas(32, 32)
    cursor_draw = ImageDraw.Draw(level_cursor)
    cursor_draw.rounded_rectangle(tile_bounds, radius=tile_radius, outline=7)
    cursor_draw.rounded_rectangle((2, 2, 29, 29), radius=7, outline=7)
    # Shade only existing contour pixels, preserving the exact stone silhouette.
    for cy in range(32):
        for cx in range(32):
            if level_cursor.getpixel((cx, cy)):
                level_cursor.putpixel((cx, cy), 8 if cx + cy < 25 else (14 if cx + cy > 38 else 7))
    save(level_cursor, 'level_cursor', {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    sliders = canvas(64, 32 * 11)
    d = ImageDraw.Draw(sliders)
    for level in range(11):
        y = level * 32
        d.rounded_rectangle((2, y + 13, 61, y + 18), radius=2, fill=1, outline=12)
        if level:
            d.rectangle((4, y + 15, 4 + level * 5, y + 16), fill=5)
        thumb = 4 + level * 5
        d.rounded_rectangle((thumb, y + 11, thumb + 5, y + 20), radius=1, fill=13, outline=12)
        d.line((thumb + 2, y + 13, thumb + 2, y + 17), fill=8)
    save(sliders, 'volume_slider', {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    field_items = canvas(32, 32 * 3)
    for index, frame in enumerate((6, 7, 8)):
        field_items.paste(small_sprite(frame), (8, index * 32 + 8))
    save(field_items, 'field_items', {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    # Each sheet is one character, with an explicit pose order down its column.
    plant_sheets = {'sunflowers': (0, 12), 'peashooters': (1, 13, 20),
                    'wallnuts': (2, 10, 11, 21, 22, 23), 'cherries': (3, 24),
                    'potato_mines': (25, 26, 27, 28)}
    for name, frames in plant_sheets.items():
        atlas = canvas(32, 32 * len(frames))
        for index, frame in enumerate(frames):
            atlas.paste(character_sprite(frame, COLORS), (0, index * 32))
        apply_palette(atlas, character_palette(COLORS))
        save(atlas, name, {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    for name, frames in (('normal_zombies', (4, 14, 16, 18)),
                         ('cone_zombies', (5, 15, 17, 19))):
        atlas = canvas(32, 32 * 8)
        for index in range(8):
            atlas.paste(character_sprite(frames[index % 4], COLORS, 3, index >= 4), (0, index * 32))
        apply_palette(atlas, character_palette(COLORS, 3))
        save(atlas, name, {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    # Keep the masks available as separate art layers for future shadow edits.
    for name, frames in [('plant', PLANT_FRAMES), ('zombie', ZOMBIE_FRAMES)]:
        masks = canvas(32, 32 * len(frames))
        for index, frame in enumerate(frames):
            _, mask = character_layers(frame, canvas)
            masks.paste(mask, (0, index * 32))
        apply_palette(masks, character_palette(COLORS))
        masks.save(ROOT / 'tools' / 'art' / (name + '_shadow_masks.png'))
    enemy_atlases = [('newspaper_zombies', 29, 8, True), ('flag_zombies', 37, 16, False),
                     ('bucket_zombies', 69, 8, True), ('screen_door_zombies', 500, 12, True)]
    for name, first, count, buffed in enemy_atlases:
        colors = COLORS.copy()
        slot = 3
        if name in ('bucket_zombies', 'screen_door_zombies'):
            # Slot 2 is unused by the bucket artwork; reserve it for a softer
            # mesh steel so both armored zombies can still share one palette.
            colors[2] = (163, 180, 195)
            colors[3:7] = [(61, 72, 80), (115, 130, 143), (185, 198, 208), (226, 232, 232)]
            slot = 14
        if name == 'newspaper_zombies':
            with Image.open(Path(__file__).parent / 'art' / 'runner_base.png') as base:
                palette = base.getpalette()
                colors = [tuple(palette[i * 3:i * 3 + 3]) for i in range(16)]
        width = 64 if name == 'screen_door_zombies' else 32
        enemy_atlas = canvas(width, 32 * count * (2 if buffed else 1))
        for index in range(count * (2 if buffed else 1)):
            image = character_sprite(first + index % count, colors, slot, buffed and index >= count)
            enemy_atlas.paste(image, (0, index * 32))
        apply_palette(enemy_atlas, character_palette(colors, slot))
        save(enemy_atlas, name, {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    for name, first, count, buffed, slot in (('squash', 260, 5, False, None),
                                             ('repeaters', 270, 6, False, None),
                                             ('tallnuts', 280, 6, False, None),
                                             ('chompers', 400, 11, False, None),
                                             ('vaulting_zombies', 300, 12, True, 3)):
        atlas = canvas(32, 32 * count * (2 if buffed else 1))
        for index in range(count * (2 if buffed else 1)):
            atlas.paste(character_sprite(first + index % count, plant_colors(name, COLORS), slot, buffed and index >= count), (0, index * 32))
        apply_palette(atlas, character_palette(plant_colors(name, COLORS), slot))
        save(atlas, name, {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    # A top tray holds the selected bank; the complete roster remains in the grid below.
    selection = canvas(256, 256)
    d = ImageDraw.Draw(selection)
    d.rectangle((4, 1, 235, 38), fill=9, outline=1)
    d.rectangle((6, 3, 233, 36), fill=10)
    for slot in range(6):
        x = 20 + slot * 31
        left = x - PACKET_WIDTH // 2
        top = 20 - PACKET_HEIGHT // 2
        d.rounded_rectangle((left, top, left + PACKET_WIDTH - 1, top + PACKET_HEIGHT - 1),
                            radius=1, fill=12, outline=13)
    d.rectangle((6, 42, 158, 148), fill=9, outline=1)
    d.rectangle((8, 44, 156, 146), fill=10)
    d.line((9, 55, 155, 55), fill=9)
    for index in range(10):
        x = 22 + index % 5 * 30
        y = 78 + index // 5 * 48
        left, top = x - PACKET_WIDTH // 2, y - PACKET_HEIGHT // 2
        d.rounded_rectangle((left, top, left + PACKET_WIDTH - 1, top + PACKET_HEIGHT - 1),
                            radius=1, fill=12, outline=13)
    selection_colors = COLORS.copy()
    selection_colors[12:14] = [(76, 83, 79), (139, 146, 137)]
    apply_palette(selection, selection_colors)
    save(selection, 'seed_select', {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})
    football = canvas(32, 32 * 36)
    for index in range(36):
        football.paste(character_sprite(200 + index % 18, COLORS, 3, index >= 18), (0, index * 32))
    apply_palette(football, character_palette(COLORS, 3))
    save(football, 'football_zombies', {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    generate_zombie_effects(save)
    snow = canvas(32, 32 * 4)
    for i in range(4):
        snow.paste(character_sprite(101 + i, snow_colors(COLORS)), (0, i * 32))
    apply_palette(snow, character_palette(snow_colors(COLORS)))
    save(snow, 'snow_peashooters', {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    flash = canvas(16, 16)
    red = []
    for r, g, b in COLORS:
        brightness = (r + g + b) / (3 * 255)
        red.append((round(160 + brightness * 95), round(12 + brightness * 64), round(8 + brightness * 32)))
    red[1] = COLORS[1]
    red[15] = SHADOW_COLOR
    apply_palette(flash, red)
    save(flash, 'damage_flash', {'type': 'sprite', 'height': 16, 'bpp_mode': 'bpp_4'})
    for name, color in [('cursor', 8), ('selection', 7)]:
        image = canvas(32, 128 if name == 'cursor' else 96)
        d = ImageDraw.Draw(image)
        if name == 'cursor':
            for frame, tint in enumerate([8, 11, 7, 6]):
                for x, y, sx, sy in [(5, 7, 1, 1), (26, 7, -1, 1), (5, 24, 1, -1), (26, 24, -1, -1)]:
                    y += frame * 32
                    d.line((x, y + sy * 5, x, y, x + sx * 5, y), fill=tint, width=2)
        else:
            for frame, tint in enumerate([7, 11, 8]):
                d.rounded_rectangle((4, 2 + frame * 32, 27, 30 + frame * 32), radius=1, outline=tint)
        save(image, name, {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    # Reveal the packet from bottom to top, keeping its border continuous.
    frames_per_plant = (RECHARGE_STEPS + 1) * 2
    packet_colors = COLORS.copy()
    packet_colors[12] = (65, 69, 68)
    packet_colors[15] = (139, 144, 138)
    grey_map = [0, 1, 12, 12, 12, 15, 13, 13, 13, 1, 15, 15, 15, 15, 15, 15]
    dark_map = {0: 0, 1: 1, 12: 1, 13: 15, 15: 12}
    for kind, name in enumerate(SEED_NAMES):
        packets = canvas(32, 32 * frames_per_plant)
        apply_palette(packets, seed_colors(kind, packet_colors))
        packet = seed_packet(kind)
        grey = packet.point(grey_map + [0] * (256 - len(grey_map)))
        dark = grey.point([dark_map.get(i, 12) for i in range(256)])
        for affordable in (True, False):
            ready = packet if affordable else grey
            for step in range(RECHARGE_STEPS + 1):
                image = dark.copy()
                reveal = PACKET_HEIGHT * step // RECHARGE_STEPS
                top = PACKET_HEIGHT - reveal
                image.paste(ready.crop((0, top, PACKET_WIDTH, PACKET_HEIGHT)), (0, top))
                if not affordable:
                    text(image, PACKET_WIDTH // 2, 22, SEED_COSTS[kind], color=11, centered=True)
                ImageDraw.Draw(image).rounded_rectangle((0, 0, PACKET_WIDTH - 1, PACKET_HEIGHT - 1),
                                                        radius=1, outline=9 if affordable else 15)
                frame = (0 if affordable else RECHARGE_STEPS + 1) + step
                packets.paste(image, (4, frame * 32 + 2))
        save(packets, name + '_seed_packet', {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    for total, name in ((5, 'wave_bar'), (3, 'wave_bar_short')):
        widths = [168 * (i + 1) // total - 168 * i // total for i in range(total)]
        frames_per_state = 168 + total * 2
        wave_bars = canvas(64, 32 * frames_per_state * 3)
        d = ImageDraw.Draw(wave_bars)
        # Red (queued), yellow (entered), and green (defeated) have identical fill geometry.
        for state in range(3):
            for wave_index, width in enumerate(widths):
                for filled in range(-1, width + 1):
                    frame = state * frames_per_state + sum(widths[:wave_index]) + wave_index * 2 + filled + 1
                    y = frame * 32
                    d.rectangle((0, y + 14, width - 1, y + 17), fill=1)
                    d.line((0, y + 14, width - 1, y + 14), fill=9)
                    d.line((0, y + 17, width - 1, y + 17), fill=9)
                    if filled > 0:
                        d.rectangle((0, y + 15, filled - 1, y + 16), fill=5)
                        d.line((0, y + 15, filled - 1, y + 15), fill=6)
                    flag_y = y - 1
                    if wave_index == total - 1:
                        skull_flag(d, width, flag_y + 4, (11, 7, 4)[state])
                    else:
                        d.polygon([(width - 9, flag_y + 6), (width - 1, flag_y + 6), (width - 1, flag_y + 12),
                                   (width - 9, flag_y + 11), (width - 7, flag_y + 9)], fill=(11, 7, 6)[state], outline=9)
                        d.line((width - 7, flag_y + 7, width - 2, flag_y + 7), fill=14 if state == 0 else 8)
                    pole_top = 5 if wave_index == total - 1 else 6
                    d.line((width, flag_y + pole_top, width, y + 17), fill=13)
                    d.point((width, flag_y + pole_top - 1), fill=8)
        save(wave_bars.transpose(Image.Transpose.FLIP_LEFT_RIGHT), name, {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    icons = canvas(16, 16 * 14)
    for frame in range(14):
        icon = canvas(16, 16)
        d = ImageDraw.Draw(icon)
        if frame == 0:
            d.line((4, 4, 12, 12), fill=11, width=3)
            d.line((12, 4, 4, 12), fill=11, width=3)
        elif frame == 1:
            d.line((3, 8, 6, 11, 13, 4), fill=6, width=3)
        elif frame == 2:
            icon = shovel_icon(canvas)
        elif frame == 3:
            icon = hourglass_icon(0)
        elif frame in (10, 11):
            icon = hourglass_icon(frame - 9)
        elif frame == 4:
            icon = small_sprite(7)
        elif frame in (12, 13):
            points = [(10, 4), (6, 7), (10, 10)]
            if frame == 13: points = [(15 - x, y) for x, y in points]
            d.polygon(points, fill=7)
        elif frame == 5:
            head = canvas(32, 32)
            zombie_head(ImageDraw.Draw(head))
            icon.paste(head.crop((9, 7, 23, 19)), (1, 0))
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
        effect = small_sprite(9).resize((64 - frame * 12, 64 - frame * 12), Image.Resampling.NEAREST)
        explosion.paste(effect, (frame * 6, frame * 64 + frame * 6))
    save(explosion, 'explosion', {'type': 'sprite', 'height': 64, 'bpp_mode': 'bpp_4'})
    # Reward panels preserve the lawn underneath their transparent pixels.
    reward = reward_panel()
    save(completion_trophy(), 'completion_trophy', {'type': 'sprite', 'height': 64, 'bpp_mode': 'bpp_4'})
    for name, prompt in [('loss_message', False), ('loss_prompt', True)]:
        save(loss_notice(prompt), name, {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})
    # Menus share the native 3x5 lettering used in the background artwork.
    font = canvas(8, 8 * 94)
    for code in range(33, 127):
        char = chr(code)
        glyph = char if char.upper() in FONT_ROWS else '?'
        text(font, 0, (code - 33) * 8 + 1, glyph, color=8)
    save(font, 'menu_font', {'type': 'sprite', 'height': 8, 'bpp_mode': 'bpp_4'})
    save(reward, 'reward_panel', {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})
    shadow = canvas(32, 16)
    ImageDraw.Draw(shadow).ellipse((5, 6, 26, 10), fill=2)
    save(shadow, 'reward_shadow', {'type': 'sprite', 'height': 16, 'bpp_mode': 'bpp_4'})
    for kind, name in enumerate(SEED_NAMES):
        card = reward_packet(kind)
        apply_palette(card, seed_colors(kind, COLORS))
        save(card, name + '_reward_packet', {'type': 'sprite', 'height': 64, 'bpp_mode': 'bpp_4'})
    base = lawn()
    save(base, 'lawn', {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})
    save(yard_road(), 'yard_road', {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})
    # Two native 64×32 sprites preserve the heading without duplicating the scene.
    heading = canvas(128, 32)
    text(heading, 64, 8, 'INCOMING ZOMBIES', color=7, scale=2, centered=True)
    heading_tiles = canvas(64, 64)
    for half in range(2):
        heading_tiles.paste(heading.crop((half * 64, 0, (half + 1) * 64, 32)), (0, half * 32))
    save(heading_tiles, 'scouting_heading', {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    for lanes in (1, 3):
        save(lawn(lanes=lanes), f'lawn_{lanes}', {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})
        save(yard_road(lanes), f'yard_road_{lanes}', {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})
    # Framed inset track and a shaded thumb; each size reflects visible content.
    scrollbar = canvas(32, 64 * 4 * 25)
    for size in range(4):
        thumb = (10, 16, 22, 28)[size]
        travel = 42 - thumb
        for position in range(25):
            d = ImageDraw.Draw(scrollbar)
            y = (size * 25 + position) * 64
            d.rounded_rectangle((13, y + 10, 18, y + 53), radius=1, fill=9, outline=1)
            d.line((14, y + 11, 14, y + 52), fill=10)
            top = y + 11 + position * travel // 24
            d.rectangle((14, top, 17, top + thumb - 1), fill=7)
            d.line((14, top, 17, top), fill=8)
            d.line((17, top + 1, 17, top + thumb - 1), fill=10)
            d.line((15, top + thumb // 2, 16, top + thumb // 2), fill=9)
    save(scrollbar, 'almanac_scrollbar', {'type': 'sprite', 'height': 64, 'bpp_mode': 'bpp_4'})
    header = canvas(256, 256)
    ImageDraw.Draw(header).rectangle((0, 0, 239, 21), fill=15)
    # Palette index zero is transparent; the header needs an opaque near-black.
    palette = header.getpalette()
    palette[45:48] = list(COLORS[0])
    header.putpalette(palette)
    save(header, 'tutorial_header', {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})
    # Preserve every grass blade and edge pixel in each full 22x19 lawn cell.
    # Cell frames follow Game's row-major order; the last two frame collectible sun.
    target = canvas(32, 32 * (5 * 9 + 2))
    native_lawn = lawn(show_hud=False)
    for row in range(5):
        for column in range(9):
            x, y = 34 + column * 22, 58 + row * 19
            tile = native_lawn.crop((x, y, x + 22, y + 19))
            target.paste(tile, (5, (row * 9 + column) * 32 + 7))
    for pose in range(2):
        d = ImageDraw.Draw(target)
        top = (5 * 9 + pose) * 32
        color = 7 if pose else 8
        for x, direction in ((5, 1), (26, -1)):
            for y, vertical in ((5, 1), (26, -1)):
                d.line((x + direction * 4, top + y, x, top + y, x, top + y + vertical * 4), fill=1, width=3)
                d.line((x + direction * 4, top + y, x, top + y, x, top + y + vertical * 4), fill=color)
    target_colors = COLORS.copy()
    target_colors[2:7] = [(132, 100, 27), (184, 139, 37), (221, 177, 57),
                          (247, 214, 109), (255, 232, 150)]
    apply_palette(target, target_colors)
    save(target, 'lesson_target', {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    # Four hand-drawn poses keep the roots planted while the stalk and leaves sag.
    wilted = canvas(32, 32 * 4)
    for pose, (lean, droop, leaf_drop) in enumerate(((0, 0, 0), (-1, 1, 1), (-1, 2, 1), (0, 1, 0))):
        plant = canvas(32, 32)
        d = ImageDraw.Draw(plant)
        d.ellipse((8, 24, 24, 26), fill=15)
        d.line((16, 23, 17, 18, 15 + lean, 13 + droop,
                12 + lean, 10 + droop, 9 + lean, 12 + droop), fill=1, width=3)
        d.line((16, 22, 16, 18, 14 + lean, 13 + droop,
                12 + lean, 11 + droop, 10 + lean, 12 + droop), fill=4)
        d.line((16, 19, 16, 22), fill=5)
        d.polygon([(17, 18), (21, 18), (24, 21 + leaf_drop),
                   (24, 23 + leaf_drop), (21, 22 + leaf_drop), (18, 22)], fill=4, outline=1)
        d.line((19, 20, 22, 21 + leaf_drop), fill=5)
        d.polygon([(15, 21), (12, 19 + leaf_drop), (9, 20 + leaf_drop),
                   (7, 23 + leaf_drop), (11, 23 + leaf_drop), (15, 23)], fill=4, outline=1)
        d.line((10, 21 + leaf_drop, 13, 22), fill=5)
        # A folded leaf-head with heavy eyelids and a small downturned mouth.
        def point(x, y): return x + lean, y + droop
        d.ellipse((*point(7, 11), *point(18, 21)), fill=5, outline=1)
        d.arc((*point(8, 12), *point(17, 20)), 185, 285, fill=6)
        d.arc((*point(9, 13), *point(17, 20)), 10, 80, fill=4)
        if pose == 3:
            d.line((point(9, 16), point(11, 17)), fill=1)
            d.line((point(14, 17), point(16, 16)), fill=1)
        else:
            for x in (9, 14):
                d.rectangle((*point(x, 15), *point(x + 2, 17)), fill=13)
                d.line((point(x, 15), point(x + 2, 15)), fill=4)
                d.point(point(x + 1, 17), fill=1)
            d.line((point(9, 14), point(11, 13)), fill=3)
            d.line((point(14, 13), point(16, 14)), fill=3)
        d.line((point(11, 20), point(13, 19), point(15, 20)), fill=1)
        wilted.paste(plant, (0, pose * 32))
    palette = wilted.getpalette()
    palette[45:48] = (23, 42, 32)
    wilted.putpalette(palette)
    save(wilted, 'wilted_plant', {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    save(almanac_stats(), 'almanac_stats', {'type': 'sprite', 'height': 16, 'bpp_mode': 'bpp_4'})
    save(almanac_entry_panel(), 'almanac_entry', {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})
    cell = canvas(64, 64)
    cd = ImageDraw.Draw(cell)
    cd.rounded_rectangle((2, 11, 62, 53), radius=3, outline=9, width=2)
    cd.rounded_rectangle((3, 12, 61, 52), radius=2, outline=7)
    cd.line((6, 13, 58, 13), fill=8)
    save(cell, 'almanac_cell', {'type': 'sprite', 'height': 64, 'bpp_mode': 'bpp_4'})
    save(almanac_screen(), 'almanac', {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})
    options = canvas(64, 128)
    for selected in range(2):
        for choice, (label, top) in enumerate((('PLAY', 19), ('ALMANAC', 39))):
            text(options, 32, selected * 64 + top, label,
                 color=7 if selected == choice else 1, scale=2, centered=True)
    save(options, 'title_options', {'type': 'sprite', 'height': 64, 'bpp_mode': 'bpp_4'})
    screens = {'title': title_screen(), 'level_select': level_select_screen(), 'paused': pause_screen()}
    for name, image in screens.items():
        save(image, name, {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})
    sound('plant', [(280, .04), (420, .055)])
    sound('collect', [(880, .045), (1320, .065)])
    sound('navigate', [(600, .025)])
    sound('shot', [(440, .025), (260, .02)])
    sound('hit', [(150, .025)])
    chewing_sound()
    sound('wave_start', [(262, .08), (330, .08), (392, .14)])
    sound('mower', [(100, .06), (140, .06), (180, .12)], gain=1.5)
    sound('error', [(150, .08)])
    sound('boom', [(90, .06), (55, .12)], gain=1.5)
    sound('win', [(523, .09), (659, .09), (784, .09), (1046, .22)])
    defeat_sounds()


if __name__ == '__main__':
    main()
