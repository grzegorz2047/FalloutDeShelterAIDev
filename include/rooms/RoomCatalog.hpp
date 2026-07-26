#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace deep_shelter::rooms {

template <std::size_t Size>
[[nodiscard]] constexpr std::uint32_t room_stable_key(
    const char (&id)[Size]) noexcept {
    std::uint32_t hash = 2166136261u;
    for (std::size_t index = 0; index + 1u < Size; ++index) {
        hash ^= static_cast<std::uint8_t>(id[index]);
        hash *= 16777619u;
    }
    return hash;
}

[[nodiscard]] constexpr std::uint32_t room_stable_key(
    std::string_view id) noexcept {
    std::uint32_t hash = 2166136261u;
    for (const char character : id) {
        hash ^= static_cast<std::uint8_t>(character);
        hash *= 16777619u;
    }
    return hash;
}

struct RoomDefinition {
    std::uint32_t stable_key = 0;
    std::string id;
    std::string category;
    std::string display_name;
    std::string name_key;
    std::string description_key;
    std::string icon;
    char special = 'S';
    int base_cost = 0;
    int width = 1;
    int max_level = 1;
    int unlock_population = 0;
    int unlock_progress = 0;
    std::string required_achievement;
    std::string produces;
    int storage_bonus = 0;
    bool transport = false;
};

struct UnlockContext {
    int population = 0;
    int progress = 0;
    std::unordered_set<std::string> achievements;
};

struct RoomAvailability {
    const RoomDefinition* definition = nullptr;
    bool unlocked = false;
    std::string reason;
    std::string next_step;
};

class RoomCatalog {
public:
    explicit RoomCatalog(std::vector<RoomDefinition> definitions);

    [[nodiscard]] bool valid(std::string* detail = nullptr) const;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] const RoomDefinition* find(const std::string& id) const noexcept;
    [[nodiscard]] const RoomDefinition* find(std::uint32_t stable_key) const noexcept;
    [[nodiscard]] RoomAvailability availability(const std::string& id,
                                                 const UnlockContext& context) const;
    [[nodiscard]] RoomAvailability availability(std::uint32_t stable_key,
                                                 const UnlockContext& context) const;
    [[nodiscard]] std::vector<RoomAvailability> list(const UnlockContext& context) const;
    [[nodiscard]] const RoomDefinition& resolve_or_placeholder(
        const std::string& id) const noexcept;
    [[nodiscard]] const RoomDefinition& resolve_or_placeholder(
        std::uint32_t stable_key) const noexcept;

private:
    [[nodiscard]] RoomAvailability availability(
        const RoomDefinition* definition,
        const UnlockContext& context) const;

    std::vector<RoomDefinition> definitions_;
    RoomDefinition placeholder_;
};

std::vector<RoomDefinition> built_in_room_definitions();

}  // namespace deep_shelter::rooms
