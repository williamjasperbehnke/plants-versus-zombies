"""Plant animation comparisons using the current indexed artwork."""
from PIL import Image
from study_art import StudyArt

PLANTS = {
    'sunflower': 'sunflowers',
    'peashooter': 'peashooters',
    'wallnut': 'wallnuts',
    'potato_mine': 'potato_mines',
    'snow_pea': 'snow_peashooters',
    'cherry_bomb': 'cherries',
    'squash': 'squash',
    'tallnut': 'tallnuts',
    'repeater': 'repeaters',
    'chomper': 'chompers',
}
PANELS = {
    'sunflower': ['idle', 'sun production and collection', 'damage'],
    'peashooter': ['idle', 'shooting', 'damage'],
    'wallnut': ['healthy', 'damaged', 'worn', 'damage'],
    'potato_mine': ['arming', 'armed', 'detonation', 'damage before arming'],
    'snow_pea': ['idle', 'shooting', 'damage'],
    'cherry_bomb': ['idle', 'fuse and explosion'],
    'squash': ['idle', 'leap and landing', 'damage'],
    'tallnut': ['healthy', 'damaged', 'worn', 'damage'],
    'repeater': ['idle', 'paired shots', 'damage'],
    'chomper': ['idle', 'bite and swallow', 'damage'],
}
DURATION_MS = 50


def plant_studies():
    names = set(PLANTS.values()) | {'damage_flash', 'field_items', 'explosion',
                                   'normal_zombies'}
    art = StudyArt(names, width=96, height=80)
    result, details = {}, {}

    def idle(kind, clock, damage=0):
        alternate = clock // 24 % 2
        if kind == 'peashooter':
            return alternate * 2
        if kind == 'wallnut':
            return damage + alternate * 3
        if kind == 'tallnut':
            return damage * 2 + alternate
        if kind == 'potato_mine':
            return 1 + clock // 20 % 2
        if kind == 'chomper':
            return (0, 1, 8, 1)[clock // 16 % 4]
        return alternate

    def draw(scene, name, index, x=32, y=48, red=False):
        art.paste(scene, art.sprite(name, index, red), x, y)

    def damage(scene, kind, clock):
        stage = 0
        if kind in ('wallnut', 'tallnut'):
            stage = min(clock // 60, 2)
        index = 0 if kind == 'potato_mine' else idle(kind, clock, stage)
        red = clock % 48 < 6
        if kind in ('wallnut', 'tallnut'):
            red = any(0 <= clock - hit < 8 for hit in (60, 120, 162))
        draw(scene, PLANTS[kind], index, red=red)

    def shoot(scene, kind, clock):
        phase = (clock - 24) % 90
        index, recoil = idle(kind, clock), 0
        launches = (24, 114)
        if kind == 'repeater':
            launches = (24, 32, 114, 122)
            if phase < 6 or 8 <= phase < 14:
                age = phase % 8
                index = (2, 3, 5)[age // 2]
                recoil = (2, 1, 0)[age // 2]
            elif phase in (6, 7):
                index = 4
        elif phase < 6:
            index = 2 if kind == 'snow_pea' else 1
            recoil = 1
        draw(scene, PLANTS[kind], index, 32 - recoil)
        for launch in launches:
            age = clock - launch
            x = 40 + age * 5 // 2
            if age >= 0 and x <= 92:
                sheet = 'snow_peashooters' if kind == 'snow_pea' else 'field_items'
                index = 3 if kind == 'snow_pea' else 0
                draw(scene, sheet, index, x, 46)

    def sunflower(scene, clock):
        draw(scene, 'sunflowers', idle('sunflower', clock))
        age = clock - 48
        if 0 <= age < 96:
            x, y = 32, min(45 + age // 2, 54)
            if age >= 72:
                progress = (age - 72) ** 2
                x += (16 - x) * progress // (24 * 24)
                y += (16 - y) * progress // (24 * 24)
            draw(scene, 'field_items', 1, x, y)

    def squash(scene, clock):
        age = clock - 48
        if age < 0:
            draw(scene, 'squash', idle('squash', clock))
        elif age < 6:
            draw(scene, 'squash', 2)
        elif age < 30:
            time = age - 6
            x = 32 + 32 * time // 24
            y = 48 - 120 * time * (24 - time) // (24 * 24)
            draw(scene, 'squash', 3, x, y)
        elif age < 48:
            draw(scene, 'squash', 4, 64)

    def bite(scene, clock):
        age = clock - 48
        if age < 0:
            draw(scene, 'chompers', idle('chomper', clock))
            draw(scene, 'normal_zombies', clock // 12 % 2, 67)
            return
        if age < 36:
            remaining = 36 - age
            index = (2, 2, 3, 3, 4, 5)[age // 6]
            x = 31
            if remaining <= 24:
                x = 32 + min(24 - remaining, remaining) // 2
            if age < 24:
                pull = max(age - 12, 0)
                enemy_x = 67 + (46 - 67) * pull // 12
                enemy = art.sprite('normal_zombies', clock // 12 % 2)
                size = 32 * (100 - pull * 5) // 100
                enemy = enemy.resize((size, size), resample=Image.Resampling.NEAREST)
                art.paste(scene, enemy, enemy_x, 48 + pull // 3)
            draw(scene, 'chompers', index, x)
        else:
            draw(scene, 'chompers', (5, 9, 6, 10)[clock // 10 % 4])

    for kind, sheet in PLANTS.items():
        frames = []
        sources = {sheet}
        if kind != 'cherry_bomb':
            sources.add('damage_flash')
        for clock in range(0, 192, 3):
            scenes = [art.scene() for _ in PANELS[kind]]
            arming = kind == 'potato_mine'
            draw(scenes[0], sheet, 0 if arming else idle(kind, clock))
            if kind in ('wallnut', 'tallnut'):
                draw(scenes[1], sheet, idle(kind, clock, 1))
                draw(scenes[2], sheet, idle(kind, clock, 2))
            elif kind in ('peashooter', 'snow_pea', 'repeater'):
                shoot(scenes[1], kind, clock)
                sources.add('field_items' if kind != 'snow_pea' else 'snow_peashooters')
                damage(scenes[2], kind, clock)
            elif kind == 'sunflower':
                sunflower(scenes[1], clock)
                sources.add('field_items')
                damage(scenes[2], kind, clock)
            elif kind == 'potato_mine':
                draw(scenes[1], sheet, idle(kind, clock))
                if clock < 72:
                    draw(scenes[2], sheet, idle(kind, clock))
                elif clock < 90:
                    draw(scenes[2], sheet, 3)
            elif kind == 'cherry_bomb':
                if clock < 77:
                    draw(scenes[1], sheet, idle(kind, clock))
                elif clock < 95:
                    draw(scenes[1], 'explosion', (clock - 77) // 6, 48)
                sources.add('explosion')
            elif kind == 'squash':
                squash(scenes[1], clock)
                damage(scenes[2], kind, clock)
            elif kind == 'chomper':
                bite(scenes[1], clock)
                damage(scenes[2], kind, clock)
                sources.add('normal_zombies')
            if len(scenes) == 4:
                damage(scenes[3], kind, clock)
            frames.append(art.compose(scenes, columns=3 if len(scenes) == 3 else 2))
        filename = f'plant_{kind}_study.gif'
        result[filename] = frames
        details[filename] = {
            'kind': 'sprite-sheet study', 'sheets': sorted(sources),
            'duration_ms': DURATION_MS, 'panels': PANELS[kind],
        }

    # Tutorial prop: the sagging/blinking cycle has no combat or damage states.
    frames = []
    wilted_art = StudyArt({'wilted_plant'})
    for clock in range(0, 180, 3):
        scene = wilted_art.scene()
        index = (0, 1, 2, 1, 0, 3)[clock // 30 % 6]
        wilted_art.paste(scene, wilted_art.sprite('wilted_plant', index))
        frames.append(wilted_art.compose([scene], columns=1))
    filename = 'plant_wilted_study.gif'
    result[filename] = frames
    details[filename] = {
        'kind': 'sprite-sheet study', 'sheets': ['wilted_plant'],
        'duration_ms': DURATION_MS, 'panels': ['idle'],
    }
    return result, details
