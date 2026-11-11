// Optional headless ROM integration check. Requires libmgba development headers.
// Build: cc tools/rom_smoke.c -I/usr/local/include -L/usr/local/lib -lmgba -o build/rom_smoke
#include <mgba/flags.h>
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/core/blip_buf.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static color_t pixels[240 * 160];
static bool capture_audio;
static uint64_t audio_energy;

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

static uint32_t region_hash(int x, int y, int width, int height)
{
    uint32_t hash = 2166136261u;
    for(int row = y; row < y + height; ++row)
    {
        for(int column = x; column < x + width; ++column)
        { hash = (hash ^ pixels[row * 240 + column]) * 16777619u; }
    }
    return hash;
}

static uint32_t image_hash(void) { return region_hash(0, 0, 240, 160); }

static bool first_flag_cleared(void)
{
    uint32_t cloth = pixels[9 * 240 + 84];
    return (cloth & 255) < 220 && ((cloth >> 8) & 255) > 150 && ((cloth >> 16) & 255) > 70;
}

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

int main(int argc, char** argv)
{
    struct mLogger logger = { .log = log_message, .filter = NULL };
    mLogSetDefaultLogger(&logger);
    const char* rom = argc > 1 ? argv[1] : "garden_guard.gba";
    struct mCore* core = mCoreFind(rom);
    if(! core || ! core->init(core)) { fputs("mGBA init failed\n", stderr); return 1; }
    mCoreInitConfig(core, NULL);
    core->setVideoBuffer(core, pixels, 240);
    if(! mCoreLoadFile(core, rom)) { fputs("ROM load failed\n", stderr); return 1; }
    core->reset(core);
    frames(core, 0, 120);
    screenshot("build/title.ppm");
    uint32_t title_hash = image_hash();
    press(core, 1 << 3); // START
    uint32_t initial_hash = image_hash();
    uint32_t packet_illustration_hash = region_hash(56, 28, 16, 16);
    uint32_t soil_border_hash = region_hash(0, 154, 240, 6);
    assert(initial_hash != title_hash);
    press(core, 1 << 0); // A: sunflower at row 2, column 0
    press(core, 1 << 0); // Occupied tile: shake and cross.
    screenshot("build/occupied.ppm");
    press(core, 1 << 6);
    uint32_t empty_feedback_cell_hash = region_hash(37, 78, 16, 16);
    press(core, 1 << 0); // Seed cooling down: hourglass badge on packet illustration.
    screenshot("build/cooldown.ppm");
    assert(region_hash(56, 28, 16, 16) != packet_illustration_hash);
    assert(region_hash(37, 78, 16, 16) == empty_feedback_cell_hash);
    press(core, 1 << 7);
    frames(core, 1 << 2, 2); // SELECT: shovel cursor/feedback, without removing.
    screenshot("build/shovel.ppm");
    frames(core, 0, 2);
    frames(core, 0, 250);
    press(core, 1 << 6); // UP
    press(core, 1 << 0); // A: another sunflower
    press(core, 1 << 1); // B: collect
    press(core, 1 << 8); // R: peashooter
    press(core, 1 << 4); // RIGHT
    press(core, 1 << 4);
    press(core, 1 << 0); // A: pea if affordable
    frames(core, 0, 650);
    screenshot("build/sun_collection_before.ppm");
    press(core, 1 << 1);
    screenshot("build/sun_collection_start.ppm");
    frames(core, 0, 8);
    screenshot("build/sun_collection_mid.ppm");
    frames(core, 0, 18);
    screenshot("build/sun_collection_arrived.ppm");
    press(core, 1 << 0);
    frames(core, 0, 300);
    screenshot("build/playing.ppm");
    assert(region_hash(0, 154, 240, 6) == soil_border_hash);
    press(core, 1 << 3);
    screenshot("build/paused.ppm");
    uint32_t paused_hash = image_hash();
    frames(core, 0, 120);
    assert(image_hash() == paused_hash);
    press(core, 1 << 3);
    frames(core, 0, 16000);
    screenshot("build/defeat.ppm");
    assert(image_hash() != initial_hash && image_hash() != paused_hash);
    press(core, 1 << 3);
    frames(core, 0, 5);
    screenshot("build/restarted.ppm");
    assert(image_hash() == initial_hash);
    // Exercise every pause action and both menu-entry buttons.
    press(core, 1 << 3);
    press(core, 1 << 7); // DOWN: restart
    press(core, 1 << 0);
    assert(image_hash() == initial_hash);
    press(core, 1 << 3);
    press(core, 1 << 7); // DOWN: restart
    press(core, 1 << 7); // DOWN: title
    press(core, 1 << 0);
    assert(image_hash() == title_hash);
    press(core, 1 << 0); // A also starts from title.
    assert(image_hash() == initial_hash);
    press(core, 1 << 3);
    press(core, 1 << 1); // B resumes.
    assert(image_hash() == initial_hash);
    press(core, 1 << 3);
    press(core, 1 << 0); // A confirms Resume.
    assert(image_hash() == initial_hash);
    // Exercise the larger explosion sprite and its three animation frames.
    press(core, 1 << 9); // L: sunflower -> cherry bomb
    press(core, 1 << 0);
    frames(core, 0, 64);
    screenshot("build/explosion.ppm");
    frames(core, 0, 6);
    screenshot("build/explosion_mid.ppm");
    frames(core, 0, 6);
    screenshot("build/explosion_end.ppm");
    frames(core, 0, 20);
    press(core, 1 << 9); // Cherry -> wall-nut -> peashooter; only 25 sun remains.
    press(core, 1 << 9);
    uint32_t stationary_bank_hash = region_hash(0, 24, 45, 29);
    press(core, 1 << 0);
    screenshot("build/unaffordable.ppm");
    assert(region_hash(0, 24, 45, 29) == stationary_bank_hash);
    uint32_t empty_bar_hash = region_hash(56, 0, 176, 21);
    frames(core, 0, 800);
    screenshot("build/wave_flags.ppm");
    frames(core, 0, 1800);
    screenshot("build/wave_progress.ppm");
    assert(region_hash(56, 0, 176, 21) != empty_bar_hash);
    // Holding B collects sky sun without requiring another button press.
    uint32_t sun_before_collect = region_hash(0, 41, 45, 10);
    frames(core, 1 << 1, 400);
    screenshot("build/hold_collect.ppm");
    assert(region_hash(0, 41, 45, 10) != sun_before_collect);
    frames(core, 0, 400);
    // At the first completed wave, the green flag and lowered head share x=89.
    int checkpoint_wait = 0;
    while(! first_flag_cleared() && checkpoint_wait++ < 600)
    { frames(core, 0, 1); }
    assert(first_flag_cleared());
    uint32_t head_chin = pixels[20 * 240 + 89];
    assert((head_chin & 255) > 90 && (head_chin & 255) < 150);
    assert(((head_chin >> 8) & 255) > 110 && ((head_chin >> 16) & 255) > 90);
    screenshot("build/wave_checkpoint.ppm");
    // Check the mixer while paused, with no gameplay sounds.
    press(core, 1 << 3);
    assert(measure_audio(core, 120) > 1000);
    for(int row = 0; row < 4; ++row) { press(core, 1 << 7); } // Music
    frames(core, 1 << 5, 80); // LEFT: mute music
    frames(core, 0, 20);
    assert(measure_audio(core, 60) == 0);
    screenshot("build/audio_music_muted.ppm");
    audio_energy = 0;
    capture_audio = true;
    press(core, 1 << 7); // Effects: navigation sound with music muted
    frames(core, 0, 20);
    capture_audio = false;
    assert(audio_energy > 1000);
    frames(core, 1 << 5, 80); // Mute effects
    frames(core, 0, 20);
    audio_energy = 0;
    capture_audio = true;
    press(core, 1 << 6); // Music: navigation must now be silent
    frames(core, 0, 20);
    capture_audio = false;
    assert(audio_energy == 0);
    for(int step = 0; step < 5; ++step) { press(core, 1 << 4); } // Music 50%
    press(core, 1 << 6); // Master
    frames(core, 1 << 5, 80);
    frames(core, 0, 20);
    assert(measure_audio(core, 60) == 0);
    screenshot("build/audio_master_muted.ppm");
    frames(core, 1 << 4, 80); // Restore master
    frames(core, 0, 20);
    assert(measure_audio(core, 120) > 1000);
    press(core, 1 << 7); // Music: mute for isolated chewing check below
    frames(core, 1 << 5, 80);
    frames(core, 0, 20);
    press(core, 1 << 7); // Effects: restore
    frames(core, 1 << 4, 80);
    frames(core, 0, 20);
    for(int row = 0; row < 4; ++row) { press(core, 1 << 6); } // Restart
    press(core, 1 << 0);
    press(core, 1 << 3);
    frames(core, 0, 20);
    assert(measure_audio(core, 60) == 0); // Music mute survives restart.
    press(core, 1 << 7);
    press(core, 1 << 7); // Title
    press(core, 1 << 0);
    frames(core, 0, 20);
    assert(measure_audio(core, 60) == 0); // And title visits.
    press(core, 1 << 0);
    // Only a wall-nut is planted and music stays muted after restarting.
    press(core, 1 << 8);
    press(core, 1 << 8);
    press(core, 1 << 6);
    for(int column = 0; column < 8; ++column) { press(core, 1 << 4); }
    press(core, 1 << 0);
    frames(core, 0, 900);
    audio_energy = 0;
    capture_audio = true;
    frames(core, 0, 100);
    capture_audio = false;
    assert(audio_energy > 1000);
    screenshot("build/chewing.ppm");
    assert(region_hash(0, 154, 240, 6) == soil_border_hash);
    printf("Chewing audio captured: %llu total sample amplitude\n", (unsigned long long)audio_energy);
    printf("ROM smoke completed: %u emulator frames\n", core->frameCounter(core));
    mCoreConfigDeinit(&core->config);
    core->deinit(core);
    return 0;
}
