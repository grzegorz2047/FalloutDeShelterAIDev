#include "dwellers/Dweller.hpp"

#include <cassert>

using namespace deep_shelter::dwellers;

int main() {
    XpTable table{{0, 100, 300, 600, 1000}, 5};
    assert(table.valid());
    DwellerService service(table);

    Dweller dweller;
    dweller.id = 1;
    dweller.name = "Mara|Vale";
    dweller.appearance_id = "appearance.scout";
    dweller.presentation = "female";
    dweller.origin = "surface";
    dweller.base_special.values = {{12, -2, 5, 6, 7, 8, 9}};
    dweller.outfit_bonus.values = {{2, 1, 0, 0, 0, 0, 0}};
    dweller.max_hp = 100;
    dweller.hp = 150;
    dweller.radiation = 20;
    dweller.happiness = 120;
    dweller.weapon_id = "weapon.removed";
    dweller.outfit_id = "outfit.removed";
    dweller.companion_id = "companion.removed";
    dweller.children = {7, 9};
    dweller.history.push_back({44, "recruited", "gate,alpha"});
    assert(service.add(dweller));
    assert(!service.add(dweller));
    assert(service.valid_unique_ids());

    auto* stored = service.find(1);
    assert(stored != nullptr);
    assert(stored->base_special.values[0] == 10);
    assert(stored->base_special.values[1] == 1);
    assert(stored->effective_special().values[0] == 10);
    assert(stored->effective_max_hp() == 80);
    assert(stored->hp == 80);
    assert(stored->happiness == 100);

    const auto base_before = stored->base_special.values;
    service.resolve_equipment({}, {}, {});
    assert(stored->weapon_id == "weapon.unknown");
    assert(stored->outfit_id == "outfit.unknown");
    assert(stored->companion_id == "companion.unknown");
    assert(stored->base_special.values == base_before);
    assert(stored->effective_special().values == base_before);

    assert(service.grant_xp(1, 650, 1001, 5000));
    assert(!service.grant_xp(1, 650, 1001, 5000));
    assert(stored->level == 4);
    assert(stored->max_hp == 115);
    assert(stored->history.size() == 4);
    assert(stored->awarded_levels.size() == 3);

    assert(service.grant_xp(1, 10000, 1002, 6000));
    assert(stored->level == 5);
    assert(!service.grant_xp(1, 10, 1002, 7000));

    const auto payload = serialize_dweller(*stored);
    const auto restored = deserialize_dweller(payload);
    assert(restored.has_value());
    assert(restored->id == stored->id);
    assert(restored->name == stored->name);
    assert(restored->base_special.values == stored->base_special.values);
    assert(restored->outfit_bonus.values == stored->outfit_bonus.values);
    assert(restored->level == stored->level);
    assert(restored->xp == stored->xp);
    assert(restored->max_hp == stored->max_hp);
    assert(restored->hp == stored->hp);
    assert(restored->radiation == stored->radiation);
    assert(restored->happiness == stored->happiness);
    assert(restored->children == stored->children);
    assert(restored->awarded_levels == stored->awarded_levels);
    assert(restored->history.size() == stored->history.size());
    assert(restored->history.front().detail == stored->history.front().detail);
    assert(!deserialize_dweller("2|unsupported").has_value());

    Dweller duplicate;
    duplicate.id = 1;
    duplicate.name = "Duplicate";
    assert(service.add_with_unique_id(duplicate) == 2);
    Dweller missing_id;
    missing_id.name = "Recruit";
    assert(service.add_with_unique_id(missing_id) == 3);
    assert(service.valid_unique_ids());

    stored->radiation = 500;
    stored->hp = 0;
    stored->normalize(table.max_level);
    assert(stored->radiation == stored->max_hp);
    assert(stored->effective_max_hp() == 1);
    assert(!stored->alive());
    assert(stored->status == ActivityStatus::Dead);

    XpTable invalid{{10, 5}, 2};
    assert(!invalid.valid());
    DwellerService invalid_service(invalid);
    Dweller other;
    other.id = 2;
    other.name = "Invalid";
    assert(!invalid_service.add(other));
    return 0;
}
