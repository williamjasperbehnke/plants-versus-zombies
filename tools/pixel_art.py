"""Original pixel art for the garden and its characters, using the shared 16-color palette."""
import math
from PIL import Image, ImageDraw


def ellipse(draw, box, color, outline=1):
    draw.ellipse(box, fill=color, outline=outline)


def leaf(draw, x, y, right=False):
    direction = 1 if right else -1
    points = [(x, y), (x + direction * 7, y - 3),
              (x + direction * 6, y + 1), (x + direction * 2, y + 2)]
    draw.polygon(points, fill=4, outline=2)
    draw.line((x, y, x + direction * 5, y - 1), fill=5)


def plant_base(draw, pose=0):
    draw.ellipse((6, 24, 26, 26), fill=2)
    draw.line((15, 17, 15, 24), fill=2, width=3)
    draw.line((15, 18, 15, 23), fill=5)
    leaf(draw, 14, 24 - pose)
    leaf(draw, 17, 23 + pose, right=True)


def sunflower(draw, pose):
    plant_base(draw, pose)
    cx, cy = 15 + pose, 14
    for i in range(8):
        angle = i * math.pi / 4
        x, y = cx + round(math.cos(angle) * 4), cy + round(math.sin(angle) * 4)
        ellipse(draw, (x - 2, y - 2, x + 3, y + 3), 7, 10)
        draw.point((x, y - 1), fill=8)
    ellipse(draw, (cx - 5, cy - 4, cx + 5, cy + 5), 9)
    draw.ellipse((cx - 4, cy - 3, cx + 3, cy + 3), fill=10)
    draw.line((cx - 2, cy + 3, cx + 2, cy + 3), fill=8)
    for x in (cx - 2, cx + 2):
        draw.rectangle((x, cy - 1, x + 1, cy), fill=1)
        draw.point((x, cy - 1), fill=8)


def peashooter(draw, firing, pose=0):
    plant_base(draw, pose)
    dy = pose
    ellipse(draw, (9, 10 + dy, 22, 20 + dy), 4)
    draw.ellipse((11, 11 + dy, 20, 17 + dy), fill=5)
    draw.line((12, 11 + dy, 17, 11 + dy), fill=6)
    draw.polygon([(20, 12 + dy), (24, 13 + dy), (25, 18 + dy), (20, 18 + dy)], fill=4, outline=2)
    ellipse(draw, (23, 12 + dy, 27, 18 + dy), 3)
    ellipse(draw, (25, (14 if not firing else 13) + dy, 27, (16 if not firing else 17) + dy), 1)
    draw.line((24, 13 + dy, 25, 13 + dy), fill=6)
    draw.rectangle((15, 12 + dy, 16, 14 + dy), fill=1)
    draw.point((15, 12 + dy), fill=8)
    draw.point((11, 14 + dy), fill=6)
    draw.line((11, 18 + dy, 13, 19 + dy), fill=3)
    if firing:
        draw.line((6, 12, 5, 10), fill=6)


def walnut(draw, damage):
    draw.ellipse((7, 24, 26, 26), fill=2)
    ellipse(draw, (8, 8, 24, 26), 9)
    draw.ellipse((9, 9, 23, 24), fill=10)
    draw.arc((10, 10, 20, 23), 105, 260, fill=7, width=2)
    draw.line((21, 9, 22, 12, 21, 17, 22, 20), fill=9)
    draw.line((11, 19, 12, 22), fill=9)
    for x in (13, 19):
        draw.ellipse((x - 1, 12, x + 2, 16), fill=8)
        draw.rectangle((x, 13, x + 1, 15), fill=1)
    draw.line((14, 20, 17, 20), fill=9)
    if damage:
        draw.line((18, 9, 16, 11, 18, 13, 16, 16, 18, 20, 16, 25), fill=9)
        draw.point((17, 13), fill=1)
    if damage == 2:
        draw.line((9, 17, 12, 18, 10, 21), fill=1)
        draw.line((17, 12, 22, 10), fill=1)
        ellipse(draw, (14, 19, 18, 22), 1)
        draw.polygon([(23, 9), (20, 10), (23, 12)], fill=0)


def cherries(draw, pose=0):
    draw.ellipse((5, 24, 27, 26), fill=2)
    draw.line((11, 15, 17, 9, 22, 15), fill=2, width=2)
    draw.line((12, 14, 17, 10), fill=5)
    draw.polygon([(17, 10), (22, 8), (26, 9), (22, 12)], fill=4, outline=2)
    draw.line((19, 10, 24, 9), fill=6)
    for index, x in enumerate((6, 17)):
        dy = -pose if index == 0 else 0
        ellipse(draw, (x, 14 + dy, x + 9, 25 + dy), 9)
        draw.ellipse((x + 1, 14 + dy, x + 8, 23 + dy), fill=11)
        draw.line((x + 2, 16 + dy, x + 3, 15 + dy), fill=8, width=2)
        draw.line((x + 4, 18 + dy, x + 6, 19 + dy), fill=1)
        draw.point((x + 5, 20 + dy), fill=1)
        draw.point((x + 6, 22 + dy), fill=14)


def seed_icon(kind, canvas):
    """Draw centered native-pixel portraits, with an extra row for the walnut."""
    height = 16 if kind == 2 else 15
    image = canvas(15, height)
    draw = ImageDraw.Draw(image)
    if kind == 0:
        for i in range(8):
            angle = i * math.pi / 4
            x, y = 7 + round(math.cos(angle) * 4), 6 + round(math.sin(angle) * 4)
            ellipse(draw, (x - 2, y - 2, x + 2, y + 2), 7, 10)
            draw.point((x, y - 1), fill=8)
        ellipse(draw, (3, 2, 11, 10), 9)
        draw.ellipse((4, 3, 10, 9), fill=10)
        draw.line((5, 5, 5, 6), fill=1)
        draw.line((9, 5, 9, 6), fill=1)
        draw.line((6, 8, 8, 8), fill=8)
        draw.line((7, 11, 7, 13), fill=5)
        draw.polygon([(6, 12), (2, 11), (3, 13), (6, 13)], fill=4)
        draw.polygon([(8, 12), (12, 11), (11, 13), (8, 13)], fill=5)
    elif kind == 1:
        ellipse(draw, (1, 2, 11, 12), 4)
        draw.ellipse((2, 3, 10, 10), fill=5)
        draw.line((4, 3, 7, 3), fill=6)
        draw.rectangle((9, 5, 12, 10), fill=4)
        ellipse(draw, (11, 4, 14, 11), 3)
        draw.rectangle((13, 6, 14, 9), fill=1)
        draw.point((12, 5), fill=6)
        draw.rectangle((6, 5, 7, 7), fill=1)
        draw.point((6, 5), fill=8)
        draw.line((6, 12, 6, 14), fill=5, width=2)
        draw.polygon([(5, 13), (1, 11), (2, 14), (5, 14)], fill=4)
        draw.polygon([(8, 13), (13, 11), (12, 14), (8, 14)], fill=4)
        draw.line((9, 13, 12, 12), fill=6)
    elif kind == 2:
        ellipse(draw, (1, 0, 14, 15), 9)
        draw.ellipse((2, 1, 13, 14), fill=10)
        draw.arc((3, 2, 11, 13), 110, 260, fill=7)
        draw.line((12, 3, 13, 7, 12, 11), fill=9)
        for x in (5, 9):
            draw.rectangle((x, 5, x + 2, 8), fill=8)
            draw.rectangle((x + 1, 6, x + 2, 8), fill=1)
        draw.line((7, 11, 8, 11), fill=9)
    elif kind == 3:
        draw.line((3, 7, 7, 3, 10, 6), fill=5)
        draw.polygon([(7, 3), (10, 1), (13, 1), (11, 4)], fill=5, outline=3)
        draw.line((9, 3, 11, 2), fill=6)
        for x, y in [(1, 7), (8, 6)]:
            # Keep a continuous, opaque fruit edge; facial marks stay inside it.
            ellipse(draw, (x, y, x + 5, 13), 11, 9)
            draw.ellipse((x + 1, y + 1, x + 4, 11), fill=11)
            draw.line((x + 1, y + 2, x + 2, y + 1), fill=14)
            draw.point((x + 2, y + 2), fill=8)
            draw.point((x + 3, y + 3), fill=1)
            draw.point((x + 3, y + 4), fill=1)
    else:
        raise ValueError(f'Not a seed packet: {kind}')
    # Center the visible artwork, ignoring transparent padding; retain native pixels.
    bounds = image.getbbox()
    portrait = image.crop(bounds)
    centered = canvas(15, height)
    centered.paste(portrait, ((15 - portrait.width + 1) // 2, (height - portrait.height + 1) // 2))
    return centered


def traffic_cone(draw):
    # The broad oval foot wraps around the crown, centered over the head.
    ellipse(draw, (9, 8, 23, 10), 10)
    draw.line((11, 10, 21, 10), fill=9)
    # A rounded, slightly leaning tip and shaded side avoid a flat triangle.
    draw.polygon([(14, 4), (16, 4), (18, 6), (21, 9),
                  (11, 9), (12, 7)], fill=14, outline=9)
    draw.polygon([(16, 5), (20, 9), (18, 9)], fill=10)
    draw.line((14, 5, 12, 8), fill=7)
    draw.polygon([(13, 7), (18, 7), (20, 8), (12, 8)], fill=8)
    draw.line((14, 8, 19, 8), fill=13)
    draw.line((12, 9, 21, 9), fill=14)


def zombie(draw, cone, pose, chewing):
    bite_pose = pose
    if chewing:
        pose = 0  # Keep feet and hands planted; only the jaw moves while eating.
    draw.ellipse((8, 25, 24, 27), fill=2)
    # Opposing feet/arms form a two-frame shuffle, rather than just vertical bobbing.
    left = 11 - pose * 2
    right = 20 + pose
    draw.line((15, 22, left + 2, 26), fill=1, width=5)
    draw.line((20, 22, right, 26), fill=1, width=5)
    draw.line((15, 22, left + 2, 26), fill=15, width=3)
    draw.line((20, 22, right, 26), fill=15, width=3)
    draw.rounded_rectangle((left - 1, 25, left + 3, 27), radius=1, fill=1)
    draw.rounded_rectangle((right - 2, 25, right + 2, 27), radius=1, fill=1)
    draw.polygon([(14, 16), (20, 16), (22, 18), (23, 22),
                  (21, 24), (14, 24), (12, 22), (11, 19)], fill=9, outline=1)
    draw.line((21, 18, 22, 21), fill=10)
    draw.polygon([(14, 17), (16, 20), (18, 17)], fill=8)
    draw.line((16, 19, 15, 22), fill=11, width=2)
    # Sleeves stay outside the shirt instead of cutting dark stripes across it.
    draw.line((12, 18, 8, 18 - pose, 6, 17 - pose), fill=1, width=3)
    draw.line((12, 18, 8, 18 - pose, 6, 17 - pose), fill=9)
    ellipse(draw, (3, 15 - pose, 8, 18 - pose), 12)
    draw.line((4, 16 - pose, 5, 16 - pose), fill=13)
    draw.line((12, 20, 9, 22 - pose, 6, 21 - pose), fill=1, width=3)
    draw.line((12, 20, 9, 22 - pose, 6, 21 - pose), fill=9)
    ellipse(draw, (2, 20 - pose, 7, 23 - pose), 12)
    draw.line((3, 21 - pose, 4, 21 - pose), fill=13)
    zombie_head(draw, cone, bite_pose, chewing)


def zombie_head(draw, cone=False, pose=0, chewing=False):
    """Shared face artwork for world sprites and the wave indicator."""
    ellipse(draw, (9, 8, 22, 18), 12)
    draw.ellipse((10, 9, 20, 16), fill=13)
    # A sunken cheek and rounded jaw keep the mouth inside the face silhouette.
    draw.line((19, 14, 20, 16), fill=12, width=2)
    draw.line((10, 15, 10, 16), fill=12)
    for x, y in ((10, 10), (17, 10)):
        ellipse(draw, (x, y, x + 4, y + 4), 8)
        draw.line((x + 1, y + 1, x + 1, y + 2), fill=1)
    draw.line((15, 13, 15, 14), fill=12)
    draw.line((12, 16, 14, 16), fill=1)
    draw.point((12, 16), fill=13)
    if chewing and pose:
        draw.line((12, 17, 14, 17), fill=1)
        draw.point((14, 16), fill=13)
    if not cone:
        draw.line((12, 8, 13, 7), fill=1)
        draw.point((16, 7), fill=1)
        draw.point((20, 8), fill=1)
    else:
        traffic_cone(draw)


def actor(frame, canvas):
    image = canvas(32, 32)
    draw = ImageDraw.Draw(image)
    if frame in (0, 12):
        sunflower(draw, frame == 12)
    elif frame in (1, 13, 20):
        peashooter(draw, frame == 13, int(frame == 20))
    elif frame in (2, 10, 11, 21, 22, 23):
        damage = frame - 21 if frame >= 21 else (0 if frame == 2 else frame - 9)
        walnut(draw, damage)
        if frame >= 21:
            # Move the upper shell one pixel while keeping the base planted.
            body = image.crop((0, 0, 32, 24))
            image.paste(0, (0, 0, 32, 24))
            image.paste(body, (0, 1))
    elif frame in (3, 24):
        cherries(draw, int(frame == 24))
    elif frame in (4, 5, 14, 15, 16, 17, 18, 19):
        cone = frame % 2 == 1
        pose = int(frame in (14, 15, 18, 19))
        zombie(draw, cone, pose, frame >= 16)
    else:
        raise ValueError(f'Not a character frame: {frame}')
    return image


def draw_title_garden(image, canvas, cast, floor):
    """Use the exact gameplay sprites at integer 2x scale, with aligned feet."""
    for frame, center_x in cast:
        source = actor(frame, canvas)
        bottom = source.getbbox()[3]
        enlarged = source.resize((64, 64), Image.Resampling.NEAREST)
        x, y = center_x - 32, floor - (bottom * 2 - 1)
        image.paste(enlarged, (x, y), enlarged.point([0] + [255] * 255, 'L'))


def shovel_icon(canvas):
    image = canvas(16, 16)
    draw = ImageDraw.Draw(image)
    # D grip, wooden shaft, and a concave steel spade with a pointed cutting edge.
    draw.line((12, 4, 5, 11), fill=1, width=4)
    draw.line((11, 5, 5, 11), fill=10, width=2)
    draw.line((10, 5, 6, 9), fill=7)
    draw.rounded_rectangle((9, 0, 15, 5), radius=2, fill=13, outline=1)
    draw.rectangle((11, 2, 13, 3), fill=1)
    draw.line((10, 1, 14, 1), fill=8)
    draw.polygon([(5, 7), (9, 11), (7, 14), (3, 15), (0, 12), (1, 9)], fill=12, outline=1)
    draw.polygon([(4, 9), (7, 11), (6, 13), (3, 14), (1, 12), (2, 10)], fill=13)
    draw.line((2, 10, 1, 12, 3, 14), fill=8)
    draw.line((4, 10, 4, 13), fill=15)
    return image


def draw_garden(image):
    draw = ImageDraw.Draw(image)
    # Wood porch: highlighted boards, seams, grain, and small nail heads.
    draw.rectangle((0, 57, 13, 153), fill=9)
    for y in range(58, 154, 9):
        draw.line((0, y, 13, y), fill=10)
        draw.line((0, y + 7, 13, y + 7), fill=1)
        draw.line((3, y + 3, 8, y + 3), fill=10)
        draw.point((11, y + 2), fill=13)
    draw.line((14, 57, 14, 153), fill=1)
    # Stone mower path and the narrow path at the far side of the lawn.
    draw.rectangle((15, 57, 32, 153), fill=12)
    draw.rectangle((234, 57, 239, 153), fill=12)
    for y in range(58, 154, 19):
        draw.rounded_rectangle((16, y, 31, min(y + 17, 153)), radius=2, fill=13, outline=2)
        draw.line((18, y + 1, 28, y + 1), fill=8)
        draw.line((235, y + 8, 239, y + 8), fill=2)
    # Keep cell centers quiet for units; grass clusters sit mostly at tile edges.
    for row in range(5):
        for column in range(9):
            x, y = 34 + column * 22, 58 + row * 19
            base = 4 if (row + column) % 2 == 0 else 3
            draw.rectangle((x, y, x + 21, y + 18), fill=base)
            alternate = 3 if base == 4 else 4
            for seed in range(4):
                dx = (column * 7 + row * 3 + seed * 5) % 19 + 1
                dy = (row * 5 + column * 3 + seed * 7) % 16 + 1
                draw.line((x + dx, y + dy, x + dx + 1, y + dy), fill=alternate)
            for dx, dy in [(2, 14), (17, 4)]:
                draw.line((x + dx - 1, y + dy + 2, x + dx, y + dy - 1), fill=2)
                draw.line((x + dx, y + dy + 2, x + dx + 2, y + dy), fill=5)
                draw.point((x + dx, y + dy - 1), fill=4)
            draw.line((x + 4, y + 18, x + 10, y + 18), fill=alternate)
    draw.line((33, 57, 33, 153), fill=2)
    draw.line((233, 57, 233, 153), fill=2)
    for x in range(35, 233, 11):
        draw.line((x, 58, x + 3, 57), fill=5)
        draw.line((x + 2, 153, x + 5, 152), fill=2)
    # Flowers remain outside playable cells so they cannot be mistaken for units.
    draw.rectangle((0, 154, 239, 159), fill=9)
    draw.line((0, 154, 239, 154), fill=5)
    for x in range(8, 239, 21):
        draw.line((x, 159, x, 156), fill=4)
        draw.point((x - 1, 155), fill=8 if x % 2 else 7)
        draw.point((x + 1, 155), fill=8 if x % 2 else 7)
        draw.point((x, 156), fill=14)
