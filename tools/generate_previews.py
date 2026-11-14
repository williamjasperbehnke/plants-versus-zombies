"""Publish fresh emulator captures from the normal, debug, and loss smoke checks."""
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build'
DOCS = ROOT / 'docs'
NEAREST = Image.Resampling.NEAREST

# Each capture is checked against the ROM that produces it. Keep this list in
# sync with tools/rom_smoke.c; never silently publish captures from an older ROM.
CAPTURES = {
    'title': ('title', False),
    'gameplay': ('playing', False),
    'paused': ('paused', False),
    'levels_locked': ('levels_locked', False),
    'levels_progress': ('levels_progress', False),
    'preview_level_1': ('preview_level_1', False),
    'preview_level_10': ('preview_level_10', True),
    'almanac_plants': ('almanac_plants', False),
    'almanac_zombies': ('almanac_zombies', False),
    'almanac_sunflower': ('almanac_plant_0', False),
    'almanac_screen_door': ('almanac_screen_door', False),
    'almanac_lifetime_kills': ('almanac_lifetime_kills', False),
    'plant_reward': ('plant_reward', False),
    'plant_reward_mid': ('plant_reward_mid', False),
    'trophy_reward': ('trophy_reward', False),
    'loss_animation': ('loss_animation', False),
    'loss_prompt': ('loss_prompt', False),
    'snow_pea_gameplay': ('snow_pea_gameplay', True),
    'plant_red_flash': ('plant_red_flash', True),
    'zombie_damage_flash': ('zombie_damage_flash', True),
    'cooldown_cell': ('cooldown_cell', False),
    'tombstones_lawn': ('tombstones_lawn', True),
    'tombstone_cracked': ('tombstone_cracked', True),
    'tombstone_destroyed': ('tombstone_destroyed', True),
    'explosion_above_plants': ('explosion_above_plants', True),
    'grave_emergence': ('grave_emergence_2', True),
}


def capture(name, debug=False):
    path = BUILD / f'{name}.ppm'
    rom = ROOT / ('plants_vs_zombies_debug.gba' if debug else 'plants_vs_zombies.gba')
    if not path.exists() or not rom.exists() or path.stat().st_mtime < rom.stat().st_mtime:
        raise SystemExit(f'{path.name} needs a fresh capture. See docs/README.md for smoke commands.')
    with Image.open(path) as source:
        return source.convert('RGB')


def enlarged(image, scale=3):
    return image.resize((image.width * scale, image.height * scale), NEAREST)


def strip(images, labels):
    result = Image.new('RGB', (sum(image.width for image in images),
                              max(image.height for image in images) + 24), (17, 30, 29))
    draw = ImageDraw.Draw(result)
    x = 0
    for image, label in zip(images, labels):
        draw.text((x + 6, 6), label, fill=(255, 241, 191))
        result.paste(image, (x, 24))
        x += image.width
    return result


def main():
    # Validate every input before replacing any documentation images.
    images = {target: capture(source, debug) for target, (source, debug) in CAPTURES.items()}
    reward_frames = [enlarged(capture(f'reward_{frame:02d}')) for frame in range(36)]
    emergence = [enlarged(capture(f'grave_emergence_{stage}', True)) for stage in range(6)]
    DOCS.mkdir(exist_ok=True)
    for name, image in images.items():
        enlarged(image).save(DOCS / f'{name}.png')
    strip([enlarged(images[name], 2) for name in ('title', 'paused', 'levels_locked')],
          ['Title', 'Pause / audio', 'Backyard levels']).save(DOCS / 'menus.png')
    enlarged(images['gameplay'].crop((52, 22, 224, 54)), 4).save(DOCS / 'seed_packets.png')
    enlarged(images['gameplay'].crop((0, 57, 45, 154)), 5).save(DOCS / 'lawn_edge.png')
    reward_frames[0].save(DOCS / 'plant_reward.gif', save_all=True,
                          append_images=reward_frames[1:], duration=100, loop=0)
    emergence[0].save(DOCS / 'grave_emergence.gif', save_all=True,
                      append_images=emergence[1:], duration=[130, 130, 130, 130, 130, 600], loop=0)
    print(f'Refreshed {len(images)} screenshots, menu overview, close-ups, and reward animation.')


if __name__ == '__main__':
    main()
