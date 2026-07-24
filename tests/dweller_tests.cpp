#include "dwellers/Dweller.hpp"

#include <cassert>
#include <sstream>

using namespace deep_shelter::dwellers;

namespace {
void token(std::ostringstream& stream, const std::string& value) {
    stream << value.size() << ':' << value;
}

template <typename T>
void number(std::ostringstream& stream, T value) {
    token(stream, std::to_string(value));
}

std::string legacy_v0_payload() {
    std::ostringstream stream;
    number(stream, 0);
    number(stream, 77);
    token(stream, "Legacy,Resident");
    token(stream, "4,5,6,7,8,9,10");
    number(stream, 4);
    number(stream, 650);
    number(stream, 115);
    number(stream, 90);
    number(stream, 5);
    number(stream, 80);
    number(stream, static_cast<int>(ActivityStatus::Working));
    number(stream, 12);
    token(stream, "weapon.legacy");
    token(stream, "outfit.legacy");
    token(stream, "companion.legacy");
    number(stream, 1);
    number(stream, 2);
    token(stream, "8,9");
    return stream.str();
}
}  // namespace

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
    dweller.history.push_back({44, "recruited", "gate,alpha|path\\beta"});
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
    assert(!deserialize_dweller("1:2unsupported").has_value());

    const auto migrated = deserialize_dweller(legacy_v0_payload());
    assert(migrated.has_value());
    assert(migrated->id == 77);
    assert(migrated->name == "Legacy,Resident");
    assert(migrated->level == 4);
    assert(migrated->awarded_levels.size() == 3);
    assert(migrated->outfit_bonus.values[0] == 0);
    assert(migrated->children.size() == 2);
    assert(migrated->history.back().type == "schema_migrated");
    assert(migrated->history.back().detail == "v0_to_v1");

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
