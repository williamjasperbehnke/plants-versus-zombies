#include "save/save_slots.h"

#include <cassert>
#include <cstdio>
using namespace garden;
using Journal = std::array<SaveRecord, 2>;

void flush(SaveSlots& bank, int slot, Journal& journal) {
    assert(bank.dirty(slot));
    journal[bank.next_copy(slot)] = bank.pending_record(slot);
    bank.written(slot);
}

int main() {
    SaveSlots bank;
    std::array<Journal, save_slot_count> disk{};
    for (int i = 0; i < save_slot_count; ++i) {
        bank.restore(i, disk[i]);
        assert(!bank.occupied(i) && !bank.dirty(i));
    }
    auto first = bank.activate(0);
    first.unlocked = 4;
    first.selected = 3;
    first.completed = 7;
    first.zombie_kills[5] = 42;
    bank.update_active(first);
    flush(bank, 0, disk[0]);
    auto second = bank.activate(1);
    second.unlocked = 2;
    second.completed = 1;
    bank.update_active(second);
    flush(bank, 1, disk[1]);
    bank.activate(2);
    flush(bank, 2, disk[2]);
    SaveSlots reboot;
    for (int i = 0; i < save_slot_count; ++i) {
        reboot.restore(i, disk[i]);
    }
    assert(reboot.active() == -1 && reboot.progress(0) == first && reboot.progress(1) == second);
    assert(reboot.activate(0) == first && !reboot.dirty(0));
    reboot.erase(0);
    flush(reboot, 0, disk[0]);
    assert(reboot.active() == -1 && !reboot.occupied(0));
    SaveSlots deleted;
    for (int i = 0; i < save_slot_count; ++i) {
        deleted.restore(i, disk[i]);
    }
    assert(!deleted.occupied(0) && deleted.occupied(1) && deleted.occupied(2));
    assert(deleted.progress(1) == second); // Other profiles are untouched.
    assert(deleted.activate(0) == Progress());
    flush(deleted, 0, disk[0]);
    reboot.restore(0, disk[0]);
    assert(reboot.occupied(0) && reboot.progress(0) == Progress());
    // Newest valid copy wins across sequence wrap; corruption falls back to its peer.
    Journal wrapping{{encode_progress(first, UINT32_MAX), encode_progress(second, 0)}};
    reboot.restore(0, wrapping);
    assert(reboot.progress(0) == second);
    wrapping[1][48] ^= 1;
    reboot.restore(0, wrapping);
    assert(reboot.progress(0) == first);
    auto bad_state = encode_progress(first, 5);
    bad_state[7] = 2;
    put_save_word(bad_state, save_format::checksum_offset, save_checksum(bad_state));
    Progress rejected;
    assert(!decode_progress(bad_state, rejected));
    SaveSlots debug(true);
    for (int i = 0; i < save_slot_count; ++i) {
        assert(debug.activate(i).unlocked == level_count);
    }
    debug.restore(0, Journal{{encode_progress(first, 9), {}}});
    assert(debug.progress(0).unlocked == level_count && debug.progress(0).zombie_kills[5] == 42);
    debug.erase(0);
    Journal empty{};
    flush(debug, 0, empty);
    SaveSlots debug_reload(true);
    debug_reload.restore(0, empty);
    assert(!debug_reload.occupied(0));
    std::puts("PASS three independent save slots, continue, durable deletion/recreation, backup recovery, "
              "sequence wrap and debug defaults");
}
