#ifndef GARDEN_SAVE_SLOTS_H
#define GARDEN_SAVE_SLOTS_H
#include "progress.h"
namespace garden
{
inline constexpr int save_slot_count = PVZ_SAVE_SLOT_COUNT;
static_assert(PVZ_SAVE_DEBUG_OFFSET - PVZ_SAVE_NORMAL_OFFSET >= 2 * PVZ_SAVE_SIZE);
static_assert(PVZ_SAVE_SLOT_STRIDE > PVZ_SAVE_DEBUG_OFFSET - PVZ_SAVE_NORMAL_OFFSET + 2 * PVZ_SAVE_SIZE);
static_assert(PVZ_SAVE_DEBUG_OFFSET + (save_slot_count - 1) * PVZ_SAVE_SLOT_STRIDE + 2 * PVZ_SAVE_SIZE <= 32768);
// Portable journal state; SRAM I/O belongs to SaveStore.
class SaveSlots
{
public:
    explicit SaveSlots(bool debug = false) : _debug(debug) {}
    void restore(int index, const std::array<SaveRecord, 2>& records)
    {
        auto& slot = _slots[index];
        slot = {};
        Progress values[2];
        bool valid[2] = {decode_progress(records[0], values[0]), decode_progress(records[1], values[1])};
        if(! valid[0] && ! valid[1]) { return; }
        uint32_t distance = save_word(records[1], save_format::sequence_offset) - save_word(records[0], save_format::sequence_offset);
        slot.copy = valid[1] && (! valid[0] || distance < 0x80000000u) ? 1 : 0;
        slot.sequence = save_word(records[slot.copy], save_format::sequence_offset);
        slot.progress = values[slot.copy];
        slot.occupied = records[slot.copy][save_format::state_offset] == 0;
        if(_debug && slot.occupied && slot.progress.unlocked != level_count)
        { slot.progress.unlocked = level_count; slot.dirty = true; }
    }
    bool occupied(int index) const { return _slots[index].occupied; }
    const Progress& progress(int index) const { return _slots[index].progress; }
    int active() const { return _active; }
    const Progress& activate(int index)
    {
        auto& slot = _slots[index];
        if(! slot.occupied)
        { slot.progress = _debug ? Progress::all_unlocked() : Progress(); slot.occupied = true; slot.dirty = true; }
        _active = index;
        return slot.progress;
    }
    void update_active(const Progress& progress)
    {
        if(_active < 0 || ! progress.valid()) { return; }
        auto& slot = _slots[_active];
        if(! (slot.progress == progress)) { slot.progress = progress; slot.dirty = true; }
    }
    void erase(int index)
    {
        auto& slot = _slots[index];
        if(! slot.occupied) { return; }
        slot.progress = {}; slot.occupied = false; slot.dirty = true;
        if(_active == index) { _active = -1; }
    }
    bool dirty(int index) const { return _slots[index].dirty; }
    int next_copy(int index) const { return 1 - _slots[index].copy; }
    SaveRecord pending_record(int index) const
    { const auto& slot = _slots[index]; return encode_progress(slot.progress, slot.sequence + 1, slot.occupied); }
    void written(int index)
    { auto& slot = _slots[index]; slot.copy = 1 - slot.copy; ++slot.sequence; slot.dirty = false; }
private:
    struct Slot
    {
        Progress progress;
        uint32_t sequence = 0;
        int copy = 0;
        bool occupied = false;
        bool dirty = false;
    };
    std::array<Slot, save_slot_count> _slots{};
    int _active = -1;
    bool _debug;
};
}
#endif
