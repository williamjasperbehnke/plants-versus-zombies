"""Shared, palette-limited graveyard scenery and carved menu furniture."""
from PIL import ImageDraw


def graveyard(image, moon_origin=(7, 9)):
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, 239, 159), fill=3)
    moon_x, moon_y = moon_origin
    draw.ellipse((moon_x, moon_y, moon_x + 23, moon_y + 23), fill=6)
    draw.ellipse((moon_x + 7, moon_y - 2, moon_x + 26, moon_y + 17), fill=3)
    for x, top, width in ((-8, 57, 25), (220, 68, 26), (5, 93, 14)):
        draw.rounded_rectangle((x, top, x + width, 125), radius=width // 2, fill=2)
    draw.rectangle((0, 122, 239, 159), fill=4)
    draw.polygon([(0, 123), (53, 116), (105, 123), (165, 117), (239, 122),
                  (239, 135), (0, 135)], fill=4)
    for x in range(3, 240, 13):
        y = 126 + (x * 7 % 21)
        draw.line((x, y, x - 2, y - 4), fill=2)
        draw.line((x, y, x + 2, y - 5), fill=5)
    for x in (8, 226):
        for y in (125, 136, 145):
            draw.ellipse((x - 4, y, x, y + 3), fill=5)


def tombstone(image, box, crown_radius=22):
    draw = ImageDraw.Draw(image)
    left, top, right, bottom = box
    radius = min(crown_radius, (right - left) // 2, (bottom - top) // 2)
    draw.rounded_rectangle((left + 3, top + 4, right + 3, bottom + 3), radius=radius, fill=2)
    draw.rounded_rectangle(box, radius=radius, fill=12, outline=1, width=2)
    draw.rounded_rectangle((left + 3, top + 3, right - 3, bottom - 3), radius=radius - 2,
                           outline=13)
    # Square the foot, leaving the arch only at the crown.
    draw.rectangle((left, bottom - radius, right, bottom), fill=12, outline=1, width=2)
    draw.rectangle((left + 2, bottom - radius, right - 2, bottom - 2), fill=12)
    draw.line((left + 3, bottom - radius, left + 3, bottom - 3), fill=13)
    for x, y, direction in ((left + 8, top + 22, 1), (right - 7, bottom - 21, -1)):
        draw.line((x, y, x + direction * 3, y + 4, x + direction, y + 8), fill=2)
    draw.rectangle((left - 2, bottom - 1, right + 2, bottom + 3), fill=2)
    for x in range(left + 3, right, 9):
        draw.line((x, bottom, x + 2, bottom - 3), fill=3)


def wooden_sign(image, box):
    draw = ImageDraw.Draw(image)
    left, top, right, bottom = box
    draw.polygon([(left + 3, top), (right - 2, top + 1), (right, bottom - 2),
                  (right - 3, bottom), (left, bottom - 1), (left + 1, top + 3)], fill=9, outline=1)
    draw.line((left + 4, top + 2, right - 4, top + 3), fill=10)
    draw.line((left + 4, bottom - 3, right - 4, bottom - 2), fill=1)
    for x in (left + 5, right - 6):
        draw.point((x, top + 5), fill=13)
        draw.point((x, bottom - 5), fill=1)
