#include "rooms/RoomCatalog.hpp"

#include <cassert>
#include <string>

using namespace deep_shelter::rooms;

int main() {
    RoomCatalog catalog(built_in_room_definitions());
    std::string detail;
    assert(catalog.valid(&detail));
    assert(detail == "ok");
    assert(catalog.size() >= 20);

    UnlockContext start;
    start.population = 0;
    start.progress = 0;
    assert(catalog.availability("room.power_generator", start).unlocked);
    assert(catalog.availability("room.living_quarters", start).unlocked);

    const auto water_locked = catalog.availability("room.water_purifier", start);
    assert(!water_locked.unlocked);
    assert(water_locked.reason.find("population 4") != std::string::npos);
    assert(water_locked.next_step.find("4 more") != std::string::npos);

    UnlockContext exact;
    exact.population = 4;
    exact.progress = 0;
    assert(catalog.availability("room.water_purifier", exact).unlocked);

    UnlockContext late;
    late.population = 50;
    late.progress = 5;
    auto command = catalog.availability("room.command_center", late);
    assert(!command.unlocked);
    assert(command.reason.find("achievement.first_expedition") != std::string::npos);
    late.achievements.insert("achievement.first_expedition");
    assert(catalog.availability("room.command_center", late).unlocked);

    // A later population drop affects only future availability. Existing room IDs remain resolvable.
    late.population = 1;
    assert(!catalog.availability("room.command_center", late).unlocked);
    assert(catalog.resolve_or_placeholder("room.command_center").id == "room.command_center");

    const auto unknown = catalog.availability("room.retired_definition", late);
    assert(!unknown.unlocked);
    assert(unknown.definition != nullptr);
    assert(unknown.definition->id == "room.unknown");
    assert(catalog.resolve_or_placeholder("room.retired_definition").id == "room.unknown");

    const auto all = catalog.list(start);
    assert(all.size() == catalog.size());
    std::size_t unlocked = 0;
    for (const auto& item : all) {
        assert(item.definition != nullptr);
        if (item.unlocked) ++unlocked;
        else assert(!item.reason.empty() && !item.next_step.empty());
    }
    assert(unlocked == 2);

    auto invalid = built_in_room_definitions();
    invalid[0].base_cost = 0;
    RoomCatalog bad_cost(std::move(invalid));
    assert(!bad_cost.valid(&detail));

    invalid = built_in_room_definitions();
    invalid[1].id = invalid[0].id;
    RoomCatalog duplicate(std::move(invalid));
    assert(!duplicate.valid(&detail));
    return 0;
}
