"""Close-up damage/death studies assembled from the current indexed sprite sheets."""
from study_art import StudyArt

# Asset names, not global animation IDs. The last two columns identify debris.
ZOMBIES = {
    'normal': ('normal_zombies', 'wounded_normal_zombies', 'zombie_deaths', None, 0),
    'conehead': ('cone_zombies', 'wounded_normal_zombies', 'zombie_deaths', 2, 0),
    'buckethead': ('bucket_zombies', 'wounded_normal_zombies', 'zombie_deaths', 4, 0),
    'newspaper': ('newspaper_zombies', 'wounded_newspaper_zombies', 'newspaper_deaths', 3, 1),
    'screen_door': ('screen_door_zombies', 'wounded_normal_zombies', 'zombie_deaths', 5, 0),
    'flag': ('flag_zombies', 'wounded_flag_zombies', 'flag_deaths', 6, 0),
    'football': ('football_zombies', 'wounded_football_zombies', 'football_deaths', 8, 7),
    'pole_vaulter': ('vaulting_zombies', 'wounded_vaulting_zombies', 'vault_deaths', 9, 7),
}
NATIVE_WIDTH = 64
NATIVE_HEIGHT = 56
SCALE = 3
DURATION_MS = 100


def damage_studies():
    """Return frames and source metadata before the publisher writes any output."""
    names = {'zombie_debris', 'damage_flash', 'normal_zombies'}
    for whole, wounded, corpse, _, _ in ZOMBIES.values():
        names.update((whole, wounded, corpse))
    art = StudyArt(names, NATIVE_WIDTH, NATIVE_HEIGHT, SCALE)
    pose = art.sprite
    paste = art.paste

    def debris(scene, kind, age):
        if not 0 <= age < 36:
            return
        palette = 'newspaper_zombies' if kind == 1 else None
        sprite = pose('zombie_debris', kind * 6 + age // 6, palette=palette)
        arm = kind in (0, 1, 7)
        hat = kind in (2, 4, 8)
        flag = kind == 6
        origin, ground = 3, 12
        x = 32 - 7 + age // 3
        if arm:
            origin, ground = 5, 10
            x = 32 - 9 - age // 9
        elif flag:
            origin, ground = -3, 4
            x = 32 + 3 + age // 3
        elif hat:
            origin = -13
            x = 32 + age // 3
        y = 30 + origin + (ground - origin) * age * age // (36 * 36)
        if not arm:
            y -= 32 * age * (36 - age) // (36 * 36)
        paste(scene, sprite, x, y)

    def walk(kind, clock, stripped=False):
        whole, wounded, _, _, _ = ZOMBIES[kind]
        alternate = clock // 12 % 2
        if kind in ('conehead', 'buckethead', 'screen_door') and stripped:
            return 'normal_zombies', alternate
        if kind == 'flag':
            return whole, clock // 8 % 4 * 4 + alternate
        if kind == 'football':
            armor = 12 if stripped else 0
            return whole, armor + clock // 6 % 4
        if kind == 'pole_vaulter':
            return whole, (4 if stripped else 0) + alternate
        if kind == 'newspaper' and stripped:
            return whole, 4 + clock // 6 % 2
        return whole, alternate

    result = {}
    details = {}
    for kind, (whole, wounded, corpse, equipment, arm) in ZOMBIES.items():
        frames = []
        for clock in range(0, 192, 6):
            scenes = []
            for panel in range(4):
                scene = art.scene()
                if panel == 0:
                    stripped = clock >= 48 and kind != 'flag'
                    name, index = walk(kind, clock, stripped)
                    if kind == 'buckethead' and 24 <= clock < 48:
                        index += 4
                    elif kind == 'screen_door' and 24 <= clock < 48:
                        index += 6
                    elif kind == 'football' and 24 <= clock < 48:
                        index += 6
                    if kind == 'pole_vaulter' and 24 <= clock < 60:
                        name, index = whole, 8 + min(3, (clock - 24) // 9)
                    if clock >= 96:
                        name = wounded
                    red = clock in (18, 42, 96)
                    paste(scene, pose(name, index, red))
                    if equipment is not None and kind != 'flag':
                        release = 24 if kind == 'pole_vaulter' else 48
                        debris(scene, equipment, clock - release)
                    debris(scene, arm, clock - 96)
                elif panel == 1:
                    _, index = walk(kind, clock, True)
                    paste(scene, pose(wounded, index))
                else:
                    age = clock - 96
                    duration = 60 if kind == 'flag' else 48
                    stages = 10 if kind == 'flag' else 8
                    if age < 0:
                        name, index = walk(kind, clock)
                        if panel == 3:
                            # Arm loss happens after protection breaks.
                            _, index = walk(kind, clock, True)
                            name = wounded
                        paste(scene, pose(name, index))
                    elif age < duration:
                        index = age * stages // duration
                        if panel == 3:
                            index += stages
                        paste(scene, pose(corpse, index))
                    if equipment is not None and (panel == 2 or kind == 'flag'):
                        debris(scene, equipment, age)
                scenes.append(scene)
            frames.append(art.compose(scenes))
        filename = f'zombie_{kind}_damage_death_study.gif'
        result[filename] = frames
        details[filename] = {
            'kind': 'sprite-sheet study',
            'sheets': sorted({whole, wounded, corpse, 'zombie_debris', 'damage_flash'}),
            'duration_ms': DURATION_MS,
            'panels': ['damage and equipment', 'wounded movement', 'death', 'wounded death'],
        }
    return result, details
