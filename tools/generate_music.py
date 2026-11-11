"""Compose an original four-channel ProTracker loop; no external music assets."""
import math
import struct
from audio_config import OUTPUT_GAIN


def generate_music(path):
    samples = []
    for name, length, decay, volume in [('Garden bell', 4096, 9, 36),
                                         ('Soft bass', 4096, 5, 30),
                                         ('Wood pluck', 2048, 14, 24)]:
        data = bytearray()
        for i in range(length):
            t = i / 8363
            phase = 2 * math.pi * 130.81 * t
            tone = math.sin(phase) + .22 * math.sin(phase * 2)
            envelope = min(1, i / 35) * math.exp(-decay * t) * (1 - i / length)
            data.append(int(tone * envelope * 90) & 255)
        samples.append((name, min(64, round(volume * OUTPUT_GAIN)), data))
    module = bytearray(b'Garden at Dusk'.ljust(20, b'\0'))
    for i in range(31):
        name, volume, data = samples[i] if i < len(samples) else ('', 0, b'')
        module.extend(name.encode().ljust(22, b'\0'))
        module.extend(struct.pack('>HBBHH', len(data) // 2, 0, volume, 0, 1))
    module.extend(bytes([4, 0]) + bytes([0, 1, 2, 3]) + bytes(124) + b'M.K.')
    periods = {'C': 428, 'D': 381, 'E': 339, 'F': 320, 'G': 285, 'A': 254, 'B': 226}
    melodies = ['AEACEDEA GEDECEA', 'FACEGFEC ACEGFE ',
                'EGCEGAGE DCEDG E', 'GDBDGABG EDBAG E']
    chords = [('A', 'C', 'E'), ('F', 'A', 'C'), ('C', 'E', 'G'), ('G', 'B', 'D')]
    for pattern in range(4):
        assert len(melodies[pattern]) == 16
        cells = bytearray(64 * 4 * 4)

        def note(row, channel, instrument, period, effect=0, value=0):
            offset = (row * 4 + channel) * 4
            cells[offset:offset + 4] = bytes([
                (instrument & 0xf0) | (period >> 8), period & 255,
                ((instrument & 15) << 4) | effect, value])

        for beat, pitch in enumerate(melodies[pattern]):
            if pitch != ' ':
                note(beat * 4, 0, 1, periods[pitch] // 2)
        root, third, fifth = chords[pattern]
        for row in range(0, 64, 16):
            note(row, 1, 2, periods[root] * 2)
        for step, row in enumerate(range(0, 64, 8)):
            note(row, 2, 3, periods[(root, fifth, third, fifth)[step % 4]])
        if pattern == 0:
            note(0, 3, 0, 0, 15, 112)
        module.extend(cells)
    for _, _, data in samples:
        module.extend(data)
    path.write_bytes(module)
