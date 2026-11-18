#ifndef GARDEN_GAME_INTERNAL_H
#define GARDEN_GAME_INTERNAL_H

namespace garden::detail {
template <class Pool>
auto* free_slot(Pool& pool) {
    for (auto& slot : pool) {
        if (!slot.active) {
            return &slot;
        }
    }
    return static_cast<typename Pool::value_type*>(nullptr);
}

constexpr int absolute(int value) {
    return value < 0 ? -value : value;
}
} // namespace garden::detail
#endif
