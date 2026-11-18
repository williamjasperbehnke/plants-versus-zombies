// Documentation-only scenarios driven through ordinary buttons on the debug ROM.
// Included after the shared emulator helpers in rom_smoke.c.
struct GalleryCursor {
    int row, column, seed;
};

static void gallery_move(struct mCore* core, struct GalleryCursor* cursor, int row, int column) {
    while (cursor->row != row) {
        press(core, 1 << (cursor->row > row ? 6 : 7));
        cursor->row += cursor->row > row ? -1 : 1;
    }
    while (cursor->column != column) {
        press(core, 1 << (cursor->column > column ? 5 : 4));
        cursor->column += cursor->column > column ? -1 : 1;
    }
}

static void gallery_seed(struct mCore* core, struct GalleryCursor* cursor, int seed) {
    while (cursor->seed != seed) {
        press(core, 1 << 8);
        cursor->seed = (cursor->seed + 1) % 6;
    }
}

static void gallery_plant(struct mCore* core, struct GalleryCursor* cursor, int row, int column, int seed) {
    gallery_move(core, cursor, row, column);
    gallery_seed(core, cursor, seed);
    press(core, 1 << 0);
}

static void gallery_restart(struct mCore* core) {
    reset_cartridge(core);
    open_levels(core);
    start_round(core, NULL); // Debug checks leave Backyard 1-10 selected.
}

static void gallery_feedback_and_audio(struct mCore* core) {
    gallery_restart(core);
    struct GalleryCursor cursor = {2, 0, 0};
    gallery_plant(core, &cursor, 2, 0, 0);
    screenshot("build/placement_check.ppm");
    press(core, 1 << 0);
    screenshot("build/placement_occupied.ppm");
    gallery_move(core, &cursor, 2, 1);
    press(core, 1 << 0);
    screenshot("build/placement_cooldown.ppm");
    gallery_seed(core, &cursor, 4);
    press(core, 1 << 0);
    screenshot("build/placement_unaffordable.ppm");
    frames(core, 0, 620);
    screenshot("build/sun_before.ppm");
    uint32_t bank = region_hash(0, 38, 40, 16);
    frames(core, 1 << 1, 3); // Allow the video frame to present the collected total.
    screenshot("build/sun_start.ppm");
    assert(region_hash(0, 38, 40, 16) != bank);
    frames(core, 0, 11);
    screenshot("build/sun_mid.ppm");
    frames(core, 0, 13);
    screenshot("build/sun_arrived.ppm");
    gallery_move(core, &cursor, 2, 0);
    frames(core, 1 << 2, 1);
    screenshot("build/shovel_selected.ppm");
    press(core, (1 << 2) | (1 << 0));
    screenshot("build/shovel_removed.ppm");
    press(core, 1 << 3);
    for (int i = 0; i < 3; ++i) {
        press(core, 1 << 7);
    } // Master.
    for (int i = 0; i < 10; ++i) {
        press(core, 1 << 5);
    }
    assert(measure_audio(core, 20) < 100);
    screenshot("build/mixer_master_muted.ppm");
    for (int i = 0; i < 10; ++i) {
        press(core, 1 << 4);
    }
    assert(measure_audio(core, 20) > 1000);
    press(core, 1 << 7); // Music.
    for (int i = 0; i < 10; ++i) {
        press(core, 1 << 5);
    }
    screenshot("build/mixer_music_muted.ppm");
    for (int i = 0; i < 10; ++i) {
        press(core, 1 << 4);
    }
    press(core, 1 << 7); // Effects.
    for (int i = 0; i < 10; ++i) {
        press(core, 1 << 5);
    }
    assert(measure_audio(core, 20) > 1000);
    screenshot("build/mixer_effects_muted.ppm");
}

static void gallery_mine(struct mCore* core) {
    gallery_restart(core);
    struct GalleryCursor cursor = {2, 0, 0};
    gallery_plant(core, &cursor, 1, 5, 3);
    frames(core, 0, 30); // Let planting feedback clear so the unarmed mound is visible.
    screenshot("build/mine_arming.ppm");
    gallery_plant(core, &cursor, 2, 0, 0);
    frames(core, 1 << 1, 480);
    screenshot("build/mine_armed.ppm");
    bool burst = false;
    for (int tick = 0; tick < 1400 && !burst; ++tick) {
        frames(core, 1 << 1, 1);
        int white = 0;
        for (int y = 89; y < 95; ++y) {
            for (int x = 151; x < 162; ++x) {
                uint32_t pixel = pixels[y * 240 + x];
                white += (pixel & 255) > 240 && (pixel >> 8 & 255) > 220 && (pixel >> 16 & 255) > 150;
            }
        }
        burst = white > 15;
    }
    assert(burst && on_lawn());
    screenshot("build/mine_burst.ppm");
    frames(core, 1 << 1, 20);
    screenshot("build/mine_spent.ppm");
}

static void gallery_wall_pose(struct mCore* core, const char* path) {
    // Capture the damage shape between red flashes, with the zombie kept out of the sample.
    for (int tick = 0; tick < 20; ++tick) {
        int red = 0;
        for (int y = 78; y < 93; ++y) {
            for (int x = 212; x < 227; ++x) {
                uint32_t pixel = pixels[y * 240 + x];
                red += (pixel & 255) > 190 && (pixel >> 8 & 255) < 80 && (pixel >> 16 & 255) < 80;
            }
        }
        if (red <= 5) {
            break;
        }
        frames(core, 0, 1);
    }
    screenshot(path);
}

static void gallery_wall_and_mower(struct mCore* core) {
    gallery_restart(core);
    struct GalleryCursor cursor = {2, 0, 0};
    gallery_plant(core, &cursor, 1, 8, 2);
    gallery_move(core, &cursor, 1, 6);
    frames(core, 0, 30);
    screenshot("build/wallnut_healthy.ppm");
    frames(core, 1 << 1, 1628);
    gallery_wall_pose(core, "build/wallnut_damaged.ppm");
    frames(core, 1 << 1, 550);
    gallery_wall_pose(core, "build/wallnut_worn.ppm");
    gallery_restart(core);
    frames(core, 0, 2728);
    assert(on_lawn());
    screenshot("build/mower_active.ppm");
}

static void gallery_zombie_death(struct mCore* core) {
    gallery_restart(core);
    struct GalleryCursor cursor = {2, 0, 0};
    gallery_plant(core, &cursor, 1, 2, 1);
    frames(core, 1 << 1, 900);
    for (int frame = 0; frame < 120; ++frame) {
        char path[80];
        frames(core, 1 << 1, 6);
        assert(on_lawn());
        snprintf(path, sizeof(path), "build/zombie_combat_%03d.ppm", frame);
        screenshot(path);
    }
    assert(saved_kills(core, PVZ_SAVE_DEBUG_OFFSET, 0) > 0);
}

static void gallery_flag_death(struct mCore* core) {
    gallery_restart(core);
    uint32_t before = saved_kills(core, PVZ_SAVE_DEBUG_OFFSET, 3);
    for (int tick = 0; tick < 6000 && saved_kills(core, PVZ_SAVE_DEBUG_OFFSET, 3) == before; ++tick) {
        frames(core, 0, 1);
        assert(on_lawn());
    }
    assert(saved_kills(core, PVZ_SAVE_DEBUG_OFFSET, 3) > before);
    for (int frame = 0; frame < 24; ++frame) {
        char path[80];
        frames(core, 0, 6);
        assert(on_lawn());
        snprintf(path, sizeof(path), "build/flag_death_%02d.ppm", frame);
        screenshot(path);
    }
}

static void gallery_road_entry(struct mCore* core) {
    gallery_restart(core);
    frames(core, 0, 870);
    uint32_t empty = region_hash(236, 73, 4, 25);
    int delay = 0;
    while (region_hash(236, 73, 4, 25) == empty && delay < 100) {
        frames(core, 0, 1);
        ++delay;
    }
    assert(delay > 0 && delay < 100);
    screenshot("build/road_first_pixel.ppm");
    for (int frame = 0; frame < 48; ++frame) {
        char path[80];
        frames(core, 0, 6);
        snprintf(path, sizeof(path), "build/road_entry_%02d.ppm", frame);
        screenshot(path);
    }
}

static void gallery_new_plants(struct mCore* core) {
    seed_bank_scenario = 1;
    gallery_restart(core);
    seed_bank_scenario = 0;
    struct GalleryCursor cursor = {2, 0, 0};
    gallery_plant(core, &cursor, 0, 0, 0);
    frames(core, 1 << 1, 250);
    gallery_plant(core, &cursor, 3, 0, 0);
    frames(core, 1 << 1, 250);
    gallery_plant(core, &cursor, 4, 0, 0);
    frames(core, 1 << 1, 900);
    gallery_plant(core, &cursor, 2, 2, 5);
    frames(core, 1 << 1, 80);
    for (int frame = 0; frame < 24; ++frame) {
        char path[80];
        snprintf(path, sizeof(path), "build/repeater_burst_%02d.ppm", frame);
        screenshot(path);
        frames(core, 1 << 1, 1);
    }
    screenshot("build/repeater_pair.ppm");
    frames(core, 1 << 1, 361);
    gallery_plant(core, &cursor, 1, 4, 4);
    for (int frame = 0; frame < 12; ++frame) {
        char path[80];
        snprintf(path, sizeof(path), "build/squash_leap_%02d.ppm", frame);
        screenshot(path);
        frames(core, 1 << 1, 4);
    }
    screenshot("build/new_plants_lawn.ppm");
}

static void gallery_chomper(struct mCore* core) {
    seed_bank_scenario = 2;
    gallery_restart(core);
    seed_bank_scenario = 0;
    struct GalleryCursor cursor = {2, 0, 0};
    gallery_plant(core, &cursor, 0, 0, 0);
    frames(core, 1 << 1, 250);
    gallery_plant(core, &cursor, 3, 0, 0);
    frames(core, 1 << 1, 250);
    gallery_plant(core, &cursor, 4, 0, 0);
    frames(core, 1 << 1, 900);
    uint32_t kills = saved_kills(core, PVZ_SAVE_DEBUG_OFFSET, 0);
    gallery_plant(core, &cursor, 1, 5, 5);
    for (int frame = 0; frame < 80; ++frame) {
        char path[80];
        snprintf(path, sizeof(path), "build/chomper_bite_%02d.ppm", frame);
        screenshot(path);
        frames(core, 1 << 1, 4);
    }
    assert(saved_kills(core, PVZ_SAVE_DEBUG_OFFSET, 0) > kills);
    screenshot("build/chomper_chewing.ppm");
}

static void capture_gallery(struct mCore* core) {
    gallery_feedback_and_audio(core);
    gallery_mine(core);
    gallery_wall_and_mower(core);
    gallery_zombie_death(core);
    gallery_flag_death(core);
    gallery_road_entry(core);
    gallery_new_plants(core);
    gallery_chomper(core);
    puts("PASS gallery placement, sun flights, shovel, mixer, mine and mower captures");
}
