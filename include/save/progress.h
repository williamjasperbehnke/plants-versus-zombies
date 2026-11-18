#ifndef GARDEN_PROGRESS_H
#define GARDEN_PROGRESS_H
#include "save/save_layout.h"

#include <array>
#include <cstdint>

namespace garden {
inline constexpr int zombie_kind_count = 8;
inline constexpr int save_slot_count = PVZ_SAVE_SLOT_COUNT;
inline constexpr int levels_per_world = 10;
inline constexpr int world_count = 1;
inline constexpr int level_count = levels_per_world * world_count;
static_assert(level_count < 32, "Increase the save format capacity before adding more worlds");

struct WorldSpec {
    const char* name;
    int first_level;
    int count;
};

inline constexpr std::array<WorldSpec, world_count> worlds{{{"Backyard", 0, 10}}};

struct Progress {
    uint32_t completed = 0;
    int unlocked = 1;
    int selected = 0;
    std::array<uint32_t, zombie_kind_count> zombie_kills{};

    bool valid() const {
        return unlocked >= 1 && unlocked <= level_count && selected >= 0 && selected < unlocked &&
               (completed >> level_count) == 0;
    }

    int cleared_levels() const {
        int count = 0;
        for (auto remaining = completed; remaining; remaining >>= 1) {
            count += remaining & 1;
        }
        return count;
    }

    static Progress all_unlocked() {
        return {0, level_count, 0};
    }

    bool operator==(const Progress& other) const {
        return completed == other.completed && unlocked == other.unlocked && selected == other.selected &&
               zombie_kills == other.zombie_kills;
    }
};

namespace save_format {
inline constexpr uint32_t magic = PVZ_SAVE_MAGIC; // PVZ3, little endian.
inline constexpr int version = PVZ_SAVE_VERSION;
inline constexpr int record_size = PVZ_SAVE_SIZE;
inline constexpr int state_offset = 7; // 0: occupied, 1: deleted journal entry.
inline constexpr int completed_offset = 8;
inline constexpr int sequence_offset = 12;
inline constexpr int kills_offset = 20;
inline constexpr int checksum_offset = record_size - 4;

// Counter offsets are part of the current PVZ3 record layout.
constexpr int kill_offset(int type) {
    return PVZ_SAVE_KILL_OFFSET(type);
}

static_assert(kills_offset + 7 * 4 == checksum_offset);
} // namespace save_format

using SaveRecord = std::array<uint8_t, save_format::record_size>;

inline uint32_t save_word(const SaveRecord& data, int offset) {
    return uint32_t(data[offset]) | uint32_t(data[offset + 1]) << 8 | uint32_t(data[offset + 2]) << 16 |
           uint32_t(data[offset + 3]) << 24;
}

inline void put_save_word(SaveRecord& data, int offset, uint32_t value) {
    for (int i = 0; i < 4; ++i) {
        data[offset + i] = uint8_t(value >> (i * 8));
    }
}

inline uint32_t save_checksum(const SaveRecord& data) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < save_format::checksum_offset; ++i) {
        hash = (hash ^ data[i]) * 16777619u;
    }
    return hash;
}

inline SaveRecord encode_progress(const Progress& progress, uint32_t sequence, bool occupied = true) {
    SaveRecord data{};
    put_save_word(data, 0, save_format::magic);
    data[4] = save_format::version;
    data[5] = uint8_t(progress.unlocked);
    data[6] = uint8_t(progress.selected);
    data[save_format::state_offset] = occupied ? 0 : 1;
    put_save_word(data, save_format::completed_offset, progress.completed);
    put_save_word(data, save_format::sequence_offset, sequence);
    for (int i = 0; i < zombie_kind_count; ++i) {
        put_save_word(data, save_format::kill_offset(i), progress.zombie_kills[i]);
    }
    put_save_word(data, save_format::checksum_offset, save_checksum(data));
    return data;
}

inline bool decode_progress(const SaveRecord& data, Progress& progress) {
    Progress candidate{save_word(data, save_format::completed_offset), data[5], data[6]};
    if (save_word(data, 0) != save_format::magic || data[4] != save_format::version ||
        data[save_format::state_offset] > 1 ||
        save_word(data, save_format::checksum_offset) != save_checksum(data) || !candidate.valid()) {
        return false;
    }
    for (int i = 0; i < zombie_kind_count; ++i) {
        candidate.zombie_kills[i] = save_word(data, save_format::kill_offset(i));
    }
    progress = candidate;
    return true;
}
} // namespace garden
#endif
