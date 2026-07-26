#include "rooms/RoomCatalog.hpp"

#include <algorithm>
#include <cassert>
#include <string>

using namespace deep_shelter::rooms;

int main() {
    RoomCatalog catalog(built_in_room_definitions());
    std::string detail;
    assert(catalog.valid(&detail));
    assert(detail == "ok");
    assert(catalog.size() >= 20);

    const auto* generator = catalog.find("room.power_generator");
    assert(generator != nullptr);
    assert(generator->stable_key == room_stable_key("room.power_generator"));
    assert(catalog.find(generator->stable_key) == generator);
    assert(generator->display_name == "POWER GENERATOR");

    const auto* elevator = catalog.find("room.elevator");
    assert(elevator != nullptr);
    assert(elevator->transport);
    assert(elevator->category == "transport");
    assert(elevator->base_cost == 50);

    UnlockContext start;
    start.population = 0;
    start.progress = 0;
    assert(catalog.availability("room.power_generator", start).unlocked);
    assert(catalog.availability("room.living_quarters", start).unlocked);
    assert(catalog.availability("room.utility_tunnel", start).unlocked);
    assert(catalog.availability("room.elevator", start).unlocked);

    const auto water_locked = catalog.availability("room.water_purifier", start);
    assert(!water_locked.unlocked);
    assert(water_locked.reason.find("population 4") != std::string::npos);
    assert(water_locked.next_step.find("4 more") != std::string::npos);
    assert(catalog.availability(room_stable_key("room.water_purifier"), start).reason ==
           water_locked.reason);

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
    assert(catalog.resolve_or_placeholder(0xdeadbeefu).id == "room.unknown");

    const auto all = catalog.list(start);
    assert(all.size() == catalog.size());
    std::size_t unlocked = 0;
    for (const auto& item : all) {
        assert(item.definition != nullptr);
        assert(!item.definition->display_name.empty());
        if (item.unlocked) ++unlocked;
        else assert(!item.reason.empty() && !item.next_step.empty());
    }
    assert(unlocked == 4);

    // Stable identity must survive catalog reordering.
    auto reordered = built_in_room_definitions();
    std::reverse(reordered.begin(), reordered.end());
    RoomCatalog reordered_catalog(std::move(reordered));
    assert(reordered_catalog.valid(&detail));
    assert(reordered_catalog.find(generator->stable_key) != nullptr);
    assert(reordered_catalog.find(generator->stable_key)->id == generator->id);

    auto invalid = built_in_room_definitions();
    invalid[0].base_cost = 0;
    RoomCatalog bad_cost(std::move(invalid));
    assert(!bad_cost.valid(&detail));

    invalid = built_in_room_definitions();
    invalid[1].id = invalid[0].id;
    invalid[1].stable_key = invalid[0].stable_key;
    RoomCatalog duplicate(std::move(invalid));
    assert(!duplicate.valid(&detail));

    invalid = built_in_room_definitions();
    invalid[1].stable_key = invalid[0].stable_key;
    RoomCatalog duplicate_key(std::move(invalid));
    assert(!duplicate_key.valid(&detail));

    invalid = built_in_room_definitions();
    invalid[0].display_name.clear();
    RoomCatalog missing_name(std::move(invalid));
    assert(!missing_name.valid(&detail));

    invalid = built_in_room_definitions();
    invalid[0].transport = true;
    RoomCatalog inconsistent_transport(std::move(invalid));
    assert(!inconsistent_transport.valid(&detail));
    return 0;
}
