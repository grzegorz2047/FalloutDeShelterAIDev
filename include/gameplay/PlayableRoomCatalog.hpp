#pragma once

#include <cstdint>
#include <vector>

#include "gameplay/PlayableShelterSession.hpp"
#include "rooms/RoomCatalog.hpp"

namespace deep_shelter::gameplay {

struct PlayableRoomCatalogEntry {
    const rooms::RoomDefinition* definition = nullptr;
    PlayableRoomType behavior = PlayableRoomType::Workshop;
    bool unlocked = false;
    const char* reason = nullptr;
    const char* next_step = nullptr;
};

class PlayableRoomCatalog {
public:
    PlayableRoomCatalog();

    [[nodiscard]] bool valid() const;
    [[nodiscard]] std::vector<PlayableRoomCatalogEntry> list(
        const rooms::UnlockContext& context) const;
    [[nodiscard]] PlayableRoomCatalogEntry find(
        std::uint32_t stable_key,
        const rooms::UnlockContext& context) const;
    [[nodiscard]] const rooms::RoomDefinition& resolve(
        std::uint32_t stable_key) const noexcept;

    [[nodiscard]] static PlayableRoomType behavior_for(
        const rooms::RoomDefinition& definition) noexcept;

private:
    rooms::RoomCatalog catalog_;
};

}  // namespace deep_shelter::gameplay
