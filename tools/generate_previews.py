"""Publish the current ROM gallery and remove retired documentation images."""
from pathlib import Path
import hashlib
import json
from PIL import Image, ImageDraw
from asset_layout import asset_path

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build'
DOCS = ROOT / 'docs'
NEAREST = Image.Resampling.NEAREST

# target: (emulator capture, debug ROM). Shared Almanac captures are last written
# by the gallery run on the debug ROM; lifetime kills comes from the normal run.
CAPTURES = {
    'title': ('title', False),
    'seed_selection': ('seed_selection_all', True),
    'seed_selection_custom': ('seed_selection_custom', True),
    'seed_selection_error': ('seed_selection_error', True),
    'chomper_chewing': ('chomper_chewing', True),
    'repeater_pair': ('repeater_pair', True),
    'squash_landing': ('squash_leap_08', True),
    'gameplay': ('developed_lawn', False),
    'lawn_start': ('playing', False),
    'lawn_three_lanes': ('lawn_three_lanes', False),
    'paused': ('paused', False),
    'levels_locked': ('levels_locked', False),
    'levels_progress': ('levels_progress', False),
    'levels_debug': ('levels_debug', True),
    'levels_reloaded': ('levels_reloaded', False),
    'preview_level_1': ('preview_level_1', False),
    'preview_level_10': ('preview_level_10', True),
    'almanac_plants': ('almanac_plants', True),
    'almanac_zombies': ('almanac_zombies', True),
    'almanac_lifetime_kills': ('almanac_lifetime_kills', False),
    'plant_reward': ('plant_reward', False),
    'plant_reward_mid': ('plant_reward_mid', False),
    'trophy_reward': ('trophy_reward', False),
    'loss_animation': ('loss_animation', False),
    'loss_prompt': ('loss_prompt', False),
    'snow_pea_gameplay': ('snow_pea_gameplay', True),
    'plant_red_flash': ('plant_red_flash', True),
    'zombie_damage_flash': ('zombie_damage_flash', True),
    'tombstones_lawn': ('tombstones_lawn', True),
    'tombstone_cracked': ('tombstone_cracked', True),
    'tombstone_crumble': ('tombstone_crumble_31', True),
    'tombstone_destroyed': ('tombstone_destroyed', True),
    'explosion_above_plants': ('explosion_above_plants', True),
    'grave_emergence': ('grave_emergence_2', True),
    'armored_horde': ('armored_horde', True),
    'placement_check': ('placement_check', True),
    'placement_occupied': ('placement_occupied', True),
    'placement_cooldown': ('placement_cooldown', True),
    'placement_unaffordable': ('placement_unaffordable', True),
    'shovel_selected': ('shovel_selected', True),
    'shovel_removed': ('shovel_removed', True),
    'mixer_master_muted': ('mixer_master_muted', True),
    'mixer_music_muted': ('mixer_music_muted', True),
    'mixer_effects_muted': ('mixer_effects_muted', True),
    'mower_active': ('mower_active', True),
    'zombie_arm_drop': ('zombie_combat_034', True),
    'zombie_missing_arm': ('zombie_combat_055', True),
    'zombie_death': ('zombie_combat_080', True),
    'flag_death': ('flag_death_03', True),
    'road_entry': ('road_entry_12', True),
}
for name in ('sun_before', 'sun_start', 'sun_mid', 'sun_arrived',
             'mine_arming', 'mine_armed', 'mine_burst', 'mine_spent',
             'wallnut_healthy', 'wallnut_damaged', 'wallnut_worn'):
    CAPTURES[name] = (name, True)
PLANTS = ('peashooter', 'sunflower', 'wallnut', 'potato_mine', 'snow_pea', 'cherry_bomb', 'squash', 'tallnut', 'repeater', 'chomper')
ZOMBIES = ('normal', 'conehead', 'newspaper', 'flag', 'buckethead', 'screen_door', 'football', 'pole_vaulter')
for kind, names in (('plant', PLANTS), ('zombie', ZOMBIES)):
    for index, name in enumerate(names):
        CAPTURES[f'almanac_{kind}_{name}'] = (f'almanac_{kind}_{index}', True)

ANIMATIONS = {
    'seed_bank_flight': ([(f'seed_bank_flight_{i:02d}', True) for i in range(12)], [65] * 11 + [700]),
    'repeater_burst': ([(f'repeater_burst_{i:02d}', True) for i in range(24)], [65] * 23 + [700]),
    'chomper_bite': ([(f'chomper_bite_{i:02d}', True) for i in range(32)], [75] * 31 + [700]),
    'squash_leap': ([(f'squash_leap_{i:02d}', True) for i in range(12)], [85] * 11 + [700]),
    'tombstone_crumble': ([(f'tombstone_crumble_{i:02d}', True) for i in range(50)], 50),
    'road_entry': ([(f'road_entry_{i:02d}', True) for i in range(48)], 100),
    'gameplay': ([(f'gameplay_action_{i:02d}', False) for i in range(12)], 100),
    'flag_death': ([(f'flag_death_{i:02d}', True) for i in range(24)], 100),
    'zombie_combat': ([(f'zombie_combat_{i:03d}', True) for i in range(120)], 100),
    'title': ([(f'title_animation_{i:02d}', False) for i in range(12)], 200),
    'plant_reward': ([(f'reward_{i:02d}', False) for i in range(56)], 100),
    'grave_emergence': ([(f'grave_emergence_{i}', True) for i in range(6)],
                        [130, 130, 130, 130, 130, 600]),
    'sun_collection': ([(name, True) for name in ('sun_before', 'sun_start', 'sun_mid', 'sun_arrived')],
                       [400, 50, 180, 600]),
    'scouting': ([(name, True) for name in ('scouting_pan_start',
                   *(f'scouting_pan_{i}' for i in range(5)), 'preview_level_10')],
                 [260, 170, 170, 170, 170, 170, 700]),
    'defeat': ([(f'loss_sequence_{i:02d}', False) for i in range(10)] + [('loss_prompt', False)],
               [100] * 10 + [700]),
}


def capture(name, debug=False):
    path = BUILD / f'{name}.ppm'
    rom = ROOT / ('plants_vs_zombies_debug.gba' if debug else 'plants_vs_zombies.gba')
    if not path.exists() or not rom.exists() or path.stat().st_mtime < rom.stat().st_mtime:
        raise SystemExit(f'{path.name} needs a fresh capture. Run sh tools/refresh_docs.sh.')
    with Image.open(path) as source:
        return source.convert('RGB')


def enlarged(image, scale=3):
    return image.resize((image.width * scale, image.height * scale), NEAREST)


def contact_sheet(images, labels, columns=3):
    scale = 2
    width, height = 240 * scale, 160 * scale + 24
    rows = (len(images) + columns - 1) // columns
    result = Image.new('RGB', (width * columns, height * rows), (17, 30, 29))
    draw = ImageDraw.Draw(result)
    for i, (image, label) in enumerate(zip(images, labels)):
        x, y = i % columns * width, i // columns * height
        draw.text((x + 6, y + 6), label, fill=(255, 241, 191))
        result.paste(enlarged(image, scale), (x, y + 24))
    return result


def main():
    # Load and validate every source before replacing or retiring any image.
    images = {target: capture(*source) for target, source in CAPTURES.items()}
    animations = {name: [enlarged(capture(*source)) for source in sources]
                  for name, (sources, _) in ANIMATIONS.items()}
    # Validate current sprite atlases too; these studies are labeled separately from ROM captures.
    studies = {}
    for name in ('zombie_deaths', 'newspaper_deaths', 'zombie_debris', 'newspaper_zombies', 'flag_deaths', 'football_zombies', 'football_deaths', 'vaulting_zombies', 'wounded_vaulting_zombies', 'screen_door_zombies'):
        with Image.open(asset_path(name)) as source:
            studies[name] = source.copy()
    DOCS.mkdir(exist_ok=True)
    outputs = set()

    def publish(name, image):
        image.save(DOCS / name)
        outputs.add(name)

    def sheet(name, names, labels, columns=3):
        publish(name + '.png', contact_sheet([images[n] for n in names], labels, columns))

    for name, image in images.items():
        publish(name + '.png', enlarged(image))
    sheet('menus', ('title', 'paused', 'levels_locked'), ('Title', 'Pause / audio', 'Backyard levels'))
    sheet('overview', ('gameplay', 'preview_level_10', 'armored_horde', 'almanac_plants',
          'plant_reward', 'tombstones_lawn', 'snow_pea_gameplay', 'mower_active', 'loss_prompt'),
          ('Lawn and HUD', 'Road scouting', 'Flag-led horde', 'Almanac', 'New seed',
           'Tombstones', 'Snow Pea', 'Lawn mower', 'Defeat'))
    for kind, names in (('plant', PLANTS), ('zombie', ZOMBIES)):
        sheet(f'almanac_{kind}_entries', [f'almanac_{kind}_{name}' for name in names],
              [name.replace('_', ' ').title() for name in names])
    sheet('placement', ('placement_check', 'placement_occupied', 'placement_cooldown', 'placement_unaffordable'),
          ('Planted', 'Occupied', 'Recharging', 'Not enough sun'), 2)
    sheet('mixer', ('mixer_master_muted', 'mixer_music_muted', 'mixer_effects_muted'),
          ('Master muted', 'Music muted', 'Effects muted'))
    sheet('sun_collection', ('sun_before', 'sun_start', 'sun_mid', 'sun_arrived'),
          ('Before collecting', 'Collection starts', 'Flying to the bank', 'Arrived'), 2)
    sheet('mine', ('mine_arming', 'mine_armed', 'mine_burst', 'mine_spent'),
          ('Arming', 'Ready', 'Contact explosion', 'Spent'), 2)
    sheet('wallnut', ('wallnut_healthy', 'wallnut_damaged', 'wallnut_worn'),
          ('Healthy', 'Damaged', 'Nearly eaten'))
    publish('seed_packets.png', enlarged(images['tombstones_lawn'].crop((52, 22, 224, 54)), 4))
    publish('mower_closeup.png', enlarged(images['mower_active'].crop((82, 69, 120, 103)), 6))
    publish('lawn_edge.png', enlarged(images['gameplay'].crop((0, 57, 45, 154)), 5))
    for count, name in ((3, 'gameplay'), (5, 'armored_horde')):
        publish(f'wave_track_{count}.png', enlarged(images[name].crop((50, 0, 240, 23)), 4))
    publish('final_flag.png', enlarged(images['lawn_start'].crop((53, 0, 78, 20)), 8))
    for name, frames in animations.items():
        frames[0].save(DOCS / f'{name}.gif', save_all=True, append_images=frames[1:],
                       duration=ANIMATIONS[name][1], loop=0)
        outputs.add(name + '.gif')

    # Actual gameplay close-up, using the same captured frames as the full scene.
    combat = [enlarged(capture(f'zombie_combat_{i:03d}', True).crop((110, 66, 239, 108)), 4)
              for i in range(24, 88)]
    combat[0].save(DOCS / 'zombie_damage_death.gif', save_all=True,
                   append_images=combat[1:], duration=100, loop=0)
    outputs.add('zombie_damage_death.gif')
    # Isolated sprite studies show every equipment type clearly, without claiming
    # these composites are an in-game screen. Gameplay adds the falling trajectories.
    def on_grass(sprite):
        image = Image.new('RGB', sprite.size, (75, 119, 56))
        image.paste(sprite.convert('RGB'), (0, 0), sprite.point([0] + [255] * 255, 'L'))
        return enlarged(image, 4)

    for name in ('zombie_deaths', 'newspaper_deaths', 'flag_deaths', 'football_deaths'):
        source = studies[name]
        frames = [on_grass(source.crop((0, i * 32, 32, (i + 1) * 32))) for i in range(10 if name == 'flag_deaths' else 8)]
        frames[0].save(DOCS / f'{name}_study.gif', save_all=True, append_images=frames[1:], duration=100, loop=0)
        outputs.add(f'{name}_study.gif')
    runner = studies['football_zombies']
    charge = [on_grass(runner.crop((0, i * 32, 32, (i + 1) * 32))) for i in range(4)]
    charge[0].save(DOCS / 'football_charge_study.gif', save_all=True, append_images=charge[1:], duration=100, loop=0)
    outputs.add('football_charge_study.gif')

    door_frames = []
    source = studies['screen_door_zombies']
    for beat in range(4):
        result = Image.new('RGB', (512, 152), (17, 30, 29))
        draw = ImageDraw.Draw(result)
        for armor, label in enumerate(('Intact door', 'Damaged door')):
            frame = 2 + beat + armor * 6
            sprite = source.crop((0, frame * 32, 64, (frame + 1) * 32))
            draw.text((armor * 256 + 4, 5), label, fill=(255, 241, 191))
            result.paste(on_grass(sprite), (armor * 256, 24))
        door_frames.append(result)
    door_frames[0].save(DOCS / 'screen_door_bite_study.gif', save_all=True,
                        append_images=door_frames[1:], duration=170, loop=0)
    outputs.add('screen_door_bite_study.gif')
    # Exact native sprite frames, including the equipment layer after arm loss.
    vaulter_frames = []
    poses = ((0, 1, 0, 1), (2, 3, 2, 3), (6, 7, 6, 7), (8, 9, 10, 11))
    for beat in range(4):
        result = Image.new('RGB', (128 * 4, 152 * 2), (17, 30, 29))
        draw = ImageDraw.Draw(result)
        for row, name in enumerate(('vaulting_zombies', 'wounded_vaulting_zombies')):
            for column, label in enumerate(('Walk / pole', 'Bite / pole', 'Bite / spent', 'Vault')):
                frame = poses[column][beat]
                sprite = studies[name].crop((0, frame * 32, 32, (frame + 1) * 32))
                x, y = column * 128, row * 152
                draw.text((x + 4, y + 5), ('Wounded ' if row else '') + label, fill=(255, 241, 191))
                result.paste(on_grass(sprite), (x, y + 24))
        vaulter_frames.append(result)
    vaulter_frames[0].save(DOCS / 'pole_vaulter_study.gif', save_all=True,
                          append_images=vaulter_frames[1:], duration=160, loop=0)
    outputs.add('pole_vaulter_study.gif')

    frames = []
    labels = ('Arm', 'Newspaper arm', 'Cone', 'Newspaper', 'Bucket', 'Screen door', 'Flag', 'Football arm', 'Football helmet')
    for stage in range(6):
        result = Image.new('RGB', (128 * 3, 152 * 3), (17, 30, 29))
        draw = ImageDraw.Draw(result)
        for kind, label in enumerate(labels):
            source = studies['zombie_debris']
            sprite = source.crop((0, (kind * 6 + stage) * 32, 32, (kind * 6 + stage + 1) * 32))
            if kind == 1:
                sprite.putpalette(studies['newspaper_zombies'].getpalette())
            x, y = kind % 3 * 128, kind // 3 * 152
            draw.text((x + 4, y + 5), label, fill=(255, 241, 191))
            result.paste(on_grass(sprite), (x, y + 24))
        frames.append(result)
    frames[0].save(DOCS / 'falling_equipment_study.gif', save_all=True,
                   append_images=frames[1:], duration=100, loop=0)
    outputs.add('falling_equipment_study.gif')

    # This directory contains current published captures and studies; old runner/campaign studies
    # must not linger beside current ROM screenshots after a successful refresh.
    retired = []
    for path in DOCS.iterdir():
        if path.suffix.lower() in ('.png', '.gif') and path.name not in outputs:
            path.unlink()
            retired.append(path.name)
    manifest = {
        'roms': {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
                 for name in ('plants_vs_zombies.gba', 'plants_vs_zombies_debug.gba')},
        'screenshots': {name + '.png': {'source': source + '.ppm', 'debug': debug, 'scale': 3}
                        for name, (source, debug) in CAPTURES.items()},
        'animations': {name + '.gif': {'sources': [source + '.ppm' for source, _ in sources],
                                      'debug': sources[0][1], 'duration_ms': duration}
                        for name, (sources, duration) in ANIMATIONS.items()},
        'studies': ['zombie_deaths_study.gif', 'newspaper_deaths_study.gif', 'falling_equipment_study.gif', 'flag_deaths_study.gif', 'football_deaths_study.gif', 'football_charge_study.gif', 'pole_vaulter_study.gif', 'screen_door_bite_study.gif'],
        'files': sorted(outputs),
    }
    (DOCS / 'captures.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(f'Published {len(outputs)} current images/animations; removed {len(retired)} retired images.')


if __name__ == '__main__':
    main()
