#include "../include/save_layout.h"
// Optional headless ROM integration check. Requires libmgba development headers.
// Build: cc tools/rom_smoke.c -I/usr/local/include -L/usr/local/lib -lmgba -o build/rom_smoke
#include "../include/defeat_audio.h"
#include <mgba/flags.h>
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/core/blip_buf.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static color_t pixels[240 * 160];
static bool capture_audio;
static uint64_t audio_energy;
static void screenshot(const char* path);

static void log_message(struct mLogger* logger, int category, enum mLogLevel level,
                        const char* format, va_list args)
{
    (void)logger;
    (void)category;
    if(level & (mLOG_FATAL | mLOG_ERROR))
    {
        vfprintf(stderr, format, args);
        fputc('\n', stderr);
    }
}

// Hash visible RGB; the unused high byte can vary between background and sprite layers.
static uint32_t region_hash(int x, int y, int width, int height)
{
    uint32_t hash = 2166136261u;
    for(int row = y; row < y + height; ++row)
    {
        for(int column = x; column < x + width; ++column)
        { hash = (hash ^ (pixels[row * 240 + column] & 0x00ffffffu)) * 16777619u; }
    }
    return hash;
}

static uint32_t image_hash(void) { return region_hash(0, 0, 240, 160); }

static void frames(struct mCore* core, unsigned keys, int count)
{
    core->setKeys(core, keys);
    for(int i = 0; i < count; ++i)
    {
        core->runFrame(core);
        for(int channel = 0; channel < 2; ++channel)
        {
            short samples[4096];
            int read = blip_read_samples(core->getAudioChannel(core, channel), samples, 4096, 0);
            if(capture_audio)
            {
                for(int sample = 0; sample < read; ++sample)
                { audio_energy += samples[sample] < 0 ? -samples[sample] : samples[sample]; }
            }
        }
    }
}

static void press(struct mCore* core, unsigned keys)
{
    frames(core, keys, 3);
    frames(core, 0, 3);
}

static uint64_t measure_audio(struct mCore* core, int count)
{
    audio_energy = 0;
    capture_audio = true;
    frames(core, 0, count);
    capture_audio = false;
    return audio_energy;
}

static void screenshot(const char* path)
{
    FILE* file = fopen(path, "wb");
    if(! file) { perror(path); exit(1); }
    fprintf(file, "P6\n240 160\n255\n");
    for(int i = 0; i < 240 * 160; ++i)
    {
        // mGBA's default 32-bit color_t stores red in the low byte.
        fputc(pixels[i] & 255, file);
        fputc((pixels[i] >> 8) & 255, file);
        fputc((pixels[i] >> 16) & 255, file);
    }
    fclose(file);
}


static bool on_lawn(void)
{
    uint32_t label = pixels[26 * 240 + 22];
    return (label & 255) > 220 && (label >> 8 & 255) > 180 && (label >> 16 & 255) < 130;
}

static int save_slot(struct mCore* core, int offset)
{
    uint32_t sequences[2] = {0, 0};
    bool valid[2] = {false, false};
    for(int slot = 0; slot < 2; ++slot)
    {
        uint8_t data[PVZ_SAVE_SIZE];
        uint32_t hash = 2166136261u;
        for(int i = 0; i < PVZ_SAVE_SIZE; ++i)
        { data[i] = core->busRead8(core, 0x0e000000 + offset + slot * PVZ_SAVE_SIZE + i); }
        for(int i = 0; i < PVZ_SAVE_CHECKSUM; ++i) { hash = (hash ^ data[i]) * 16777619u; }
        uint32_t stored = (uint32_t)data[48] | (uint32_t)data[49] << 8 |
                          (uint32_t)data[50] << 16 | (uint32_t)data[51] << 24;
        valid[slot] = data[0] == 'P' && data[1] == 'V' && data[2] == 'Z' && data[3] == '3' && hash == stored;
        sequences[slot] = (uint32_t)data[12] | (uint32_t)data[13] << 8 |
                          (uint32_t)data[14] << 16 | (uint32_t)data[15] << 24;
    }
    assert(valid[0] || valid[1]);
    return valid[1] && (! valid[0] || sequences[1] >= sequences[0]) ? 1 : 0;
}

static int saved_byte(struct mCore* core, int offset, int field)
{ return core->busRead8(core, 0x0e000000 + offset + save_slot(core, offset) * PVZ_SAVE_SIZE + field); }

static uint32_t saved_kills(struct mCore* core, int offset, int type)
{
    int field = PVZ_SAVE_KILL_OFFSET(type);
    return (uint32_t)saved_byte(core, offset, field) | (uint32_t)saved_byte(core, offset, field + 1) << 8 |
           (uint32_t)saved_byte(core, offset, field + 2) << 16 | (uint32_t)saved_byte(core, offset, field + 3) << 24;
}

static void legacy_save(struct mCore* core)
{
    uint8_t data[24] = {'S', 'V', 'S', '1', 1, 3, 1, 0, 1, 0, 0, 0, 7};
    uint32_t hash = 2166136261u;
    for(int i = 0; i < 20; ++i) { hash = (hash ^ data[i]) * 16777619u; }
    for(int i = 0; i < 4; ++i) { data[20 + i] = hash >> (i * 8); }
    for(int i = 0; i < 24; ++i) { core->busWrite8(core, 0x0e000000 + i, data[i]); }
}

static void v2_save(struct mCore* core)
{
    uint8_t data[48] = {'P', 'V', 'Z', '2', 2, 8, 6, 0, 63, 0, 0, 0, 9};
    for(int type = 0; type < 7; ++type)
    {
        int at = PVZ_SAVE_KILL_OFFSET(type);
        data[at] = (uint8_t)(10 + type);
    }
    uint32_t hash = 2166136261u;
    for(int i = 0; i < 44; ++i) { hash = (hash ^ data[i]) * 16777619u; }
    for(int i = 0; i < 4; ++i) { data[44 + i] = hash >> (i * 8); }
    for(int i = 0; i < 48; ++i) { core->busWrite8(core, 0x0e000000 + PVZ_SAVE_V2_NORMAL_OFFSET + i, data[i]); }
}

static void reset_cartridge(struct mCore* core)
{
    void* data = NULL;
    size_t size = core->savedataClone(core, &data);
    assert(data && size >= 608);
    core->reset(core);
    assert(core->savedataRestore(core, data, size, false));
    free(data);
    frames(core, 0, 120);
}

static bool first_level_active(struct mCore* core)
{
    (void)core;
    // The gold selector disappears at either terminal transition, including replays.
    return on_lawn() && ((pixels[24 * 240 + 55] & 255) > 200 ||
                         (pixels[24 * 240 + 83] & 255) > 200);
}
struct DefenseCursor { int row, column, seed; };
static void collect_wait(struct mCore* core, int count)
{
    static bool horde_captured;
    for(int i = 0; i < count && first_level_active(core); ++i)
    {
        frames(core, 1 << 1, 1);
        if(! horde_captured && core->frameCounter(core) >= 1500)
        { screenshot("build/horde.ppm"); horde_captured = true; }

    }
}
static void plant_at(struct mCore* core, struct DefenseCursor* c, int row, int column, int seed)
{
    if(! first_level_active(core)) { return; }
    while(c->row != row) { press(core, 1 << (c->row > row ? 6 : 7)); c->row += c->row > row ? -1 : 1; }
    while(c->column != column) { press(core, 1 << (c->column > column ? 5 : 4)); c->column += c->column > column ? -1 : 1; }
    while(c->seed != seed) { press(core, 1 << 8); c->seed = (c->seed + 1) % 2; }
    if(first_level_active(core)) { press(core, 1 << 0); }
}
static bool peashooter_visible(int row, int column)
{
    int lime = 0;
    int cx = 45 + column * 22, cy = 67 + row * 19;
    for(int y = cy - 7; y < cy + 1; ++y)
    {
        for(int x = cx - 5; x < cx + 5; ++x)
        {
            uint32_t pixel = pixels[y * 240 + x];
            int red = pixel & 255, green = pixel >> 8 & 255, blue = pixel >> 16 & 255;
            lime += green > 169 && green > red + 10 && blue < 120;
        }
    }
    return lime > 12;
}
static void clear_first_level(struct mCore* core)
{
    struct DefenseCursor c = {2, 0, 0};
    bool action_captured = false;
    for(int pass = 0; pass < 180 && first_level_active(core); ++pass)
    {
        bool covered = true;
        for(int column = 2; column <= 4 && first_level_active(core); ++column)
        {
            if(! peashooter_visible(2, column))
            { covered = false; plant_at(core, &c, 2, column, 0); collect_wait(core, 30); }
        }
        if(covered && ! action_captured)
        {
            for(int frame = 0; frame < 12; ++frame)
            {
                char path[80];
                frames(core, 1 << 1, 6);
                assert(first_level_active(core));
                snprintf(path, sizeof(path), "build/gameplay_action_%02d.ppm", frame);
                screenshot(path);
            }
            action_captured = true;
        }
        if(pass % 10 == 0 && first_level_active(core)) { screenshot("build/developed_lawn.ppm"); }
        collect_wait(core, 120);
    }
    assert(action_captured);
    collect_wait(core, 24000);
    assert(saved_byte(core, PVZ_SAVE_NORMAL_OFFSET, 8) & 1);
}
static int seed_bank_scenario = 0;
static bool debug_seed_bank = false;

static void choose_seed(struct mCore* core, int* cursor, int target)
{
    while(*cursor != target) { press(core, 1 << 4); *cursor = (*cursor + 1) % 10; }
    press(core, 1 << 0);
}


static void start_round(struct mCore* core, const char* preview_path)
{
    press(core, 1 << 0);
    for(int y = 0; y < 54; ++y)
    {
        for(int x = 0; x < 60; ++x) { assert((pixels[y * 240 + x] & 0x00ffffff) == 0); }
    }
    screenshot("build/preview_pan_start.ppm");
    if(preview_path && strcmp(preview_path, "build/preview_level_10.ppm") == 0)
    { screenshot("build/scouting_pan_start.ppm"); }
    press(core, 1 << 0); // The initial pan cannot be skipped by an early press.
    for(int frame = 0; frame < 5; ++frame)
    {
        frames(core, 0, 10);
        if(preview_path && strcmp(preview_path, "build/preview_level_10.ppm") == 0)
        {
            char path[80];
            snprintf(path, sizeof(path), "build/scouting_pan_%d.ppm", frame);
            screenshot(path);
        }
    }
    int level_ink = 0;
    for(int y = 6; y < 14; ++y)
    {
        for(int x = 82; x < 158; ++x)
        {
            uint32_t color = pixels[y * 240 + x];
            level_ink += (color & 255) > 220 && (color >> 8 & 255) > 220 && (color >> 16 & 255) > 150;
        }
    }
    assert(level_ink > 8); // The level is rendered before the preview becomes ready.
    screenshot("build/preview_before_ready.ppm");
    frames(core, 0, 40);
    assert(! on_lawn());
    if(preview_path) { screenshot(preview_path); }
    uint32_t header = region_hash(0, 0, 240, 55);
    press(core, 1 << 2); // Select no longer opens the almanac.
    frames(core, 0, 15);
    assert(region_hash(0, 0, 240, 55) == header);
    press(core, 1 << 0);
    int level = saved_byte(core, debug_seed_bank ? PVZ_SAVE_DEBUG_OFFSET : PVZ_SAVE_NORMAL_OFFSET, 6);
    if(level >= 6)
    {
        if(preview_path && strcmp(preview_path, "build/preview_level_10.ppm") == 0)
        {
            for(int frame = 0; frame < 12; ++frame)
            {
                char path[80]; snprintf(path, sizeof(path), "build/seed_bank_flight_%02d.ppm", frame);
                screenshot(path); frames(core, 0, 3);
            }
        }
        else { frames(core, 0, 32); }
        if(preview_path && strcmp(preview_path, "build/preview_level_10.ppm") == 0) { screenshot("build/seed_selection_all.ppm"); }
        if(preview_path && strcmp(preview_path, "build/preview_level_10.ppm") == 0)
        {
            uint32_t heading = region_hash(8, 42, 150, 14);
            int cursor = 0; choose_seed(core, &cursor, 0);
            press(core, 1 << 3); // A five-card bank must stay on the picker with a visible error.
            assert(! on_lawn());
            int red = 0;
            for(int y = 44; y < 55; ++y)
            {
                for(int x = 48; x < 116; ++x)
                {
                    uint32_t pixel = pixels[y * 240 + x];
                    red += (pixel & 255) > 220 && (pixel >> 8 & 255) < 120 && (pixel >> 16 & 255) < 100;
                }
            }
            assert(red > 10 && region_hash(8, 42, 150, 14) != heading);
            screenshot("build/seed_selection_error.ppm");
            press(core, 1 << 0); // Restoring the sixth card clears the warning.
            frames(core, 0, 32); // Let the returning card clear the heading before comparing text.
            assert(region_hash(8, 42, 150, 14) == heading);
        }
        if(debug_seed_bank)
        {
            int cursor = 0;
            // Reorder through ordinary selection input so existing scenarios use named, stable slots.
            for(int i = 0; i < 6; ++i) { choose_seed(core, &cursor, i); }
            const int regular[] = {1, 0, 2, 3, 4, 5};
            const int custom[] = {1, 2, 3, 4, 6, 8};
            const int chomper[] = {1, 0, 4, 6, 7, 9};
            for(int i = 0; i < 6; ++i) { choose_seed(core, &cursor, seed_bank_scenario == 2 ? chomper[i] : (seed_bank_scenario == 1 ? custom[i] : regular[i])); }
            if(seed_bank_scenario == 1) { frames(core, 0, 32); screenshot("build/seed_selection_custom.ppm"); }
        }
        screenshot("build/seed_selection.ppm");
        press(core, 1 << 3); // Confirm the selected seed bank before returning to the lawn.
    }
    frames(core, 0, 53);
    screenshot("build/scouting_return.ppm");
    // Exclude the planting cursor that appears on the first lawn column at level start.
    uint32_t garden = region_hash(62, 57, 178, 103);
    uint32_t mowers = region_hash(0, 57, 32, 103);
    frames(core, 0, 17);
    assert(on_lawn());
    screenshot("build/scouting_start.ppm");
    assert(region_hash(62, 57, 178, 103) == garden && region_hash(0, 57, 32, 103) == mowers); // Lawn, palette and mowers match across the transition.
}

static void check_portrait_pixels(void)
{
    // Every original pixel maps to exactly 2x2 display pixels, including all zombie variants.
    for(int y = 48; y < 106; y += 2)
    {
        for(int x = 32; x < 96; x += 2)
        {
            uint32_t color = pixels[y * 240 + x];
            assert(pixels[y * 240 + x + 1] == color);
            assert(pixels[(y + 1) * 240 + x] == color);
            assert(pixels[(y + 1) * 240 + x + 1] == color);
        }
    }
}

static void tombstone_test(struct mCore* core)
{
    screenshot("build/tombstones_lawn.ppm");
    press(core, 1 << 6); press(core, 1 << 6); // Top lane.
    for(int i = 0; i < 5; ++i) { press(core, 1 << 4); }
    press(core, 1 << 8); // Peashooter immediately left of the first grave.
    press(core, 1 << 0);
    uint32_t intact = region_hash(170, 58, 15, 16);
    frames(core, 1 << 1, 300);
    screenshot("build/tombstone_cracked.ppm");
    assert(region_hash(170, 58, 15, 16) != intact);
    frames(core, 1 << 1, 150);
    for(int frame = 0; frame < 50; ++frame)
    {
        char path[80]; frames(core, 1 << 1, 3);
        snprintf(path, sizeof(path), "build/tombstone_crumble_%02d.ppm", frame); screenshot(path);
    }
    screenshot("build/tombstone_destroyed.ppm");
    // Every remaining pixel in the old stone's top edge is grass, rather than gray stone.
    int gray = 0;
    for(int y = 58; y < 64; ++y)
    {
        for(int x = 172; x < 184; ++x)
        {
            uint32_t pixel = pixels[y * 240 + x];
            int r = pixel & 255, g = pixel >> 8 & 255, b = pixel >> 16 & 255;
            gray += b > 110 && r > 100 && g < r + 50;
        }
    }
    assert(gray == 0 && on_lawn());
    reset_cartridge(core);
    press(core, 1 << 0);
    start_round(core, NULL);
}

static void grave_emergence_test(struct mCore* core)
{
    bool dirt = false;
    for(int tick = 0; tick < 4500 && ! dirt; ++tick)
    {
        frames(core, 1 << 1, 1);
        int brown = 0;
        for(int y = 72; y < 79; ++y)
        {
            for(int x = 164; x < 190; ++x)
            {
                uint32_t pixel = pixels[y * 240 + x];
                int r = pixel & 255, g = pixel >> 8 & 255, b = pixel >> 16 & 255;
                brown += r > 100 && r < 140 && g > 60 && g < 90 && b < 65;
            }
        }
        dirt = brown > 8;
    }
    assert(dirt && on_lawn());
    uint32_t previous = 0;
    for(int stage = 0; stage < 6; ++stage)
    {
        char path[80];
        if(stage) { frames(core, 1 << 1, 8); }
        snprintf(path, sizeof(path), "build/grave_emergence_%d.ppm", stage);
        screenshot(path);
        uint32_t image = region_hash(161, 47, 32, 32);
        assert(image != previous);
        previous = image;
    }
    reset_cartridge(core);
    press(core, 1 << 0);
    start_round(core, NULL);
}

static void explosion_layer_test(struct mCore* core)
{
    press(core, 1 << 0); // Sunflower at the initial cursor.
    press(core, 1 << 7); press(core, 1 << 7);
    press(core, 1 << 4); press(core, 1 << 8);
    press(core, 1 << 0); // Peashooter in the bottom lane.
    frames(core, 1 << 1, 1700);
    press(core, 1 << 6); // Cherry Bomb directly above it.
    for(int i = 0; i < 4; ++i) { press(core, 1 << 8); }
    press(core, 1 << 0);
    bool covered = false;
    for(int tick = 0; tick < 70 && ! covered; ++tick)
    {
        frames(core, 0, 1);
        // This pixel belongs to the lower Peashooter's head until the blast covers it.
        uint32_t pixel = pixels[137 * 240 + 67];
        covered = (pixel & 255) > 240 && (pixel >> 8 & 255) > 180 && (pixel >> 16 & 255) < 120;
    }
    assert(covered && on_lawn());
    screenshot("build/explosion_above_plants.ppm");
    reset_cartridge(core);
    press(core, 1 << 0);
    start_round(core, NULL);
}

static void plant_damage_test(struct mCore* core)
{
    press(core, 1 << 8); press(core, 1 << 8); // Wall-nut.
    press(core, 1 << 6); // First zombie's lane.
    frames(core, 1 << 4, 55);
    frames(core, 0, 3);
    press(core, 1 << 0);
    press(core, 1 << 5); press(core, 1 << 5); // Move the cursor clear of the shadow sample.
    uint32_t shadow = pixels[94 * 240 + 212];
    screenshot("build/plant_before_hit.ppm");
    bool flashed = false;
    for(int tick = 0; tick < 1200 && ! flashed; ++tick)
    {
        frames(core, 0, 1);
        int red_pixels = 0;
        for(int y = 75; y < 93; ++y)
        {
            for(int x = 212; x < 231; ++x)
            {
                uint32_t color = pixels[y * 240 + x];
                red_pixels += (color & 255) > 170 && (color >> 8 & 255) < 100 && (color >> 16 & 255) < 100;
            }
        }
        if(red_pixels > 40)
        {
            screenshot("build/plant_red_flash.ppm");
            assert(pixels[94 * 240 + 212] == shadow);
            flashed = true;
        }
    }
    assert(flashed);
    reset_cartridge(core);
    press(core, 1 << 0);
    start_round(core, NULL);
}

static void browse_almanac(struct mCore* core)
{
    uint32_t title = region_hash(0, 0, 240, 40);
    press(core, 1 << 2);
    assert(region_hash(0, 0, 240, 40) == title);
    press(core, 1 << 7);
    screenshot("build/title_almanac_option.ppm");
    press(core, 1 << 0);
    screenshot("build/almanac_plants.ppm");
    uint32_t first_cell = region_hash(27, 43, 54, 3);
    press(core, 1 << 7);
    assert(region_hash(27, 43, 54, 3) != first_cell);
    press(core, 1 << 6);
    assert(region_hash(27, 43, 54, 3) == first_cell);
    uint32_t first_page = region_hash(70, 31, 100, 8);
    for(int i = 0; i < 6; ++i) { press(core, 1 << 4); }
    assert(region_hash(70, 31, 100, 8) == first_page);
    assert(region_hash(27, 43, 54, 3) == first_cell); // Right wraps inside page one.
    press(core, 1 << 5);
    assert(region_hash(70, 31, 100, 8) == first_page); // Left cannot open the last page.
    press(core, 1 << 4);
    for(int tab = 0; tab < 2; ++tab)
    {
        uint32_t previous_name = 0;
        int entries = tab ? 8 : 10;
        for(int i = 0; i < entries; ++i)
        {
            uint32_t grid_heading = region_hash(70, 31, 100, 8);
            int selected = i;
            int x = 27 + selected % 6 % 3 * 66;
            int y = 43 + selected % 6 / 3 * 46;
            uint32_t cell = region_hash(x, y, 54, 3);
            press(core, 1 << 0);
            check_portrait_pixels();
            uint32_t name = region_hash(65, 30, 110, 8);
            assert(name != previous_name);
            previous_name = name;
            char path[80];
            snprintf(path, sizeof(path), "build/almanac_%s_%d.ppm", tab ? "zombie" : "plant", selected);
            screenshot(path);
            if(tab && selected == 5) { screenshot("build/almanac_screen_door.ppm"); }
            if(tab && selected == 4) { screenshot("build/almanac_bucket.ppm"); }
            press(core, 1 << 8); // An open entry is modal; shoulder buttons cannot change it.
            assert(region_hash(65, 30, 110, 8) == name);
            press(core, 1 << 1);
            assert(region_hash(70, 31, 100, 8) == grid_heading);
            assert(region_hash(x, y, 54, 3) == cell);
            if(i == entries - 1)
            {
                press(core, 1 << 4);
                assert(region_hash(70, 31, 100, 8) == grid_heading);
                press(core, 1 << 5);
                assert(region_hash(70, 31, 100, 8) == grid_heading);
                assert(region_hash(x, y, 54, 3) == cell);
            }
            if(i < entries - 1) { press(core, 1 << (i == 5 ? 8 : 4)); }
        }
        if(tab == 0)
        {
            press(core, 1 << 8);
            screenshot("build/almanac_zombies.ppm");
            uint32_t idle = region_hash(38, 46, 32, 32);
            frames(core, 0, 32);
            assert(region_hash(38, 46, 32, 32) != idle);
            screenshot("build/almanac_zombies_animated.ppm");
        }
    }
    press(core, 1 << 1);
    assert(region_hash(0, 0, 240, 40) == title);
    press(core, 1 << 6);
}

#include "rom_gallery.h"

int main(int argc, char** argv)
{
    bool v2 = argc > 2 && strcmp(argv[2], "v2") == 0;
    bool legacy = argc > 2 && strcmp(argv[2], "legacy") == 0;
    bool loss = argc > 2 && strcmp(argv[2], "loss") == 0;
    bool gallery = argc > 2 && strcmp(argv[2], "gallery") == 0;
    bool debug = gallery || (argc > 2 && strcmp(argv[2], "debug") == 0);
    debug_seed_bank = debug;
    int offset = debug ? PVZ_SAVE_DEBUG_OFFSET : PVZ_SAVE_NORMAL_OFFSET;
    struct mLogger logger = {.log = log_message, .filter = NULL};
    mLogSetDefaultLogger(&logger);
    struct mCore* core = mCoreFind(argc > 1 ? argv[1] : "plants_vs_zombies.gba");
    assert(core && core->init(core));
    mCoreInitConfig(core, NULL);
    core->setVideoBuffer(core, pixels, 240);
    assert(mCoreLoadFile(core, argc > 1 ? argv[1] : "plants_vs_zombies.gba"));
    core->reset(core);
    if(legacy) { legacy_save(core); }
    if(v2) { v2_save(core); }
    frames(core, 0, 120);
    assert(saved_byte(core, offset, 5) == (v2 ? 8 : (legacy ? 3 : (debug ? 10 : 1))));
    if(v2)
    {
        assert(saved_byte(core, offset, 6) == 6 && saved_byte(core, offset, 8) == 63);
        for(int type = 0; type < 8; ++type) { assert(saved_kills(core, offset, type) == (type < 7 ? (uint32_t)(10 + type) : 0)); }
        assert(core->busRead8(core, 0x0e000000 + PVZ_SAVE_V2_NORMAL_OFFSET + 3) == '2');
        reset_cartridge(core);
        for(int type = 0; type < 8; ++type) { assert(saved_kills(core, offset, type) == (type < 7 ? (uint32_t)(10 + type) : 0)); }
        puts("PASS PVZ2 SRAM migration preserves all seven existing counters and progression across reload");
        mCoreConfigDeinit(&core->config); core->deinit(core); return 0;
    }
    if(legacy)
    {
        assert(saved_byte(core, offset, 6) == 1 && saved_byte(core, offset, 8) == 1);
        for(int type = 0; type < 6; ++type) { assert(saved_kills(core, offset, type) == 0); }
        reset_cartridge(core);
        assert(saved_byte(core, offset, 5) == 3 && saved_byte(core, offset, 6) == 1);
        puts("PASS legacy SRAM migration preserves unlocks, selection and completion");
        mCoreConfigDeinit(&core->config); core->deinit(core); return 0;
    }
    assert(measure_audio(core, 60) > 1000);
    screenshot(debug ? "build/debug_title.ppm" : "build/title.ppm");
    if(! debug && ! loss)
    {
        for(int frame = 0; frame < 12; ++frame)
        {
            char path[80];
            snprintf(path, sizeof(path), "build/title_animation_%02d.ppm", frame);
            screenshot(path);
            frames(core, 0, 12);
        }
    }
    browse_almanac(core);
    press(core, 1 << 0);
    assert(! on_lawn());
    screenshot(debug ? "build/levels_debug.ppm" : "build/levels_locked.ppm");
    uint32_t first_badge = region_hash(38, 70, 13, 11);
    uint32_t second_badge = region_hash(76, 70, 13, 11);
    assert(debug ? first_badge == second_badge : first_badge != second_badge);
    if(loss)
    {
        start_round(core, NULL);
        for(int i = 0; i < 30000 && first_level_active(core); ++i) { frames(core, 0, 1); }
        assert(! first_level_active(core));
        assert(measure_audio(core, 12) > 1000); // Record scratch.
        uint32_t before = image_hash();
        press(core, 1 << 0); // Cannot skip the breach animation.
        assert(! first_level_active(core));
        frames(core, 0, 8); // Let the recorded scratch finish before checking the voice.
        assert(measure_audio(core, 12) > 1000); // Scream follows the scratch.
        for(int frame = 0; frame < 10; ++frame)
        {
            char path[80];
            frames(core, 0, 6);
            snprintf(path, sizeof(path), "build/loss_sequence_%02d.ppm", frame);
            screenshot(path);
        }
        screenshot("build/loss_animation.ppm");
        assert(image_hash() != before);
        frames(core, 0, PVZ_DEFEAT_READY_FRAME);
        screenshot("build/loss_prompt.ppm");
        assert(measure_audio(core, 60) < 100); // Music stays stopped after the effects finish.
        start_round(core, NULL);
        assert(measure_audio(core, 30) > 1000); // Retry restores the music.
        assert(first_level_active(core));
        for(int i = 0; i < 30000 && first_level_active(core); ++i) { frames(core, 0, 1); }
        frames(core, 0, PVZ_DEFEAT_READY_FRAME);
        press(core, 1 << 1);
        assert(! on_lawn());
    }
    else if(debug)
    {
        press(core, 1 << 7);
        for(int i = 0; i < 4; ++i) { press(core, 1 << 4); }
        screenshot("build/level_10_selected.ppm");
        start_round(core, "build/preview_level_10.ppm");
        assert(on_lawn() && saved_byte(core, offset, 6) == 9);
        screenshot("build/debug_level_10.ppm");
        reset_cartridge(core);
        assert(saved_byte(core, offset, 5) == 10 && saved_byte(core, offset, 6) == 9);
        press(core, 1 << 0);
        start_round(core, NULL);
        assert(on_lawn());
        assert(core->busRead8(core, 0x0e000100) != 'P'); // Debug save is isolated.
        tombstone_test(core);
        grave_emergence_test(core);
        explosion_layer_test(core);
        plant_damage_test(core);
        // Six repeated steps wrap around; four more select Snow Pea in the new order.
        frames(core, 1 << 8, 75);
        frames(core, 0, 3);
        assert((pixels[24 * 240 + 55] & 255) > 200);
        frames(core, 1 << 9, 75);
        frames(core, 0, 3);
        assert((pixels[24 * 240 + 55] & 255) > 200);
        frames(core, 1 << 8, 55);
        frames(core, 0, 3);
        assert((pixels[24 * 240 + 167] & 255) > 200);
        press(core, 1 << 6);
        press(core, 1 << 4);
        press(core, 1 << 4);
        press(core, 1 << 0);
        int ice = 0, hurt_seen = 0;
        // Observe the visible hit flash, then the blue freeze tint on the same lane.
        for(int tick = 0; tick < 1200 && (ice <= 20 || hurt_seen <= 8); ++tick)
        {
            frames(core, 1 << 1, 1);
            ice = 0;
            int hurt = 0;
            for(int y = 72; y < 96; ++y)
            {
                for(int x = 120; x < 232; ++x)
                {
                    uint32_t pixel = pixels[y * 240 + x];
                    int red = pixel & 255, green = pixel >> 8 & 255, blue = pixel >> 16 & 255;
                    ice += blue > red + 30 && blue > green;
                    hurt += red > 170 && green < 100 && blue < 100;
                }
            }
            if(hurt > 8 && hurt_seen <= 8) { screenshot("build/zombie_damage_flash.ppm"); }
            if(hurt > hurt_seen) { hurt_seen = hurt; }
        }
        assert(hurt_seen > 8);
        assert(on_lawn());
        screenshot("build/snow_pea_gameplay.ppm");
        assert(ice > 20); // The slowed zombie is visibly blue, beyond the Snow Pea itself.
        reset_cartridge(core);
        press(core, 1 << 0);
        start_round(core, NULL);
        frames(core, 0, 3450);
        assert(on_lawn());
        screenshot("build/armored_horde.ppm");
    }
    else
    {
        press(core, 1 << 4);
        uint32_t locked = image_hash();
        press(core, 1 << 0);
        assert(image_hash() == locked && ! on_lawn());
        press(core, 1 << 5);
        start_round(core, "build/preview_level_1.ppm");
        assert(on_lawn());
        screenshot("build/playing.ppm");
        uint32_t selected_edge = pixels[24 * 240 + 55];
        assert((selected_edge & 255) > 220 && (selected_edge >> 8 & 255) > 180 && (selected_edge >> 16 & 255) < 130);
        press(core, 1 << 8);
        selected_edge = pixels[24 * 240 + 55];
        assert((selected_edge & 255) > 220 && (selected_edge >> 8 & 255) > 180 && (selected_edge >> 16 & 255) < 130);
        press(core, 1 << 9);
        press(core, 1 << 3);
        screenshot("build/paused.ppm");
        press(core, 1 << 1);
        assert(on_lawn());
        press(core, 1 << 0); // Plant a Peashooter, then try an empty cell during its cooldown.
        press(core, 1 << 4);
        press(core, 1 << 0);
        screenshot("build/cooldown_cell.ppm");
        int sand = 0;
        for(int y = 91; y < 107; ++y)
        {
            for(int x = 59; x < 75; ++x)
            {
                uint32_t pixel = pixels[y * 240 + x];
                sand += (pixel & 255) > 220 && (pixel >> 8 & 255) > 180 && (pixel >> 16 & 255) < 130;
            }
        }
        assert(sand >= 8); // Hourglass feedback appears on the target cell, clear of the seed bank.
        reset_cartridge(core);
        press(core, 1 << 0);
        start_round(core, NULL);
        assert(on_lawn());
        clear_first_level(core);
        assert(saved_byte(core, offset, 5) == 2 && (saved_byte(core, offset, 8) & 1));
        assert(saved_kills(core, offset, 0) == 28 && saved_kills(core, offset, 3) == 3);
        screenshot("build/level_1_clear.ppm");
        uint32_t reward_start = image_hash();
        uint32_t reward_lawn = region_hash(0, 58, 30, 28);
        press(core, 1 << 0); // Early input cannot skip the reveal.
        assert(saved_byte(core, offset, 6) == 0);
        for(int i = 0; i < 56; ++i)
        {
            char path[80];
            frames(core, 0, 6);
            snprintf(path, sizeof(path), "build/reward_%02d.ppm", i);
            screenshot(path);
            if(i == 18)
            {
                screenshot("build/plant_reward_mid.ppm");
                assert(image_hash() != reward_start);
            }
        }
        screenshot("build/plant_reward.ppm");
        assert(region_hash(0, 58, 30, 28) == reward_lawn); // The lawn edge remains visible behind the reward.
        assert(on_lawn()); // The sun bank, wave track and packets remain visible through the reward.
        press(core, 1 << 8); // A shoulder button dismisses the unlock to level selection.
        assert(! on_lawn());
        assert(region_hash(76, 70, 13, 11) != second_badge);
        screenshot("build/levels_progress.ppm");
        start_round(core, NULL);
        assert(on_lawn() && saved_byte(core, offset, 6) == 1);
        screenshot("build/lawn_three_lanes.ppm");
        reset_cartridge(core);
        assert(saved_byte(core, offset, 5) == 2 && saved_byte(core, offset, 6) == 1);
        assert(saved_kills(core, offset, 0) == 28 && saved_kills(core, offset, 3) == 3);
        press(core, 1 << 0);
        screenshot("build/levels_reloaded.ppm");
        assert(region_hash(76, 70, 13, 11) != second_badge);
        int newest = save_slot(core, offset);
        core->busWrite8(core, 0x0e000000 + offset + newest * PVZ_SAVE_SIZE + PVZ_SAVE_CHECKSUM,
                       core->busRead8(core, 0x0e000000 + offset + newest * PVZ_SAVE_SIZE + PVZ_SAVE_CHECKSUM) ^ 1);
        reset_cartridge(core);
        assert(saved_byte(core, offset, 5) == 2); // Backup retains completion and unlock.
        press(core, 1 << 0);
        press(core, 1 << 5); // Replay the already-completed first level.
        start_round(core, NULL);
        assert(first_level_active(core));
        clear_first_level(core);
        assert(! first_level_active(core));
        frames(core, 0, 120);
        screenshot("build/trophy_bounce.ppm");
        frames(core, 0, 210);
        screenshot("build/trophy_reward.ppm");
        assert(saved_kills(core, offset, 0) == 56 && saved_kills(core, offset, 3) == 6);
        assert(image_hash() != reward_start);
        press(core, 1 << 1); // B also dismisses a repeat-clear trophy to levels, rather than title.
        assert(! on_lawn());
        assert(region_hash(76, 70, 13, 11) != second_badge);
    }
    if(! debug && ! loss)
    {
        press(core, 1 << 1);
        press(core, 1 << 7);
        press(core, 1 << 0);
        press(core, 1 << 8);
        press(core, 1 << 0);
        screenshot("build/almanac_lifetime_kills.ppm");
        assert(saved_kills(core, offset, 0) == 56 && saved_kills(core, offset, 3) == 6);
        reset_cartridge(core);
        assert(saved_kills(core, offset, 0) == 56 && saved_kills(core, offset, 3) == 6);
    }
    if(gallery) { capture_gallery(core); }
    printf("PASS %s selection, locks, SRAM progress, reload, and save isolation\n", loss ? "loss animation and retry" : (debug ? "debug" : "normal"));
    mCoreConfigDeinit(&core->config);
    core->deinit(core);
    return 0;
}
