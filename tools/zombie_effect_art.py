"""Damage and death poses derived from the existing indexed zombie artwork."""
from PIL import Image, ImageDraw
from asset_layout import asset_path
from pixel_art import traffic_cone, bucket_hat, flag_banner, flag_grip, zombie, football_helmet, vaulting_pole


def blank(width, height, palette):
    image = Image.new('P', (width, height), 0)
    image.putpalette(palette)
    return image


def wounded_atlas(source, flag=False, coat=9):
    result = source.copy()
    draw = ImageDraw.Draw(result)
    for top in range(0, source.height, 32):
        if flag:
            draw.rectangle((0, top + 14, 9, top + 19), fill=0)
        else:
            draw.rectangle((0, top + 19, 10, top + 24), fill=0)
        # A tiny off-white bone tip projects from the ragged sleeve.
        y = top + (18 if flag else 20)
        draw.point((11, y), fill=coat)
        draw.point((10, y), fill=8)
        draw.point((10, y + 1), fill=13)
    return result


def death_atlas(whole, wounded, coat=9, flag=False):
    palette = whole.getpalette()
    angles = (0, -5, -15, -30, -50, -70, -90, -90, -90, -90) if flag else (0, 0, 25, 55, 85, 90, 90, 90)
    squashes = (100, 95, 90, 85, 75, 65, 55, 45, 45, 45) if flag else (100, 85, 90, 75, 55, 45, 45, 45)
    stages = len(angles)
    atlas = blank(32, 32 * stages * 2, palette)
    for lost_arm, source in enumerate((whole, wounded)):
        body = source.point([0 if color == 15 else color for color in range(256)])
        resting = body.copy()
        # Reaching hands must not become the low point after rotation: that held
        # the head/torso in the air. Fold the arms along the body before the fall.
        rd = ImageDraw.Draw(resting)
        rd.rectangle((0, 15, 10, 24), fill=0)
        rd.rectangle((23, 18, 31, 24), fill=0)
        for stage, angle in enumerate(angles):
            image = blank(32, 32, palette)
            draw = ImageDraw.Draw(image)
            landed = stage >= stages - 4
            draw.ellipse((3, 27, 29, 29) if landed else (8, 25, 24, 27), fill=15)
            pose = resting if stage >= (3 if flag else 2) else body
            pose = pose.crop(pose.getbbox())
            pose = pose.rotate(angle, resample=Image.Resampling.NEAREST, expand=True, fillcolor=0)
            pose = pose.crop(pose.getbbox())
            pose.thumbnail((30, 27), Image.Resampling.NEAREST)
            width = min(30, pose.width + (2 if landed else 0))
            pose = pose.resize((width, max(1, pose.height * squashes[stage] // 100)), Image.Resampling.NEAREST)
            # The head and torso share the same contact plane as the feet.
            image.paste(pose, (16 - pose.width // 2, 28 - pose.height), pose.point([0] + [255] * 255, 'L'))
            if landed:
                draw.line((13, 26, 18, 27), fill=1, width=3)
                draw.line((13, 26, 17, 27), fill=coat)
                draw.line((18, 27, 21, 27), fill=12)
                draw.point((20, 26), fill=13)
            if stage >= stages - 2:
                for y in range(32):
                    for x in range(32):
                        if (x + y) % 2 or (stage == stages - 1 and x % 2):
                            image.putpixel((x, y), 0)
            atlas.paste(image, (0, (lost_arm * stages + stage) * 32))
    return atlas


def debris_atlas(palette):
    colors = list(palette)
    # Steel colors occupy slots unused by the isolated cloth/paper/cone artwork.
    for slot, rgb in enumerate(((61, 72, 80), (115, 130, 143), (185, 198, 208), (226, 232, 232)), 3):
        colors[slot * 3:slot * 3 + 3] = rgb
    atlas = blank(32, 32 * 60, colors)
    for kind in range(10):
        piece = blank(32, 32, colors)
        draw = ImageDraw.Draw(piece)
        if kind in (0, 1, 7):
            coat = 11 if kind == 7 else (9 if kind == 0 else 3)
            draw.polygon([(11, 14), (16, 15), (17, 17), (11, 17)], fill=coat, outline=1)
            draw.line((12, 15, 15, 16), fill=11 if kind == 7 else (10 if kind == 0 else 3))
            draw.line((16, 15, 16, 17), fill=8)
            draw.rounded_rectangle((17, 15, 20, 18), radius=1, fill=12, outline=1)
            draw.line((18, 16, 19, 16), fill=13)
        elif kind == 2:
            traffic_cone(draw)
        elif kind == 3:
            draw.polygon([(6, 8), (16, 10), (24, 7), (25, 22), (16, 25), (7, 23)], fill=13, outline=1)
            draw.polygon([(8, 10), (15, 12), (15, 22), (9, 21)], fill=8)
            draw.line((17, 12, 17, 22), fill=12)
            for y in (13, 16, 19):
                draw.line((9, y, 13, y), fill=12)
                draw.line((19, y, 23, y - 1), fill=1 if y == 13 else 12)
        elif kind == 4:
            bucket_hat(draw)
        elif kind == 6:
            flag_banner(draw)
        elif kind == 8:
            football_helmet(draw)
        elif kind == 9:
            draw.line((2, 15, 29, 15), fill=1, width=3)
            draw.line((2, 15, 29, 15), fill=10)
        else:
            draw.rounded_rectangle((8, 4, 22, 27), radius=1, outline=1, width=2)
            draw.rectangle((9, 5, 21, 26), outline=13)
            for x in (12, 15, 18):
                draw.line((x, 7, x, 24), fill=12)
            for y in (10, 14, 18, 22):
                draw.line((10, y, 20, y), fill=12)
        if kind in (0, 1, 7):
            # The missing arm reaches left: its hand must stay left of the sleeve.
            # Positive rotation then swings the hand downward from the shoulder.
            piece = piece.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
        piece = piece.crop(piece.getbbox())
        angles = ((0, 12, 25, 35, 15, 0) if kind in (0, 1, 7) else
                  (0, -15, -35, -60, -85, -90) if kind == 6 else (0, 25, 70, 125, 175, 195))
        for stage, angle in enumerate(angles):
            pose = piece.rotate(angle, resample=Image.Resampling.NEAREST, expand=True, fillcolor=0)
            frame = blank(32, 32, colors)
            if pose.width > 30 or pose.height > 30:
                pose.thumbnail((30, 30), Image.Resampling.NEAREST)
            frame.paste(pose, (16 - pose.width // 2, 16 - pose.height // 2), pose.point([0] + [255] * 255, 'L'))
            atlas.paste(frame, (0, (kind * 6 + stage) * 32))
    return atlas


def generate_zombie_effects(save):
    atlases = {}
    for name in ('normal_zombies', 'cone_zombies', 'newspaper_zombies', 'flag_zombies', 'football_zombies', 'vaulting_zombies'):
        with Image.open(asset_path(name)) as source:
            atlas = source.copy()
        damaged = wounded_atlas(atlas, name == 'flag_zombies', 3 if name == 'newspaper_zombies' else (11 if name in ('football_zombies', 'vaulting_zombies') else 9))
        if name == 'vaulting_zombies':
            # Arm removal must never punch a hole in the equipment layer.
            for index in range(atlas.height // 32):
                state = index % 12
                if state < 4:
                    pose = damaged.crop((0, index * 32, 32, (index + 1) * 32))
                    draw = ImageDraw.Draw(pose)
                    # The compact grip reaches one pixel closer to the torso.
                    # Clear its last column without cutting the jersey or leg.
                    draw.rectangle((11, 19, 11, 23), fill=0)
                    draw.point((11, 20), fill=11)
                    vaulting_pole(draw, state, wounded=True)
                    damaged.paste(pose, (0, index * 32))
        save(damaged, 'wounded_' + name, {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
        atlases[name] = (atlas, damaged)
    for name, source_name, frame in (('zombie_deaths', 'normal_zombies', 0),
                                     ('newspaper_deaths', 'newspaper_zombies', 4),
                                     ('football_deaths', 'football_zombies', 12),
                                     ('vault_deaths', 'vaulting_zombies', 4)):
        normal, damaged = atlases[source_name]
        box = (0, frame * 32, 32, (frame + 1) * 32)
        save(death_atlas(normal.crop(box), damaged.crop(box), 3 if source_name == 'newspaper_zombies' else (11 if source_name in ('football_zombies', 'vaulting_zombies') else 9)), name,
             {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    # Keep the flag carrier's grip, but release the pole into its own falling sprite.
    palette = atlases['normal_zombies'][0].getpalette()
    carrier = blank(32, 32, palette)
    draw = ImageDraw.Draw(carrier)
    zombie(draw, False, 0, False, carrying='flag')
    flag_grip(draw)
    carrier = carrier.point([15 if color == 16 else 3 if color == 15 else color for color in range(256)])
    carrier.putpalette(palette)
    save(death_atlas(carrier, wounded_atlas(carrier, True), flag=True), 'flag_deaths',
         {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
    save(debris_atlas(atlases['normal_zombies'][0].getpalette()), 'zombie_debris',
         {'type': 'sprite', 'height': 32, 'bpp_mode': 'bpp_4'})
