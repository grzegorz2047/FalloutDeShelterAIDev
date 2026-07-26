#include "gameplay/PlayableRoomCatalog.hpp"

#include <utility>

namespace deep_shelter::gameplay {

namespace {

PlayableRoomCatalogEntry make_entry(
    const rooms::RoomAvailability& availability) {
    PlayableRoomCatalogEntry entry;
    entry.definition = availability.definition;
    entry.behavior = availability.definition == nullptr
                         ? PlayableRoomType::Workshop
                         : PlayableRoomCatalog::behavior_for(
                               *availability.definition);
    entry.unlocked = availability.unlocked;
    entry.reason = availability.reason;
    entry.next_step = availability.next_step;
    return entry;
}

}  // namespace

PlayableRoomCatalog::PlayableRoomCatalog()
    : catalog_(rooms::built_in_room_definitions()) {}

bool PlayableRoomCatalog::valid() const {
    return catalog_.valid();
}

std::vector<PlayableRoomCatalogEntry> PlayableRoomCatalog::list(
    const rooms::UnlockContext& context) const {
    const auto availability = catalog_.list(context);
    std::vector<PlayableRoomCatalogEntry> entries;
    entries.reserve(availability.size());
    for (const auto& item : availability) {
        entries.push_back(make_entry(item));
    }
    return entries;
}

PlayableRoomCatalogEntry PlayableRoomCatalog::find(
    std::uint32_t stable_key,
    const rooms::UnlockContext& context) const {
    return make_entry(catalog_.availability(stable_key, context));
}

const rooms::RoomDefinition& PlayableRoomCatalog::resolve(
    std::uint32_t stable_key) const noexcept {
    return catalog_.resolve_or_placeholder(stable_key);
}

PlayableRoomType PlayableRoomCatalog::behavior_for(
    const rooms::RoomDefinition& definition) noexcept {
    if (definition.transport) return PlayableRoomType::Elevator;
    if (definition.produces == "resource.power") {
        return PlayableRoomType::Power;
    }
    if (definition.produces == "resource.food") {
        return PlayableRoomType::Food;
    }
    if (definition.produces == "resource.water") {
        return PlayableRoomType::Water;
    }
    if (definition.category == "residential") {
        return PlayableRoomType::Living;
    }
    return PlayableRoomType::Workshop;
}

}  // namespace deep_shelter::gameplay
