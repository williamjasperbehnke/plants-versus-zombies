"""Refresh documentation images after running the current ROM through rom_smoke."""
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build'
DOCS = ROOT / 'docs'
NEAREST = Image.Resampling.NEAREST
BACKGROUND = (75, 119, 56)


def capture(name):
    path = BUILD / f'{name}.ppm'
    if not path.exists():
        raise SystemExit(f'Missing {path.name}: run build/rom_smoke first.')
    rom = ROOT / 'plants_vs_zombies.gba'
    if not rom.exists() or path.stat().st_mtime < rom.stat().st_mtime:
        raise SystemExit(f'{path.name} needs a fresh ROM capture: rebuild and run build/rom_smoke.')
    return Image.open(path).convert('RGB')


def enlarged(image, scale):
    return image.resize((image.width * scale, image.height * scale), NEAREST)


def save_capture(source, target=None):
    enlarged(capture(source), 3).save(DOCS / f'{target or source}.png')


def sprite_on_grass(sprite, scale=6):
    background = Image.new('RGB', sprite.size, BACKGROUND)
    background.paste(sprite.convert('RGB'), (0, 0), sprite.point([0] + [255] * 255, 'L'))
    return enlarged(background, scale)


def strip(images, labels):
    width = sum(image.width for image in images)
    result = Image.new('RGB', (width, max(image.height for image in images) + 24), (17, 30, 29))
    draw = ImageDraw.Draw(result)
    x = 0
    for image, label in zip(images, labels):
        draw.text((x + 6, 6), label, fill=(255, 241, 191))
        result.paste(image, (x, 24))
        x += image.width
    return result


def main():
    DOCS.mkdir(exist_ok=True)
    for source, target in [('playing', 'gameplay'), ('wave_progress', 'waves')]:
        save_capture(source, target)
    for name in ('title', 'paused', 'defeat', 'shovel', 'cooldown', 'unaffordable',
                 'chewing', 'wave_checkpoint', 'occupied', 'explosion',
                 'audio_master_muted', 'audio_music_muted', 'sun_collection_before',
                 'sun_collection_start', 'sun_collection_mid', 'sun_collection_arrived'):
        save_capture(name)
    for name in ('level_1', 'level_2', 'level_3', 'level_1_clear', 'level_2_clear',
                 'level_3_clear', 'mine_arming', 'mine_armed', 'mine_seed_bank', 'campaign_title', 'runner_gameplay'):
        save_capture(name)
    strip([enlarged(capture(f'level_{level}'), 2) for level in (1, 2, 3)],
          ['Front Yard', 'Cone Trouble', 'Last Stand']).save(DOCS / 'campaign.png')
    enlarged(capture('mine_seed_bank').crop((44, 22, 236, 54)), 4).save(DOCS / 'mine_seed_packets.png')
    enlarged(capture('seed_bank_more').crop((44, 22, 236, 54)), 4).save(DOCS / 'seed_bank_more.png')
    strip([enlarged(capture(name), 2) for name in ('title', 'paused', 'defeat')],
          ['Title', 'Pause / audio', 'Results']).save(DOCS / 'menus.png')
    title_frames = [enlarged(capture(f'title_animation_{frame:02d}'), 3) for frame in range(12)]
    title_frames[0].save(DOCS / 'title.gif', save_all=True, append_images=title_frames[1:],
                         duration=200, loop=0)
    enlarged(capture('playing').crop((52, 22, 224, 54)), 4).save(DOCS / 'seed_packets.png')
    enlarged(capture('playing').crop((0, 57, 45, 154)), 5).save(DOCS / 'lawn_edge.png')
    enlarged(capture('wave_checkpoint').crop((50, 0, 240, 25)), 4).save(DOCS / 'wave_indicator.png')
    enlarged(capture('wave_progress').crop((207, 0, 228, 24)), 8).save(DOCS / 'final_flag.png')

    atlas = Image.open(ROOT / 'graphics' / 'actors.bmp')
    icons = Image.open(ROOT / 'graphics' / 'hud_icons.bmp')
    newspapers = Image.open(ROOT / 'graphics' / 'newspaper_zombies.bmp')
    flags = Image.open(ROOT / 'graphics' / 'flag_zombies.bmp')

    def actor(frame):
        if frame >= 37:
            return flags.crop((0, (frame - 37) * 32, 32, (frame - 36) * 32))
        if frame >= 29:
            return newspapers.crop((0, (frame - 29) * 32, 32, (frame - 28) * 32))
        return atlas.crop((0, frame * 32, 32, frame * 32 + 32))

    sprite_on_grass(actor(7).crop((8, 8, 24, 24)), 8).save(DOCS / 'sun.png')
    sprite_on_grass(actor(8).crop((8, 8, 24, 24)), 8).save(DOCS / 'mower.png')
    sprite_on_grass(actor(5).crop((7, 2, 25, 20)), 8).save(DOCS / 'cone.png')
    sprite_on_grass(actor(1).crop((8, 9, 29, 28)), 8).save(DOCS / 'peashooter.png')
    sprite_on_grass(icons.crop((0, 32, 16, 48)), 8).save(DOCS / 'shovel_icon.png')
    zombie_frames = (4, 14, 16, 18, 5, 15, 17, 19, 29, 30, 31, 32)
    strip([sprite_on_grass(actor(frame), 3) for frame in zombie_frames],
          ['Walk 1', 'Walk 2', 'Eat 1', 'Eat 2', 'Cone walk 1', 'Cone walk 2',
           'Cone eat 1', 'Cone eat 2', 'Runner walk 1', 'Runner walk 2',
           'Runner eat 1', 'Runner eat 2']).save(DOCS / 'zombies.png')
    strip([sprite_on_grass(actor(frame), 3) for frame in (25, 26, 27, 28)],
          ['Arming', 'Ready', 'Blink', 'Burst']).save(DOCS / 'potato_mine.png')
    runner = [sprite_on_grass(actor(frame), 6) for frame in (29, 30)]
    runner[0].save(DOCS / 'runner.gif', save_all=True, append_images=runner[1:],
                   duration=100, loop=0)
    sprite_on_grass(actor(29), 6).save(DOCS / 'runner.png')
    eating = [sprite_on_grass(actor(frame), 6) for frame in (16, 18)]
    eating[0].save(DOCS / 'zombie_eating.gif', save_all=True, append_images=eating[1:],
                   duration=[470, 200], loop=0)
    idle = []
    for frames in ((0, 1, 2, 3, 26, 5, 29), (12, 20, 21, 24, 27, 15, 30)):
        idle.append(strip([sprite_on_grass(actor(frame), 3) for frame in frames],
                          ['Sunflower', 'Peashooter', 'Wall-nut', 'Cherry', 'Potato Mine', 'Cone zombie', 'Runner']))
    idle[0].save(DOCS / 'plant_idle.png')
    idle[0].save(DOCS / 'plant_idle.gif', save_all=True, append_images=idle[1:],
                 duration=400, loop=0)
    strip([enlarged(capture(name), 2) for name in ('sun_collection_before',
          'sun_collection_start', 'sun_collection_mid', 'sun_collection_arrived')],
          ['Before collecting', 'Flight starts', 'In flight', 'Arrived']).save(DOCS / 'sun_collection.png')
    # Retire the historical comparison: previews now show current assets only.
    (DOCS / 'graphics_comparison.png').unlink(missing_ok=True)
    print('Documentation captures and asset previews refreshed.')


if __name__ == '__main__':
    main()
