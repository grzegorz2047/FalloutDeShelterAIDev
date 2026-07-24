#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace deep_shelter::rooms {

struct RoomDefinition {
    std::string id;
    std::string category;
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
    [[nodiscard]] RoomAvailability availability(const std::string& id,
                                                const UnlockContext& context) const;
    [[nodiscard]] std::vector<RoomAvailability> list(const UnlockContext& context) const;
    [[nodiscard]] const RoomDefinition& resolve_or_placeholder(const std::string& id) const noexcept;

private:
    std::vector<RoomDefinition> definitions_;
    RoomDefinition placeholder_;
};

std::vector<RoomDefinition> built_in_room_definitions();

}  // namespace deep_shelter::rooms
