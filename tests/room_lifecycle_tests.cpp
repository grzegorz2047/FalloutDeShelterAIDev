#include "rooms/RoomLifecycle.hpp"

#include <cassert>

using namespace deep_shelter::rooms;

int main() {
    RoomLifecycle lifecycle(1000);

    assert(lifecycle.add_segment({3, 0, "room.kitchen", 1, 2, 0, 1, 10, 2, false}));
    assert(lifecycle.add_segment({1, 0, "room.kitchen", 1, 0, 0, 1, 5, 1, false}));
    assert(lifecycle.add_segment({2, 0, "room.kitchen", 1, 1, 0, 1, 7, 3, false}));
    assert(lifecycle.segments().size() == 3);

    // Merge is independent from construction order and uses the minimum stable segment ID.
    for (const auto& segment : lifecycle.normalized_segments()) {
        assert(segment.group_id == 1);
    }

    const auto upgrade = lifecycle.preview_upgrade(1, 300);
    assert(upgrade.allowed);
    assert(upgrade.credit_delta == -300);
    assert(upgrade.residents_to_evacuate == 3);
    assert(upgrade.in_progress_units_to_preserve == 6);
    assert(lifecycle.credits() == 1000);
    assert(lifecycle.confirm_upgrade(1, 300));
    assert(lifecycle.credits() == 700);
    assert(!lifecycle.confirm_upgrade(1, 300)); // group ID changes after level normalization

    std::uint64_t upgraded_group = lifecycle.segments().front().group_id;
    assert(lifecycle.confirm_upgrade(upgraded_group, 200));
    assert(lifecycle.credits() == 500);
    assert(!lifecycle.preview_upgrade(lifecycle.segments().front().group_id, 100).allowed);

    const auto demolition = lifecycle.preview_demolish(2, 40);
    assert(demolition.allowed);
    assert(demolition.residents_to_evacuate == 1);
    assert(demolition.stored_units_to_relocate == 7);
    assert(demolition.in_progress_units_to_preserve == 3);
    assert(!lifecycle.confirm_demolish(2, 40, 10));
    assert(lifecycle.credits() == 500);
    assert(lifecycle.confirm_demolish(2, 40, 11));
    assert(lifecycle.credits() == 540);
    assert(!lifecycle.confirm_demolish(2, 40, 100));
    assert(lifecycle.segments().size() == 2);

    // Removing the middle segment deterministically splits the former group.
    assert(lifecycle.segments()[0].group_id != lifecycle.segments()[1].group_id);

    RoomLifecycle blocked(100);
    assert(blocked.add_segment({10, 0, "room.medbay", 1, 0, 0, 2, 0, 4, true}));
    assert(!blocked.preview_upgrade(10, 50).allowed);
    assert(!blocked.preview_demolish(10, 10).allowed);
    assert(blocked.credits() == 100);

    RoomLifecycle poor(20);
    assert(poor.add_segment({20, 0, "room.power_generator", 1, 0, 0, 0, 0, 0, false}));
    assert(!poor.confirm_upgrade(20, 50));
    assert(poor.credits() == 20);
    return 0;
}
