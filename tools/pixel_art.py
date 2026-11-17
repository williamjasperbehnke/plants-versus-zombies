"""Original pixel art for the garden and its characters, using the shared 16-color palette."""
import math
from pathlib import Path
from PIL import Image, ImageDraw


# Temporary mask index: character atlases pack this into a dedicated palette slot.
SHADOW_PIXEL = 16


def shadow(draw, box):
    draw.ellipse(box, fill=SHADOW_PIXEL)


def ellipse(draw, box, color, outline=1):
    draw.ellipse(box, fill=color, outline=outline)


def leaf(draw, x, y, right=False):
    direction = 1 if right else -1
    points = [(x, y), (x + direction * 7, y - 3),
              (x + direction * 6, y + 1), (x + direction * 2, y + 2)]
    draw.polygon(points, fill=4, outline=2)
    draw.line((x, y, x + direction * 5, y - 1), fill=5)


def plant_base(draw, pose=0):
    shadow(draw, (6, 24, 26, 26))
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
    # Bright green tube and lip surround a recessed, dark-green opening.
    draw.polygon([(20, 12 + dy), (24, 13 + dy), (25, 18 + dy), (20, 18 + dy)], fill=5, outline=4)
    ellipse(draw, (23, 12 + dy, 27, 18 + dy), 5, 4)
    ellipse(draw, (25, (14 if not firing else 13) + dy, 26, (16 if not firing else 17) + dy), 2, 2)
    draw.point((26, 15 + dy), fill=1)
    draw.line((24, 13 + dy, 25, 13 + dy), fill=6)
    draw.rectangle((15, 12 + dy, 16, 14 + dy), fill=1)
    draw.point((15, 12 + dy), fill=8)
    draw.point((11, 14 + dy), fill=6)
    draw.line((11, 18 + dy, 13, 19 + dy), fill=3)
    if firing:
        draw.line((6, 12, 5, 10), fill=6)


def snow_peashooter(draw, firing, pose):
    peashooter(draw, firing, pose)
    draw.polygon([(10, 11 + pose), (10, 7 + pose), (13, 9 + pose),
                  (15, 6 + pose), (16, 9 + pose), (20, 7 + pose), (19, 11 + pose)], fill=6, outline=4)
    draw.point((15, 8 + pose), fill=8)
    draw.point((11, 13 + pose), fill=6)


def repeater(draw, firing, pose=0):
    peashooter(draw, firing, pose)
    # A swept leaf crest and heavier brow distinguish him from a Peashooter.
    draw.polygon([(10, 12 + pose), (7, 8 + pose), (11, 9 + pose),
                  (10, 6 + pose), (15, 9 + pose), (18, 9 + pose), (19, 11 + pose)], fill=4, outline=2)
    draw.line((11, 9 + pose, 15, 10 + pose), fill=6)
    draw.line((14, 11 + pose, 18, 12 + pose), fill=2)


def chomper(draw, state):
    chewing = state in (5, 6, 9, 10)
    lean = 1 if state in (1, 3, 6) else 0
    bob = 1 if state in (8, 10) else (-1 if state == 2 else 0)
    shadow(draw, (5, 25, 27, 28))
    # The stem bends with the head; the roots and ground shadow stay planted.
    draw.line((14, 25, 13, 21, 13 + lean, 16 + bob), fill=2, width=3)
    draw.line((14, 24, 14, 21, 14 + lean, 17 + bob), fill=3)
    leaf_lift = 1 if state == 8 else 0
    draw.polygon([(14, 25), (7, 21 - leaf_lift), (3, 22 - leaf_lift), (7, 26)], fill=3, outline=2)
    draw.polygon([(15, 25), (22, 21 + leaf_lift), (27, 23 + leaf_lift), (22, 27)], fill=3, outline=2)
    draw.line((17, 25, 23, 23 + leaf_lift), fill=2)
    def points(values): return [(x + lean, y + bob) for x, y in values]
    def box(values):
        x1, y1, x2, y2 = values
        return (x1 + lean, y1 + bob, x2 + lean, y2 + bob)
    opened = state in (0, 1, 2, 3, 7, 8)
    if opened:
        gape = 1 if state in (2, 3, 8) else 0
        ellipse(draw, box((6, 4, 26, 18)), 5, 2)
        draw.arc(box((8, 5, 24, 17)), 110, 270, fill=6)
        draw.polygon(points([(10, 16), (12, 22 + gape), (20, 24 + gape),
                              (27, 22 + gape), (29, 19 + gape), (22, 19), (15, 15)]), fill=5, outline=2)
        draw.polygon(points([(13, 12), (27, 9), (26, 13), (18, 14),
                              (27, 20 + gape), (21, 21 + gape), (14, 18)]), fill=14)
        for x, y in ((17, 12), (22, 11), (25, 10)):
            draw.polygon(points([(x, y), (x + 2, y), (x + 1, y + 3)]), fill=8)
        for x in (18, 23, 26):
            draw.polygon(points([(x, 21 + gape), (x + 2, 21 + gape), (x + 1, 18 + gape + (1 if x == 18 else 0))]), fill=8)
        draw.line(points([(17, 18), (22, 20 + gape)]), fill=10, width=2)
        draw.line(points([(13, 21 + gape), (18, 23 + gape), (22, 23 + gape)]), fill=4)
    else:
        squeeze = 1 if state == 9 else 0
        ellipse(draw, box((6 - squeeze, 5 + squeeze, 27, 23 - squeeze)), 5, 2)
        draw.arc(box((8, 6 + squeeze, 25, 22 - squeeze)), 110, 270, fill=6)
        draw.arc(box((10, 12, 18, 21)), 80, 250, fill=4)
        lip = 17 if state == 6 else 16
        draw.line(points([(14, lip), (27, lip - 1)]), fill=14, width=2)
        for x in (16, 21, 25): draw.point((x + lean, lip + bob), fill=8)
        if state == 6:
            draw.line(points([(18, lip + 1), (24, lip)]), fill=4)
    draw.polygon(points([(10, 6), (9, 2), (14, 5), (18, 2),
                          (19, 5), (23, 4), (22, 7)]), fill=3, outline=2)
    draw.line(points([(10, 9), (13, 8)]), fill=6)
    if chewing:
        # The grip tucks inward on compression and falls back on the open chew.
        tuck = 1 if state in (6, 9) else 0
        drop = 1 if state in (6, 10) else 0
        arm = [(25 - tuck, 16), (28 - tuck, 17 + drop), (29 - tuck, 20 + drop)]
        draw.line(points(arm), fill=1, width=3)
        draw.line(points(arm[:2]), fill=9, width=2)
        hand = (28 - tuck, 18 + drop, 31 - tuck, 22 + drop)
        draw.rounded_rectangle(box(hand), radius=1, fill=12, outline=1)
        draw.line(points([(29 - tuck, 19 + drop), (30 - tuck, 19 + drop)]), fill=13)
        draw.line(points([(24, 18), (25 - tuck, 24 + drop), (27 - tuck, 26 + drop)]), fill=1, width=3)
        draw.line(points([(24, 18), (25 - tuck, 23 + drop)]), fill=9, width=2)
        draw.line(points([(25 - tuck, 24 + drop), (27 - tuck, 26 + drop)]), fill=12, width=2)
        draw.line(points([(26 - tuck, 27 + drop), (29 - tuck, 27 + drop)]), fill=1, width=2)
        draw.point((28 - tuck + lean, 26 + drop + bob), fill=13)


def squash(draw, state):
    if state != 3:
        shadow(draw, (5, 25, 27, 28))
    if state == 4:
        ellipse(draw, (4, 22, 28, 28), 5, 2)
        draw.arc((6, 22, 26, 27), 190, 340, fill=6)
        draw.line((10, 24, 14, 25), fill=2)
        draw.line((19, 25, 23, 24), fill=2)
        return
    dy = 4 if state == 2 else state % 2
    # Narrow crown, broad hanging cheeks and a heavy, unimpressed face.
    outline = [(13, 5 + dy), (19, 5 + dy), (22, 8 + dy), (23, 15 + dy),
               (26, 21), (25, 25), (21, 27), (10, 27), (6, 24), (7, 19),
               (10, 14 + dy), (10, 8 + dy)]
    draw.polygon(outline, fill=5, outline=2)
    draw.polygon([(19, 7 + dy), (21, 10 + dy), (21, 17 + dy), (24, 23),
                  (21, 25), (19, 22)], fill=4)
    draw.line((12, 9 + dy, 12, 16 + dy, 9, 21, 10, 24), fill=6, width=2)
    draw.line((14, 6 + dy, 18, 6 + dy), fill=6)
    draw.polygon([(14, 6 + dy), (14, 2 + dy), (17, 1 + dy), (19, 3 + dy), (17, 5 + dy)], fill=4, outline=2)
    for x in (11, 18):
        draw.rectangle((x, 13 + dy, x + 4, 16 + dy), fill=8)
        draw.line((x + 2, 14 + dy, x + 2, 16 + dy), fill=1)
    draw.line((10, 12 + dy, 15, 14 + dy), fill=2, width=2)
    draw.line((18, 14 + dy, 23, 12 + dy), fill=2, width=2)
    draw.line((14, 20 + dy, 19, 20 + dy), fill=2)
    draw.line((13, 22 + dy, 20, 22 + dy), fill=4)


def pole_grip(draw, x, y):
    """Use the same outlined skin silhouette and highlight as other zombie hands."""
    ellipse(draw, (x - 2, y - 1, x + 2, y + 2), 12)
    draw.line((x - 1, y, x + 1, y), fill=13)


def vaulting_pole(draw, state, wounded=False):
    """Continuous pole with compact grips, repainted after arm damage masking."""
    pose = state % 2
    chewing = state % 4 >= 2
    dy = 0 if chewing else pose
    # Paint sleeves behind the shaft and fingers over it.
    draw.line((22, 19, 23, 20, 23, 19 + dy), fill=1, width=3)
    draw.line((22, 19, 23, 20, 23, 19 + dy), fill=11)
    if not wounded:
        draw.line((12, 20, 12, 21, 11, 22 + dy), fill=1, width=3)
        draw.line((12, 20, 12, 21, 11, 22 + dy), fill=11)
    draw.line((2, 23 + dy, 29, 15 + dy), fill=1, width=3)
    draw.line((2, 23 + dy, 29, 15 + dy), fill=10)
    pole_grip(draw, 22, 17 + dy)
    if not wounded:
        ellipse(draw, (8, 20 + dy, 11, 23 + dy), 12)
        draw.line((9, 21 + dy, 10, 21 + dy), fill=13)


def vaulting_legs(draw, stage):
    """Push off, tuck, reach forward, then extend for landing."""
    poses = (
        ((15, 23, 13, 25, 12, 27), (20, 23, 22, 23, 24, 25)),
        ((15, 23, 12, 23, 11, 25), (20, 23, 22, 25, 24, 24)),
        ((15, 23, 12, 24, 10, 26), (20, 23, 22, 24, 23, 26)),
        ((15, 23, 13, 25, 12, 27), (20, 23, 21, 25, 20, 27)),
    )
    for leg in poses[stage]:
        draw.line(leg, fill=1, width=4)
        draw.line(leg, fill=15, width=2)
        foot_x, foot_y = leg[-2:]
        draw.polygon([(foot_x - 2, foot_y), (foot_x + 1, foot_y),
                      (foot_x + 1, foot_y + 1), (foot_x - 3, foot_y + 1)], fill=1)


def pole_vaulter(draw, state):
    pose = state % 2
    chewing = state % 4 >= 2 and state < 8
    zombie(draw, False, pose, chewing, carrying='athlete')
    if state >= 8:
        # Replace the planted legs before painting the jersey and hands, so
        # no old shoe pixels remain and the hands never get clipped away.
        draw.rectangle((7, 22, 27, 29), fill=0)
        vaulting_legs(draw, state - 8)
    draw.polygon([(14, 17), (20, 17), (22, 19), (23, 22),
                  (21, 24), (14, 24), (12, 22), (12, 19)], fill=11, outline=1)
    jersey_skull(draw, 15, 18)
    draw.line((11, 10, 22, 10), fill=11)
    draw.line((13, 10, 19, 10), fill=8)
    if state < 4:
        vaulting_pole(draw, state)
    else:
        # Keep the hands beside the torso even during the bite; do not stretch
        # the forearm toward the plant after the pole has been spent.
        hand_y = 20 + pose if chewing else 21 - pose
        draw.line((12, 20, 11, 21, 10, hand_y), fill=1, width=3)
        draw.line((12, 20, 11, 21, 10, hand_y), fill=11)
        ellipse(draw, (6, hand_y - 1, 10, hand_y + 2), 12)
        draw.line((7, hand_y, 9, hand_y), fill=13)
        # Start inside the sleeve so the upper border has no projecting corner.
        draw.line((21, 19, 23, 20), fill=1, width=3)
        draw.line((21, 19, 23, 20), fill=11)
        ellipse(draw, (21, 19, 25, 22), 12)
        draw.line((22, 20, 24, 20), fill=13)


def walnut(draw, damage):
    shadow(draw, (7, 24, 26, 26))
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


def tallnut(draw, state):
    damage, pose = state // 2, state % 2
    shadow(draw, (7, 26, 26, 29))
    # An elongated walnut, with a continuous convex shell rather than a pinched waist.
    shape = [(13, 2 + pose), (19, 2 + pose), (23, 4 + pose), (25, 8),
             (26, 14), (26, 21), (24, 26), (20, 28), (12, 28),
             (8, 25), (7, 20), (7, 12), (9, 6)]
    draw.polygon(shape, fill=10, outline=1)
    draw.polygon([(22, 6), (24, 10), (24, 20), (22, 25), (19, 27),
                  (21, 22), (22, 15)], fill=9)
    draw.arc((9, 4 + pose, 23, 27), 110, 255, fill=7)
    draw.line((12, 5 + pose, 16, 4 + pose, 19, 5 + pose), fill=7)
    # Broad whites and two-pixel pupils keep his steady, forward-looking expression readable.
    for x in (11, 19):
        ellipse(draw, (x - 1, 10 + pose, x + 4, 16 + pose), 8, 9)
        draw.rectangle((x + 2, 12 + pose, x + 3, 15 + pose), fill=1)
        draw.point((x + 2, 12 + pose), fill=8)
    draw.line((10, 9 + pose, 15, 10 + pose), fill=9)
    draw.line((18, 10 + pose, 23, 9 + pose), fill=9)
    draw.line((14, 20 + pose, 15, 21 + pose, 19, 21 + pose, 20, 20 + pose), fill=9)
    draw.point((10, 18), fill=9)
    draw.point((23, 18), fill=9)
    if damage:
        draw.line((18, 3 + pose, 16, 7, 18, 10, 16, 14, 18, 18, 16, 27), fill=9)
    if damage == 2:
        draw.line((8, 17, 12, 16, 11, 20, 13, 25), fill=9)
        draw.polygon([(24, 5), (21, 7), (25, 10)], fill=0)


def jersey_skull(draw, x, y):
    # One shared five-pixel emblem, with inset eye sockets and two teeth.
    for row, pixels in enumerate(('.###.', '#####', '#.#.#', '.###.', '.#.#.')):
        for column, pixel in enumerate(pixels):
            if pixel == '#': draw.point((x + column, y + row), fill=8)
            elif row == 2: draw.point((x + column, y + row), fill=11)


def cherries(draw, pose=0):
    shadow(draw, (5, 24, 27, 26))
    draw.line((11, 15, 17, 9, 22, 15), fill=2, width=2)
    draw.line((12, 14, 17, 10), fill=5)
    draw.polygon([(17, 10), (22, 8), (26, 9), (22, 12)], fill=4, outline=2)
    draw.line((19, 10, 24, 9), fill=6)
    for index, x in enumerate((6, 17)):
        dy = -pose if index == 0 else 0
        ellipse(draw, (x, 14 + dy, x + 9, 25 + dy), 9)
        draw.ellipse((x + 1, 14 + dy, x + 8, 23 + dy), fill=11)
        draw.line((x + 2, 16 + dy, x + 3, 15 + dy), fill=14)
        draw.point((x + 3, 15 + dy), fill=8)
        for eye in (x + 3, x + 6):
            draw.rectangle((eye, 18 + dy, eye + 1, 20 + dy), fill=8)
            draw.point((eye + (1 if index == 0 else 0), 19 + dy), fill=1)
        if index == 0:
            draw.line((x + 2, 17 + dy, x + 4, 18 + dy), fill=1)
            draw.line((x + 6, 18 + dy, x + 7, 17 + dy), fill=1)
            draw.line((x + 3, 22 + dy, x + 6, 22 + dy), fill=1)
            draw.point((x + 4, 22 + dy), fill=8)
        else:
            draw.line((x + 2, 17 + dy, x + 4, 16 + dy), fill=9)
            draw.line((x + 6, 16 + dy, x + 7, 17 + dy), fill=9)
            draw.line((x + 4, 22 + dy, x + 6, 22 + dy), fill=1)
            draw.point((x + 4, 23 + dy), fill=1)


def seed_icon(kind, canvas):
    """Draw centered native-pixel portraits, with an extra row for the walnut."""
    height = 17 if kind == 8 else (16 if kind in (1, 2, 5, 6, 7, 9) else 15)
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
    elif kind in (1, 5, 7):
        ellipse(draw, (1, 2, 11, 12), 4)
        draw.ellipse((2, 3, 10, 10), fill=5)
        draw.line((4, 3, 7, 3), fill=6)
        draw.rectangle((9, 5, 12, 10), fill=5)
        ellipse(draw, (11, 4, 14, 11), 5, 4)
        draw.line((13, 6, 13, 9), fill=2)
        draw.point((13, 8), fill=1)
        draw.point((12, 5), fill=6)
        draw.rectangle((6, 5, 7, 7), fill=1)
        draw.point((6, 5), fill=8)
        draw.line((6, 12, 6, 15), fill=5, width=2)
        draw.polygon([(5, 13), (1, 11), (2, 14), (5, 14)], fill=4)
        draw.polygon([(8, 13), (13, 11), (12, 14), (8, 14)], fill=4)
        draw.line((9, 13, 12, 12), fill=6)
        if kind == 7:
            draw.polygon([(2, 4), (0, 1), (4, 2), (4, 0), (8, 2), (9, 4)], fill=4, outline=2)
            draw.line((5, 4, 8, 5), fill=2)
        if kind == 5:
            draw.polygon([(2, 3), (2, 0), (5, 2), (7, 0), (8, 2), (11, 1), (9, 4)], fill=6, outline=4)
            draw.point((7, 1), fill=8)
    elif kind == 2:
        ellipse(draw, (1, 0, 14, 15), 9)
        draw.ellipse((2, 1, 13, 14), fill=10)
        draw.arc((3, 2, 11, 13), 110, 260, fill=7)
        draw.line((12, 3, 13, 7, 12, 11), fill=9)
        for x in (5, 9):
            draw.rectangle((x, 5, x + 2, 8), fill=8)
            draw.rectangle((x + 1, 6, x + 2, 8), fill=1)
        draw.line((7, 11, 8, 11), fill=9)
    elif kind == 9:
        draw.line((6, 9, 6, 15), fill=3, width=2)
        draw.polygon([(6, 14), (1, 12), (1, 14), (5, 15)], fill=3, outline=2)
        draw.polygon([(7, 14), (12, 12), (13, 14), (8, 15)], fill=3, outline=2)
        ellipse(draw, (0, 1, 14, 11), 5, 2)
        draw.line((3, 4, 6, 3), fill=6)
        draw.polygon([(7, 6), (14, 4), (12, 8), (14, 11), (8, 10)], fill=14)
        for x, y in ((9, 5), (12, 5), (10, 9)):
            draw.line((x, y, x, y + 1), fill=8)
        draw.polygon([(3, 3), (2, 0), (6, 2), (9, 0), (10, 3)], fill=3, outline=2)
    elif kind == 8:
        # Tall, softly rounded shell with a wider, readable face and a shaded flank.
        draw.polygon([(6, 0), (11, 0), (13, 2), (14, 5), (14, 10),
                      (13, 13), (11, 15), (9, 16), (6, 16), (3, 14), (1, 10), (1, 5), (2, 2)], fill=10, outline=1)
        draw.line((5, 2, 3, 4, 3, 10, 5, 14), fill=7)
        draw.line((12, 3, 13, 6, 13, 12, 11, 14), fill=9)
        for x in (4, 9):
            draw.ellipse((x - 1, 4, x + 2, 8), fill=8)
            draw.line((x + 1, 5, x + 1, 7), fill=1)
            draw.point((x + 1, 5), fill=8)
        draw.line((3, 3, 6, 4), fill=9)
        draw.line((8, 4, 11, 3), fill=9)
        draw.line((5, 11, 6, 12, 9, 12, 10, 11), fill=9)
    elif kind == 4:
        ellipse(draw, (1, 4, 13, 13), 10, 9)
        draw.arc((2, 5, 12, 12), 140, 275, fill=7)
        draw.line((4, 5, 8, 5), fill=7)
        draw.point((3, 7), fill=8)
        draw.point((11, 11), fill=9)
        draw.line((7, 2, 7, 5), fill=1)
        ellipse(draw, (5, 0, 9, 3), 11, 13)
        draw.point((6, 1), fill=8)
        for x in (4, 9):
            draw.ellipse((x - 1, 7, x + 1, 10), fill=8)
            draw.line((x, 8, x, 9), fill=1)
        draw.line((6, 11, 8, 11), fill=9)
    elif kind == 6:
        draw.polygon([(5, 2), (10, 2), (12, 5), (12, 9), (14, 12),
                      (12, 15), (3, 15), (1, 12), (3, 7), (3, 4)], fill=5, outline=2)
        draw.line((5, 4, 4, 9, 3, 12), fill=6)
        draw.line((8, 3, 7, 0, 9, 0), fill=2)
        for x in (4, 9):
            draw.rectangle((x, 7, x + 2, 9), fill=8)
            draw.point((x + 1, 9), fill=1)
        draw.line((3, 6, 6, 7), fill=2)
        draw.line((9, 7, 12, 6), fill=2)
        draw.line((6, 12, 9, 12), fill=2)
    elif kind == 3:
        draw.line((3, 7, 7, 3, 10, 6), fill=5)
        draw.polygon([(7, 3), (10, 1), (13, 1), (11, 4)], fill=5, outline=3)
        draw.line((9, 3, 11, 2), fill=6)
        for x, y in [(1, 6), (8, 5)]:
            # Two readable eyes and a little grin, enclosed by a solid fruit edge.
            ellipse(draw, (x, y, x + (6 if x == 8 else 5), 14), 11, 9)
            draw.point((x + 2, y + 1), fill=14)
            for eye in (x + 1, x + (4 if x == 8 else 3)):
                draw.line((eye, y + 2, eye, y + 3), fill=8)
                draw.point((eye, y + 3), fill=1)
            draw.line((x + 2, y + 5, x + 3, y + 5), fill=1)
    else:
        raise ValueError(f'Not a seed packet: {kind}')
    # Center the visible artwork, ignoring transparent padding; retain native pixels.
    bounds = image.getbbox()
    portrait = image.crop(bounds)
    centered = canvas(15, height)
    centered.paste(portrait, ((15 - portrait.width + 1) // 2, (height - portrait.height + 1) // 2))
    return centered


def traffic_cone(draw):
    # A blunt tip, shaded side, and clear reflective band define the frustum.
    draw.polygon([(14, 2), (16, 2), (17, 3), (21, 8),
                  (11, 8), (12, 6)], fill=14, outline=9)
    draw.polygon([(16, 3), (20, 8), (18, 8)], fill=10)
    draw.line((14, 3, 12, 7), fill=7)
    draw.polygon([(13, 5), (17, 5), (19, 6), (12, 6)], fill=8)
    draw.point((18, 6), fill=13)
    # Warm, rounded edges seat the cone without a black bar across the forehead.
    ellipse(draw, (10, 7, 22, 9), 10, 9)
    draw.line((12, 8, 20, 8), fill=14)


def zombie(draw, cone, pose, chewing, carrying=None):
    bite_pose = pose
    if chewing:
        pose = 0  # Keep feet and hands planted; only the jaw moves while eating.
    shadow(draw, (8, 25, 24, 27))
    # Opposing feet/arms form a two-frame shuffle, rather than just vertical bobbing.
    left = 11 - pose * 2
    right = 20 + pose
    trousers = 15
    # Closed silhouettes retain a single-pixel border along each trouser leg.
    draw.polygon([(13, 22), (17, 22), (left + 3, 26), (left, 26)], fill=trousers, outline=1)
    draw.polygon([(18, 22), (22, 22), (right + 1, 26), (right - 2, 26)], fill=trousers, outline=1)
    draw.rounded_rectangle((left - 1, 26, left + 3, 27), radius=1, fill=1)
    draw.rounded_rectangle((right - 2, 26, right + 2, 27), radius=1, fill=1)
    coat = 9
    coat_light = 10
    if carrying != 'athlete':
        draw.polygon([(14, 16), (20, 16), (22, 18), (23, 22),
                      (21, 24), (14, 24), (12, 22), (11, 19)], fill=coat, outline=1)
        draw.polygon([(19, 18), (21, 18), (22, 21), (21, 23), (19, 23)], fill=coat_light)
        draw.line((14, 18, 13, 20), fill=coat_light)
        draw.point((20, 22), fill=coat)
        # A shaded shirt and narrow tie leave room for small vertical fabric folds.
        draw.polygon([(14, 17), (18, 17), (18, 22), (16, 23), (14, 22)], fill=13)
        draw.line((14, 20, 14, 21), fill=12)
        draw.line((18, 20, 18, 21), fill=8)
        draw.polygon([(14, 17), (16, 20), (18, 17)], fill=8)
        draw.polygon([(16, 19), (17, 20), (16, 23), (15, 22)], fill=11)
    # Sleeves stay outside the shirt instead of cutting dark stripes across it.
    if carrying not in ('door', 'athlete'):
        draw.line((12, 18, 8, 18 - pose, 6, 17 - pose), fill=1, width=3)
        draw.line((12, 18, 8, 18 - pose, 6, 17 - pose), fill=coat)
        ellipse(draw, (3, 15 - pose, 8, 18 - pose), 12)
        draw.line((4, 16 - pose, 5, 16 - pose), fill=13)
    if carrying is None:
        draw.line((12, 20, 9, 22 - pose, 6, 21 - pose), fill=1, width=3)
        draw.line((12, 20, 9, 22 - pose, 6, 21 - pose), fill=coat)
        ellipse(draw, (2, 20 - pose, 7, 23 - pose), 12)
        draw.line((3, 21 - pose, 4, 21 - pose), fill=13)
    zombie_head(draw, cone, bite_pose, chewing)


def newspaper_zombie(draw, pose, chewing, shielded):
    # Preserve the runner's exposed brain, blue jacket, and original animation.
    with Image.open(Path(__file__).parent / 'art' / 'runner_base.png') as base:
        # The bite changes the jaw and grip, never the planted feet.
        frame = 0 if chewing else pose
        for y in range(32):
            for x in range(32):
                color = base.getpixel((x, frame * 32 + y))
                # The shielded poses use a new grip tied to the paper's motion.
                # Remove the runner's reaching hands, which swing in the opposite direction.
                if color == 2 and y >= 25: color = SHADOW_PIXEL
                # The outer jacket corner belongs to the blue cloth, not skin.
                if (x, y) == (22, 22) and color == 12: color = 15
                reaching_hand = shielded and x < 9 and 14 <= y <= 23
                if color and not reaching_hand:
                    draw.point((x, y), fill=color)
    for x in (10, 17):
        draw.rectangle((x, 10, x + 4, 14), outline=1)
    draw.line((14, 11, 17, 11), fill=1)
    if chewing:
        zombie_bite(draw, pose)
    if shielded:
        # Lower the paper to expose the biting mouth; both hands follow its edge.
        dy = 2 + pose if chewing else pose
        # The bent sleeve passes behind the paper; only the fingertips peek over it.
        # Preserve a shallow, beveled elbow even when the grip reaches shoulder
        # height; no pose collapses into one horizontal rectangular strip.
        sleeve_dy = 2 if chewing else 0
        sleeve = (21, 18, 20, 19, 18, 19 + sleeve_dy // 2,
                  16, 17 + sleeve_dy, 14, 16 + sleeve_dy)
        draw.line(sleeve, fill=1, width=3)
        draw.line(sleeve, fill=15)
        draw.polygon([(2, 15 + dy), (8, 16 + dy), (14, 14 + dy), (15, 23 + dy),
                      (9, 25 + dy), (3, 24 + dy)], fill=13, outline=1)
        draw.polygon([(3, 16 + dy), (8, 17 + dy), (8, 23 + dy), (4, 23 + dy)], fill=8)
        draw.line((9, 17 + dy, 9, 23 + dy), fill=12)
        draw.line((4, 18 + dy, 7, 18 + dy), fill=1)
        for y in (20, 22):
            draw.line((4, y + dy, 7, y + dy), fill=12)
            draw.line((11, y - 2 + dy, 13, y - 2 + dy), fill=12)
        for x, y in ((4, 14 + dy), (11, 13 + dy)):
            draw.rounded_rectangle((x, y, x + 4, y + 3), radius=1, fill=12, outline=1)
            # Explicitly fill the tiny palm: Pillow's rounded box can leave its
            # center pixel untouched at this size. Keep the original highlight.
            draw.rectangle((x + 1, y + 1, x + 3, y + 2), fill=12)
            draw.line((x + 1, y + 1, x + 3, y + 1), fill=13)
    else:
        draw.line((10, 9, 13, 10), fill=1)
        draw.line((18, 10, 20, 9), fill=1)


def flag_banner(draw, wind=0):
    draw.line((25, 1, 25, 25), fill=1, width=2)
    draw.line((25, 2, 25, 24), fill=13)
    # The free edge lifts and curls while the edge fastened to the pole stays put.
    edge = (0, 1, 2, 1)[wind]
    tip = (11, 12, 14, 12)[wind]
    draw.polygon([(tip, edge), (18, 1 + edge), (24, 0), (24, 6),
                  (18, 7 - edge), (tip, 6 - edge), (tip + 2, 3)], fill=11, outline=9)
    draw.line((tip + 3, 1 + edge, 18, 2 + edge, 23, 1), fill=14)
    draw.line((18, 3 + edge, 20, 4, 23, 3), fill=9)


def flag_grip(draw):
    # A bent sleeve reaches the pole; fingers wrap over it rather than stopping short.
    draw.line((21, 19, 22, 22, 25, 20), fill=1, width=4)
    draw.line((21, 19, 22, 21, 24, 20), fill=9, width=2)
    ellipse(draw, (23, 18, 27, 21), 12)
    draw.line((24, 19, 26, 19), fill=13)
    draw.point((26, 21), fill=1)


def flag_zombie(draw, pose, chewing, wind):
    zombie(draw, False, pose, chewing, carrying='flag')
    flag_banner(draw, wind)
    flag_grip(draw)


def bucket_hat(draw, damaged=False):
    # A narrow steel pail sits on the forehead; its wire handle arches behind it.
    draw.arc((10, 1, 23, 13), 180, 360, fill=1, width=3)
    draw.arc((11, 2, 22, 12), 180, 360, fill=5)
    draw.polygon([(12, 4), (20, 4), (22, 9), (10, 9)], fill=4, outline=1)
    ellipse(draw, (12, 3, 20, 5), 4)
    draw.line((13, 4, 19, 4), fill=6)
    draw.polygon([(12, 5), (13, 5), (12, 8), (11, 8)], fill=5)
    draw.polygon([(19, 5), (20, 5), (21, 8), (19, 8)], fill=3)
    ellipse(draw, (10, 9, 23, 11), 3)
    draw.line((11, 10, 22, 10), fill=5)
    draw.point((11, 7), fill=6)
    draw.point((22, 7), fill=6)
    draw.point((18, 6), fill=3)
    draw.point((19, 6), fill=6)
    if damaged:
        draw.line((17, 5, 16, 6, 18, 7, 17, 9), fill=3)
        draw.point((18, 7), fill=6)
        draw.line((12, 8, 14, 8), fill=3)


def bucket_zombie(draw, pose, chewing, damaged):
    zombie(draw, False, pose, chewing)
    bucket_hat(draw, damaged)

def football_helmet(draw, damaged=False):
    draw.polygon([(8, 9), (8, 6), (11, 3), (19, 3), (24, 6), (25, 11),
                  (21, 12), (20, 8), (12, 8)], fill=11, outline=1)
    draw.line((14, 4, 17, 4, 19, 7), fill=8, width=2)
    draw.line((10, 5, 12, 4), fill=11)
    draw.polygon([(21, 9), (24, 9), (24, 16), (21, 16), (19, 14)], fill=11, outline=1)
    draw.ellipse((21, 11, 23, 13), fill=8)
    draw.line((8, 11, 6, 12, 7, 16, 16, 17, 20, 14), fill=1, width=2)
    draw.line((7, 12, 8, 15, 16, 16, 20, 14), fill=13)
    if damaged:
        draw.line((20, 4, 19, 6, 21, 8), fill=1)
        draw.point((11, 6), fill=13)


def football_zombie(draw, pose, chewing, helmet):
    step = 0 if chewing else pose % 2
    zombie(draw, False, step, chewing, carrying='athlete')
    # A short bent forearm and the same rounded hand used by other zombies.
    hand_y = 20 + pose % 2
    arm_y = hand_y if not chewing else 20
    draw.line((12, 19, 10, arm_y, 9, arm_y), fill=1, width=3)
    draw.line((12, 19, 10, arm_y, 9, arm_y), fill=11)
    # Cover the sleeve root with the jersey so its moving outline never cuts
    # across the torso. Keep the trousers and raised arm in their original order.
    # Athlete bodies omit the ordinary jacket and its shirt folds entirely.
    draw.polygon([(11, 17), (14, 16), (21, 16), (24, 18), (24, 22),
                  (21, 24), (13, 23), (10, 20)], fill=11, outline=1)
    draw.line((12, 17, 14, 17), fill=8)
    jersey_skull(draw, 15, 18)
    # A stable red sleeve opening joins the torso to the visible forearm.
    # Keep its black contour outside the jersey, and the hand outside the sleeve.
    draw.rectangle((11, 20, 12, 21), fill=11)
    draw.line((11, 22, 12, 22), fill=1)
    # Swing the exposed forearm and its underside with the hand. The shoulder
    # opening stays fixed so no animated dark edge crosses the torso.
    draw.rectangle((10, arm_y, 10, arm_y + 1), fill=11)
    draw.point((10, arm_y + 2), fill=1)
    ellipse(draw, (5, hand_y - 1, 9, hand_y + 2), 12)
    draw.line((6, hand_y, 8, hand_y), fill=13)
    left, right = 11 - step * 2, 20 + step
    draw.polygon([(13, 23), (17, 23), (left + 3, 26), (left, 26)], fill=8, outline=1)
    draw.polygon([(18, 23), (22, 23), (right + 1, 26), (right - 2, 26)], fill=8, outline=1)
    if not chewing and pose >= 2:
        draw.line((left, 26, left - 2, 27), fill=1, width=2)
        draw.line((21, 25, 23, 24), fill=1, width=2)
    if not chewing:
        # The far arm is bent upward, fist beside the helmet, in a charging pose.
        draw.line((23, 19, 25, 18, 26, 16), fill=1, width=3)
        draw.line((23, 19, 25, 18, 26, 16), fill=11)
        ellipse(draw, (24, 12, 28, 15), 12)
        draw.line((25, 13, 27, 13), fill=13)
    else:
        draw.line((23, 19, 24, 20, 23, 20), fill=1, width=3)
        draw.line((23, 19, 24, 20, 23, 20), fill=11)
        ellipse(draw, (21, 19, 25, 22), 12)
        draw.line((22, 20, 24, 20), fill=13)
    zombie_head(draw, False, pose % 2, chewing)
    if helmet < 2:
        football_helmet(draw, helmet == 1)


def zombie_hand(draw, left, top, width=6):
    """Standard rounded palm: skin fill, a thin outline and two lit knuckles."""
    ellipse(draw, (left, top, left + width - 1, top + 3), 12)
    draw.line((left + 1, top + 1, left + 2, top + 1), fill=13)


def screen_door_mesh(draw, damaged):
    """Silver wires with explicit openings, independent of the zombie skin."""
    def wire(x1, y1, x2, y2):
        draw.line((x1, y1, x2, y2), fill=2)

    for x in (4, 7, 10, 13):
        if damaged and x in (7, 10):
            wire(x, 9, x, 13)
            wire(x, 20, x, 22)
        else:
            wire(x, 9, x, 22)
    for y in (10, 13, 16, 19, 22):
        if damaged and y in (16, 19):
            wire(3, y, 5, y)
            wire(11, y, 13, y)
        else:
            wire(3, y, 13, y)


def screen_door_zombie(image, pose, chewing, damaged, canvas):
    # Eating moves the intact upright door aside, with a small grip-led bob.
    slide = (0, 1, 2, 1)[pose] if chewing else 0
    bob = (0, 0, 1, 1)[pose] if chewing else pose

    def door_point(x, y):
        return x - slide, y + bob

    body = canvas(32, 32)
    draw = ImageDraw.Draw(body)
    face_pose = int(pose == 2) if chewing else pose
    zombie(draw, False, face_pose, chewing, carrying='door')
    far_x, far_y = door_point(1, 22)
    draw.line((12, 19, 9, 21, far_x, far_y), fill=1, width=3)
    draw.line((12, 19, 9, 21, far_x, far_y), fill=9)
    grip_x, grip_y = 14 - slide, 20 + bob
    # A gentle idle bend follows the grip instead of dipping below the wrist.
    elbow_y = 19 + bob
    draw.line((21, 18, 20, elbow_y, grip_x + 3, grip_y), fill=1, width=3)
    draw.line((21, 18, 20, elbow_y, grip_x + 3, grip_y), fill=9)
    image.paste(body, (16, 0))
    # The small palm sits outside the left stile, behind the door's edge.
    # Only curled fingertips are drawn over the metal after projection.
    zombie_hand(ImageDraw.Draw(image), far_x + 14, far_y - 1, width=4)

    door = canvas(32, 32)
    draw = ImageDraw.Draw(door)
    draw.rounded_rectangle((1, 6, 15, 27), radius=1, outline=1, width=2)
    draw.rectangle((2, 7, 14, 26), outline=13)
    screen_door_mesh(draw, damaged)
    draw.line((3, 23, 13, 23), fill=13)
    draw.rectangle((3, 24, 13, 25), fill=4)
    draw.line((4, 24, 10, 24), fill=13)
    image.paste(door, (16 - slide, bob), door.point([0] + [255] * 255, 'L'))
    # Fingers wrap around the frame; no vertical grey cuff across the sleeve.
    draw = ImageDraw.Draw(image)
    gx = grip_x + 16
    zombie_hand(draw, gx - 2, grip_y - 1)
    draw.line((far_x + 16, far_y, far_x + 17, far_y), fill=12)
    draw.point((far_x + 16, far_y + 1), fill=12)


def potato_mine(draw, frame):
    if frame == 28:
        draw.polygon([(3, 25), (7, 19), (4, 13), (12, 16), (16, 7),
                      (20, 16), (27, 12), (25, 20), (30, 26)], fill=10, outline=9)
        ellipse(draw, (9, 17, 24, 27), 7, 10)
        draw.ellipse((12, 20, 21, 25), fill=8)
        return
    armed = frame != 25
    top = 16 if armed else 22
    shadow(draw, (6, 24, 26, 26))
    ellipse(draw, (9, top, 24, 26), 10, 1)
    draw.arc((10, top + 1, 23, 25), 145, 280, fill=7)
    draw.arc((10, top, 23, 26), 10, 65, fill=9)
    draw.line((12, top + 1, 16, top + 1), fill=7)
    draw.point((11, top + 2), fill=8)
    draw.point((22, 24), fill=9)
    draw.line((17, top - 4, 17, top), fill=9, width=2)
    draw.line((17, top - 3, 17, top - 1), fill=13)
    draw.line((16, top - 2, 18, top - 2), fill=12)
    ellipse(draw, (14, top - 7, 20, top - 3), 7 if frame == 27 else (11 if armed else 12), 13)
    draw.point((16, top - 6), fill=8 if armed else 13)
    if armed:
        draw.polygon([(11, 18), (14, 17), (17, 17), (15, 19),
                      (12, 20), (11, 22), (10, 21)], fill=7)
        draw.line((12, 18, 13, 18), fill=8)
        for x in (12, 19):
            draw.ellipse((x, 19, x + 2, 22), fill=8)
            draw.line((x + 1, 20, x + 1, 21), fill=1)
        for point in ((10, 22), (22, 20), (21, 24)):
            draw.point(point, fill=9)
        draw.point((12, 24), fill=7)
        draw.point((18, 18), fill=7)
        draw.line((21, 18, 22, 21), fill=9)
        draw.line((19, 25, 21, 24), fill=9)
        draw.line((15, 23, 16, 24, 18, 24, 19, 23), fill=9)


def zombie_bite(draw, pose):
    """One connected jaw shape, shared by normal and exposed-brain faces."""
    # Keep the jaw attached to the face. No bright single-pixel tooth below
    # the closed lip: the mouth opens as one shape, then closes into a crease.
    draw.rectangle((11, 15, 15, 17), fill=12)
    if pose:
        draw.polygon([(12, 15), (14, 15), (15, 16), (11, 16)], fill=1)
    else:
        draw.line((11, 16, 14, 16), fill=1)


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
    if chewing:
        zombie_bite(draw, pose)
    if not cone:
        draw.line((12, 8, 13, 7), fill=1)
        draw.point((16, 7), fill=1)
        draw.point((20, 8), fill=1)
    else:
        traffic_cone(draw)


def actor(frame, canvas, separate_shadow=False):
    image = canvas(64 if 500 <= frame < 512 else 32, 32)
    draw = ImageDraw.Draw(image)
    if 500 <= frame < 512:
        local = (frame - 500) % 6
        screen_door_zombie(image, local - 2 if local >= 2 else local,
                           local >= 2, frame >= 506, canvas)
    elif 400 <= frame < 411:
        chomper(draw, frame - 400)
    elif 300 <= frame < 312:
        pole_vaulter(draw, frame - 300)
    elif 260 <= frame < 265:
        squash(draw, frame - 260)
    elif 280 <= frame < 286:
        tallnut(draw, frame - 280)
    elif 270 <= frame < 276:
        repeater(draw, frame in (272, 273), int(frame in (271, 275)))
        if frame in (272, 274):
            ellipse(draw, (10, 15, 14, 18), 5, 4)
            draw.point((11, 15), fill=6)
        if frame == 272:
            ellipse(draw, (23, 12, 29, 18), 5, 4)
            ellipse(draw, (26, 13, 28, 17), 2, 4)
            draw.line((24, 13, 26, 13), fill=6)
    elif 200 <= frame < 218:
        local = frame - 200
        football_zombie(draw, local % 6, local % 6 >= 4, local // 6)
    elif frame in (0, 12):
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
    elif frame in (25, 26, 27, 28):
        potato_mine(draw, frame)
    elif 29 <= frame <= 36:
        local = (frame - 29) % 4
        newspaper_zombie(draw, local % 2, local >= 2, frame < 33)
    elif 37 <= frame <= 52:
        local = (frame - 37) % 4
        flag_zombie(draw, local % 2, local >= 2, (frame - 37) // 4)
    elif 69 <= frame <= 76:
        local = (frame - 69) % 4
        bucket_zombie(draw, local % 2, local >= 2, frame >= 73)
    elif 101 <= frame <= 104:
        if frame == 104:
            ellipse(draw, (13, 13, 18, 18), 5, 4)
            draw.line((14, 14, 16, 14), fill=6)
            draw.point((14, 15), fill=8)
            draw.point((10, 16), fill=6)
            draw.point((8, 14), fill=5)
        else:
            snow_peashooter(draw, frame == 103, int(frame == 102))
    elif frame in (4, 5, 14, 15, 16, 17, 18, 19):
        cone = frame % 2 == 1
        pose = int(frame in (14, 15, 18, 19))
        zombie(draw, cone, pose, frame >= 16)
    else:
        raise ValueError(f'Not a character frame: {frame}')
    if not separate_shadow:
        image = image.point([2 if index == SHADOW_PIXEL else index for index in range(256)])
    return image


def character_layers(frame, canvas):
    """Return independently editable body pixels and the ground-shadow mask."""
    image = actor(frame, canvas, separate_shadow=True)
    body = image.point([0 if index == SHADOW_PIXEL else index for index in range(256)])
    mask = image.point([15 if index == SHADOW_PIXEL else 0 for index in range(256)])
    return body, mask


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
    # Broader porch boards and a raised timber lip against the stone path.
    draw.rectangle((0, 57, 13, 153), fill=9)
    for y in range(58, 154, 16):
        draw.line((0, y, 10, y), fill=10)
        draw.line((0, min(y + 14, 153), 10, min(y + 14, 153)), fill=1)
        draw.line((2, y + 4, 6, y + 4), fill=10)
        if y + 10 <= 153:
            draw.line((5, y + 10, 9, y + 9), fill=10)
        draw.point((1, y + 2), fill=12)
    draw.rectangle((11, 57, 13, 153), fill=9)
    draw.line((11, 57, 11, 153), fill=10)
    draw.line((14, 57, 14, 153), fill=1)
    # One uninterrupted slab per lane keeps joints clear of parked mower wheels.
    draw.rectangle((15, 57, 32, 153), fill=2)
    for y in range(58, 153, 19):
        bottom = y + 17
        draw.rounded_rectangle((15, y, 31, bottom), radius=2, fill=12, outline=2)
        draw.rounded_rectangle((16, y + 1, 30, bottom - 2), radius=1, fill=13)
        draw.line((18, y + 1, 28, y + 1), fill=8)
        draw.line((17, bottom - 1, 29, bottom - 1), fill=12)
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
    # Small turf overhangs soften the stone-to-lawn seam, clear of plant centers.
    for y in range(61, 151, 7):
        draw.polygon([(33, y - 2), (31, y), (32, y + 2), (34, y + 1)], fill=3)
        draw.line((32, y, 33, y - 2), fill=5)
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

    draw_sidewalk(image)


def draw_sidewalk(image, right=239):
    """The lawn's road edge continues into the scouting panorama without a seam."""
    draw = ImageDraw.Draw(image)
    draw.rectangle((234, 57, right, 159), fill=13)
    draw.line((234, 57, 234, 159), fill=12)
    for y in range(58, 160, 19):
        draw.line((236, y, right, y), fill=12)
        draw.line((236, y + 1, right, y + 1), fill=8)
    # A few blades overhang the concrete rather than ending at a ruler-straight edge.
    for y in (65, 103, 141):
        draw.line((233, y + 2, 235, y), fill=3)
        draw.point((234, y + 1), fill=5)
        draw.point((235, y), fill=4)
