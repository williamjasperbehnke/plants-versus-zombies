"""Indexed sprite loading and layouts for documentation studies."""
import json
from PIL import Image
from asset_layout import asset_path

GRASS = (75, 119, 56)


class StudyArt:
    def __init__(self, names, width=64, height=56, scale=3):
        self.width, self.height, self.scale = width, height, scale
        self.assets = {}
        self.frame_heights = {}
        for name in names:
            with Image.open(asset_path(name)) as source:
                self.assets[name] = source.copy()
            metadata = json.loads(asset_path(name, '.json').read_text())
            self.frame_heights[name] = metadata['height']

    def sprite(self, name, index, red=False, palette=None):
        source = self.assets[name]
        height = self.frame_heights[name]
        assert 0 <= index < source.height // height, (name, index)
        sprite = source.crop((0, index * height, source.width, (index + 1) * height))
        if palette:
            sprite.putpalette(self.assets[palette].getpalette())
        if red:
            colors = self.assets['damage_flash'].getpalette().copy()
            colors[45:48] = sprite.getpalette()[45:48]
            sprite.putpalette(colors)
        return sprite

    @staticmethod
    def paste(scene, sprite, x=32, y=30):
        left, top = x - sprite.width // 2, y - sprite.height // 2
        bounds = sprite.getbbox()
        if bounds:
            assert left + bounds[0] >= 0
            assert left + bounds[2] <= scene.width
            assert top + bounds[1] >= 0
            assert top + bounds[3] <= scene.height
        scene.paste(sprite.convert('RGB'), (left, top), sprite.point([0] + [255] * 255, 'L'))

    def scene(self):
        return Image.new('RGB', (self.width, self.height), GRASS)

    def compose(self, scenes, columns=2):
        rows = (len(scenes) + columns - 1) // columns
        width, height = self.width * self.scale, self.height * self.scale
        image = Image.new('RGB', (width * columns, height * rows), GRASS)
        for index, scene in enumerate(scenes):
            assert scene.size == (self.width, self.height)
            image.paste(scene.resize((width, height), Image.Resampling.NEAREST),
                        (index % columns * width, index // columns * height))
        return image
