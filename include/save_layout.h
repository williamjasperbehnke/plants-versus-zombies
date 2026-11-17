#ifndef GARDEN_SAVE_LAYOUT_H
#define GARDEN_SAVE_LAYOUT_H
// Current-format layout shared with the C emulator harness.
#define PVZ_SAVE_VERSION 3
#define PVZ_SAVE_MAGIC 0x335A5650u
#define PVZ_SAVE_SIZE 52
#define PVZ_SAVE_CHECKSUM 48
#define PVZ_SAVE_NORMAL_OFFSET 768
#define PVZ_SAVE_DEBUG_OFFSET 1024
#define PVZ_SAVE_KILL_OFFSET(type) ((type) == 6 ? 16 : ((type) == 7 ? 44 : 20 + (type) * 4))
#endif
