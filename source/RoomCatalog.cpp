#include "rooms/RoomCatalog.hpp"

#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace deep_shelter::rooms {
namespace {

RoomDefinition room(const char* id,
                    const char* category,
                    const char* display_name,
                    const char* special,
                    int cost,
                    int width,
                    int population,
                    int progress,
                    const char* achievement,
                    const char* produces,
                    int storage,
                    bool transport = false) {
    RoomDefinition value;
    value.id = id;
    value.stable_key = room_stable_key(value.id);
    value.category = category;
    value.display_name = display_name;
    value.name_key = value.id + ".name";
    value.description_key = value.id + ".description";
    value.icon = "romfs:/icons/" + value.id.substr(5) + ".t3x";
    value.special = special[0];
    value.base_cost = cost;
    value.width = width;
    value.max_level = 3;
    value.unlock_population = population;
    value.unlock_progress = progress;
    value.required_achievement = achievement;
    value.produces = produces;
    value.storage_bonus = storage;
    value.transport = transport;
    return value;
}

}  // namespace

RoomCatalog::RoomCatalog(std::vector<RoomDefinition> definitions)
    : definitions_(std::move(definitions)) {
    placeholder_.id = "room.unknown";
    placeholder_.stable_key = room_stable_key(placeholder_.id);
    placeholder_.category = "special";
    placeholder_.display_name = "UNKNOWN ROOM";
    placeholder_.name_key = "room.unknown.name";
    placeholder_.description_key = "room.unknown.description";
    placeholder_.icon = "romfs:/icons/room_unknown.t3x";
    placeholder_.base_cost = 1;
}

bool RoomCatalog::valid(std::string* detail) const {
    std::unordered_set<std::string> ids;
    std::unordered_set<std::uint32_t> stable_keys;
    static const std::unordered_set<std::string> categories = {
        "production", "storage", "residential", "training", "medical",
        "recruitment", "crafting", "cosmetic", "special", "transport"};
    static const std::string special = "SPECIAL";

    for (const auto& definition : definitions_) {
        if (definition.id.rfind("room.", 0) != 0 ||
            !ids.insert(definition.id).second) {
            if (detail) *detail = "room id is invalid or duplicated: " + definition.id;
            return false;
        }
        if (definition.stable_key == 0 ||
            definition.stable_key != room_stable_key(definition.id) ||
            !stable_keys.insert(definition.stable_key).second) {
            if (detail) *detail = "room stable key is invalid or duplicated: " + definition.id;
            return false;
        }
        if (categories.count(definition.category) == 0) {
            if (detail) *detail = "unknown room category: " + definition.category;
            return false;
        }
        if (definition.display_name.empty() || definition.name_key.empty() ||
            definition.description_key.empty() || definition.icon.empty()) {
            if (detail) *detail = "room presentation metadata is incomplete: " + definition.id;
            return false;
        }
        if (special.find(definition.special) == std::string::npos ||
            definition.base_cost <= 0 || definition.width < 1 ||
            definition.width > 3 || definition.max_level < 1 ||
            definition.unlock_population < 0 || definition.unlock_progress < 0 ||
            definition.storage_bonus < 0) {
            if (detail) *detail = "room numeric definition is invalid: " + definition.id;
            return false;
        }
        if (definition.transport != (definition.category == "transport")) {
            if (detail) *detail = "room transport metadata is inconsistent: " + definition.id;
            return false;
        }
    }
    if (detail) *detail = "ok";
    return definitions_.size() >= 20;
}

std::size_t RoomCatalog::size() const noexcept { return definitions_.size(); }

const RoomDefinition* RoomCatalog::find(const std::string& id) const noexcept {
    const auto it = std::find_if(definitions_.begin(), definitions_.end(),
                                 [&](const RoomDefinition& value) {
                                     return value.id == id;
                                 });
    return it == definitions_.end() ? nullptr : &*it;
}

const RoomDefinition* RoomCatalog::find(std::uint32_t stable_key) const noexcept {
    const auto it = std::find_if(definitions_.begin(), definitions_.end(),
                                 [&](const RoomDefinition& value) {
                                     return value.stable_key == stable_key;
                                 });
    return it == definitions_.end() ? nullptr : &*it;
}

RoomAvailability RoomCatalog::availability(
    const RoomDefinition* definition,
    const UnlockContext& context) const {
    if (definition == nullptr) {
        return {&placeholder_, false, "Unknown room type from an older save.",
                "Keep the room as a safe placeholder and update the game data."};
    }
    if (context.population < definition->unlock_population) {
        std::ostringstream reason;
        reason << "Requires population " << definition->unlock_population << ".";
        std::ostringstream next;
        next << "Recruit " << (definition->unlock_population - context.population)
             << " more resident(s).";
        return {definition, false, reason.str(), next.str()};
    }
    if (context.progress < definition->unlock_progress) {
        std::ostringstream reason;
        reason << "Requires shelter progress " << definition->unlock_progress << ".";
        return {definition, false, reason.str(),
                "Complete objectives to advance shelter progress."};
    }
    if (!definition->required_achievement.empty() &&
        context.achievements.count(definition->required_achievement) == 0) {
        return {definition, false,
                "Requires achievement " + definition->required_achievement + ".",
                "Complete the linked achievement."};
    }
    return {definition, true, {}, {}};
}

RoomAvailability RoomCatalog::availability(const std::string& id,
                                            const UnlockContext& context) const {
    return availability(find(id), context);
}

RoomAvailability RoomCatalog::availability(std::uint32_t stable_key,
                                            const UnlockContext& context) const {
    return availability(find(stable_key), context);
}

std::vector<RoomAvailability> RoomCatalog::list(const UnlockContext& context) const {
    std::vector<RoomAvailability> values;
    values.reserve(definitions_.size());
    for (const auto& definition : definitions_) {
        values.push_back(availability(&definition, context));
    }
    return values;
}

const RoomDefinition& RoomCatalog::resolve_or_placeholder(
    const std::string& id) const noexcept {
    const RoomDefinition* definition = find(id);
    return definition == nullptr ? placeholder_ : *definition;
}

const RoomDefinition& RoomCatalog::resolve_or_placeholder(
    std::uint32_t stable_key) const noexcept {
    const RoomDefinition* definition = find(stable_key);
    return definition == nullptr ? placeholder_ : *definition;
}

std::vector<RoomDefinition> built_in_room_definitions() {
    return {
        room("room.power_generator", "production", "POWER GENERATOR", "S", 100, 1, 0, 0, "", "resource.power", 0),
        room("room.water_purifier", "production", "WATER PURIFIER", "P", 120, 1, 4, 0, "", "resource.water", 0),
        room("room.kitchen", "production", "DINER", "A", 120, 1, 4, 0, "", "resource.food", 0),
        room("room.reactor", "production", "NUCLEAR REACTOR", "I", 1600, 2, 40, 4, "", "resource.power", 0),
        room("room.hydroponics", "production", "GARDEN", "A", 1450, 2, 36, 4, "", "resource.food", 0),
        room("room.filtration", "production", "WATER TREATMENT", "P", 1450, 2, 36, 4, "", "resource.water", 0),
        room("room.power_storage", "storage", "POWER STORAGE", "E", 300, 1, 12, 1, "", "", 250),
        room("room.food_storage", "storage", "FOOD STORAGE", "E", 300, 1, 12, 1, "", "", 250),
        room("room.water_storage", "storage", "WATER STORAGE", "E", 300, 1, 12, 1, "", "", 250),
        room("room.living_quarters", "residential", "LIVING QUARTERS", "C", 150, 1, 0, 0, "", "", 2),
        room("room.strength_gym", "training", "WEIGHT ROOM", "S", 600, 1, 20, 2, "", "", 0),
        room("room.perception_lab", "training", "ARMORY", "P", 650, 1, 22, 2, "", "", 0),
        room("room.agility_course", "training", "ATHLETICS ROOM", "A", 700, 1, 24, 2, "", "", 0),
        room("room.medbay", "medical", "MEDBAY", "I", 500, 1, 16, 1, "", "resource.medkits", 20),
        room("room.decontamination", "medical", "SCIENCE LAB", "I", 800, 1, 28, 3, "", "resource.antirad", 20),
        room("room.radio_station", "recruitment", "RADIO STUDIO", "C", 900, 2, 26, 2, "", "", 0),
        room("room.workshop", "crafting", "WEAPON WORKSHOP", "I", 1100, 2, 30, 3, "", "", 0),
        room("room.outfit_bench", "crafting", "OUTFIT WORKSHOP", "I", 1200, 2, 32, 3, "", "", 0),
        room("room.community_hall", "cosmetic", "BARBERSHOP", "C", 750, 2, 18, 2, "", "", 0),
        room("room.command_center", "special", "OVERSEER OFFICE", "L", 2500, 3, 50, 5, "achievement.first_expedition", "", 0),
        room("room.utility_tunnel", "special", "UTILITY TUNNEL", "E", 40, 1, 0, 0, "", "", 0),
        room("room.elevator", "transport", "ELEVATOR", "E", 50, 1, 0, 0, "", "", 0, true),
    };
}

}  // namespace deep_shelter::rooms
