#include "dwellers/Dweller.hpp"

#include <cassert>

using namespace deep_shelter::dwellers;

int main() {
    XpTable table{{0, 100, 300, 600, 1000}, 5};
    assert(table.valid());
    DwellerService service(table);

    Dweller dweller;
    dweller.id = 1;
    dweller.name = "Mara";
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
    assert(service.add(dweller));
    assert(!service.add(dweller));
    assert(service.valid_unique_ids());

    auto* stored = service.find(1);
    assert(stored != nullptr);
    assert(stored->base_special.values[0] == 10);
    assert(stored->base_special.values[1] == 0);
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
    assert(stored->history.size() == 3);
    assert(stored->awarded_levels.size() == 3);

    assert(service.grant_xp(1, 10000, 1002, 6000));
    assert(stored->level == 5);
    assert(!service.grant_xp(1, 10, 1002, 7000));

    stored->radiation = 500;
    stored->hp = 0;
    stored->normalize();
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
