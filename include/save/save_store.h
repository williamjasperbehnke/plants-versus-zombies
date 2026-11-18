#ifndef GARDEN_SAVE_STORE_H
#define GARDEN_SAVE_STORE_H
#include "bn_sram.h"
#include "save/save_slots.h"

namespace garden {
class SaveStore {
public:
    void load(SaveSlots& slots) {
        for (int index = 0; index < save_slot_count; ++index) {
            std::array<SaveRecord, 2> records;
            for (int copy = 0; copy < 2; ++copy) {
                bn::sram::read_offset(records[copy], offset(index) + copy * int(sizeof(SaveRecord)));
            }
            slots.restore(index, records);
        }
    }

    void update(SaveSlots& slots) {
        for (int index = 0; index < save_slot_count; ++index) {
            if (!slots.dirty(index)) {
                continue;
            }
            auto record = slots.pending_record(index);
            bn::sram::write_offset(record, offset(index) + slots.next_copy(index) * int(sizeof(SaveRecord)));
            slots.written(index);
        }
    }

private:
    static constexpr int offset(int index) {
#ifdef GARDEN_DEBUG_SAVE
        return PVZ_SAVE_DEBUG_OFFSET + index * PVZ_SAVE_SLOT_STRIDE;
#else
        return PVZ_SAVE_NORMAL_OFFSET + index * PVZ_SAVE_SLOT_STRIDE;
#endif
    }
};
} // namespace garden
#endif
