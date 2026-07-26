#include "gameplay/PlayableRoomCatalog.hpp"

#include <cassert>
#include <cstdint>
#include <string>

using namespace deep_shelter::gameplay;
using deep_shelter::rooms::UnlockContext;
using deep_shelter::rooms::room_stable_key;

int main() {
    PlayableRoomCatalog catalog;
    assert(catalog.valid());

    UnlockContext start;
    start.population = 0;
    start.progress = 0;
    const auto entries = catalog.list(start);
    assert(entries.size() >= 20);

    const auto power = catalog.find(room_stable_key("room.power_generator"), start);
    assert(power.definition != nullptr);
    assert(power.definition->display_name == "POWER GENERATOR");
    assert(power.behavior == PlayableRoomType::Power);
    assert(power.unlocked);

    const auto water = catalog.find(room_stable_key("room.water_purifier"), start);
    assert(water.definition != nullptr);
    assert(water.behavior == PlayableRoomType::Water);
    assert(!water.unlocked);
    assert(water.reason.find("population 4") != std::string::npos);
    assert(water.next_step.find("4 more") != std::string::npos);

    const auto elevator = catalog.find(room_stable_key("room.elevator"), start);
    assert(elevator.definition != nullptr);
    assert(elevator.behavior == PlayableRoomType::Elevator);
    assert(elevator.definition->transport);
    assert(elevator.unlocked);

    const auto living = catalog.find(room_stable_key("room.living_quarters"), start);
    assert(living.definition != nullptr);
    assert(living.behavior == PlayableRoomType::Living);

    const auto weapon = catalog.find(room_stable_key("room.workshop"), start);
    const auto outfit = catalog.find(room_stable_key("room.outfit_bench"), start);
    assert(weapon.definition != nullptr && outfit.definition != nullptr);
    assert(weapon.behavior == PlayableRoomType::Workshop);
    assert(outfit.behavior == PlayableRoomType::Workshop);
    assert(weapon.definition->stable_key != outfit.definition->stable_key);

    UnlockContext exact;
    exact.population = 4;
    exact.progress = 0;
    assert(catalog.find(room_stable_key("room.water_purifier"), exact).unlocked);

    const std::uint32_t retired = 0xdeadbeefu;
    const auto unknown = catalog.find(retired, start);
    assert(unknown.definition != nullptr);
    assert(unknown.definition->id == "room.unknown");
    assert(!unknown.unlocked);
    assert(!unknown.reason.empty());
    assert(catalog.resolve(retired).id == "room.unknown");
    return 0;
}
