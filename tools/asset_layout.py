"""Stable asset categories shared by exporters and preview tools."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PLANTS = {'sunflowers', 'peashooters', 'wallnuts', 'cherries', 'potato_mines',
          'snow_peashooters', 'squash', 'repeaters', 'tallnuts', 'chompers', 'wilted_plant'}
BACKGROUNDS = {'title', 'save_select', 'save_delete', 'paused', 'level_select', 'almanac', 'seed_select',
               'reward_panel', 'tutorial_header',
               'yard_road', 'yard_road_1', 'yard_road_3', 'lawn', 'lawn_1', 'lawn_3'}


def asset_path(name, suffix='.bmp'):
    if name.endswith(('_seed_packet', '_reward_packet')):
        folder = 'seeds'
    elif name in PLANTS:
        folder = 'plants'
    elif name.endswith('_zombies') and name != 'emerging_zombies':
        folder = 'zombies'
    elif name.endswith('_deaths') or name in {'field_items', 'explosion', 'emerging_zombies',
                                             'zombie_debris', 'damage_flash', 'tombstones'}:
        folder = 'effects'
    elif name in BACKGROUNDS:
        folder = 'backgrounds'
    else:
        folder = 'ui'
    return ROOT / 'graphics' / folder / (name + suffix)
