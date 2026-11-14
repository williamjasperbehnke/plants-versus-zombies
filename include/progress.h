#ifndef GARDEN_PROGRESS_H
#define GARDEN_PROGRESS_H
#include <array>
#include <cstdint>
namespace garden
{
inline constexpr int zombie_kind_count = 6;
inline constexpr int levels_per_world = 10;
inline constexpr int world_count = 1;
inline constexpr int level_count = levels_per_world * world_count;
static_assert(level_count < 32, "Increase the save format capacity before adding more worlds");
struct WorldSpec { const char* name; int first_level; int count; };
inline constexpr std::array<WorldSpec, world_count> worlds{{{"Backyard", 0, 10}}};
struct Progress
{
    uint32_t completed = 0;
    int unlocked = 1;
    int selected = 0;
    std::array<uint32_t, zombie_kind_count> zombie_kills{};
    bool valid() const
    { return unlocked >= 1 && unlocked <= level_count && selected >= 0 && selected < unlocked &&
             (completed >> level_count) == 0; }
    static Progress all_unlocked() { return {0, level_count, 0}; }
    bool operator==(const Progress& other) const
    { return completed == other.completed && unlocked == other.unlocked && selected == other.selected && zombie_kills == other.zombie_kills; }
};
namespace save_format
{
inline constexpr uint32_t magic = 0x325A5650; // PVZ2, little endian.
inline constexpr uint32_t legacy_magic = 0x31535653; // SVS1.
inline constexpr int version = 2;
inline constexpr int legacy_version = 1;
inline constexpr int record_size = 48;
inline constexpr int legacy_record_size = 24;
inline constexpr int completed_offset = 8;
inline constexpr int sequence_offset = 12;
inline constexpr int kills_offset = 20;
inline constexpr int checksum_offset = record_size - 4;
inline constexpr int legacy_checksum_offset = legacy_record_size - 4;
static_assert(kills_offset + zombie_kind_count * 4 == checksum_offset);
}
using SaveRecord = std::array<uint8_t, save_format::record_size>;
using LegacySaveRecord = std::array<uint8_t, save_format::legacy_record_size>;
template<std::size_t Size> inline uint32_t save_word(const std::array<uint8_t, Size>& data, int offset)
{
    return uint32_t(data[offset]) | uint32_t(data[offset + 1]) << 8 |
           uint32_t(data[offset + 2]) << 16 | uint32_t(data[offset + 3]) << 24;
}
template<std::size_t Size> inline void put_save_word(std::array<uint8_t, Size>& data, int offset, uint32_t value)
{ for(int i = 0; i < 4; ++i) { data[offset + i] = uint8_t(value >> (i * 8)); } }
template<std::size_t Size> inline uint32_t save_checksum(const std::array<uint8_t, Size>& data)
{
    uint32_t hash = 2166136261u;
    for(int i = 0; i < int(Size) - 4; ++i) { hash = (hash ^ data[i]) * 16777619u; }
    return hash;
}
inline SaveRecord encode_progress(const Progress& progress, uint32_t sequence)
{
    SaveRecord data{};
    put_save_word(data, 0, save_format::magic);
    data[4] = save_format::version;
    data[5] = uint8_t(progress.unlocked);
    data[6] = uint8_t(progress.selected);
    put_save_word(data, save_format::completed_offset, progress.completed);
    put_save_word(data, save_format::sequence_offset, sequence);
    for(int i = 0; i < zombie_kind_count; ++i) { put_save_word(data, save_format::kills_offset + i * 4, progress.zombie_kills[i]); }
    put_save_word(data, save_format::checksum_offset, save_checksum(data));
    return data;
}
inline bool decode_progress(const SaveRecord& data, Progress& progress)
{
    Progress candidate{save_word(data, save_format::completed_offset), data[5], data[6]};
    if(save_word(data, 0) != save_format::magic || data[4] != save_format::version ||
       save_word(data, save_format::checksum_offset) != save_checksum(data) || ! candidate.valid()) { return false; }
    for(int i = 0; i < zombie_kind_count; ++i) { candidate.zombie_kills[i] = save_word(data, save_format::kills_offset + i * 4); }
    progress = candidate;
    return true;
}
inline bool decode_legacy_progress(const LegacySaveRecord& data, Progress& progress)
{
    Progress candidate{save_word(data, save_format::completed_offset), data[5], data[6]};
    if(save_word(data, 0) != save_format::legacy_magic || data[4] != save_format::legacy_version ||
       save_word(data, save_format::legacy_checksum_offset) != save_checksum(data) || ! candidate.valid()) { return false; }
    progress = candidate;
    return true;
}
}
#endif
