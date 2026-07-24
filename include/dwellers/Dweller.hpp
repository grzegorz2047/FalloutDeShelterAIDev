#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace deep_shelter::dwellers {

enum class ActivityStatus { Idle, Working, Training, Exploring, Questing, Injured, Dead };

struct SpecialStats {
    std::array<int, 7> values{{1, 1, 1, 1, 1, 1, 1}};
    void clamp(int minimum) noexcept;
};

struct DwellerEvent {
    std::int64_t timestamp = 0;
    std::string type;
    std::string detail;
};

struct Dweller {
    static constexpr int kSchemaVersion = 1;

    std::uint64_t id = 0;
    std::string name;
    std::string appearance_id;
    std::string presentation;
    std::string origin;
    SpecialStats base_special;
    SpecialStats outfit_bonus{{{0, 0, 0, 0, 0, 0, 0}}};
    int level = 1;
    std::int64_t xp = 0;
    int max_hp = 100;
    int hp = 100;
    int radiation = 0;
    int happiness = 50;
    ActivityStatus status = ActivityStatus::Idle;
    std::uint64_t room_id = 0;
    std::string weapon_id;
    std::string outfit_id;
    std::string companion_id;
    std::uint64_t parent_a = 0;
    std::uint64_t parent_b = 0;
    std::vector<std::uint64_t> children;
    std::vector<DwellerEvent> history;
    std::unordered_set<int> awarded_levels;

    void normalize(int max_level = 50) noexcept;
    [[nodiscard]] SpecialStats effective_special() const noexcept;
    [[nodiscard]] int effective_max_hp() const noexcept;
    [[nodiscard]] bool alive() const noexcept;
};

struct XpTable {
    std::vector<std::int64_t> thresholds;
    int max_level = 50;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] int level_for_xp(std::int64_t xp) const noexcept;
};

class DwellerService {
public:
    explicit DwellerService(XpTable table);

    bool add(Dweller dweller);
    std::uint64_t add_with_unique_id(Dweller dweller);
    [[nodiscard]] Dweller* find(std::uint64_t id) noexcept;
    [[nodiscard]] const std::vector<Dweller>& all() const noexcept;
    bool grant_xp(std::uint64_t id, std::int64_t amount, std::uint64_t transaction_id,
                  std::int64_t timestamp);
    void resolve_equipment(const std::unordered_set<std::string>& weapons,
                           const std::unordered_set<std::string>& outfits,
                           const std::unordered_set<std::string>& companions);
    [[nodiscard]] bool valid_unique_ids() const noexcept;

private:
    [[nodiscard]] std::uint64_t next_unique_id() const noexcept;

    XpTable table_;
    std::vector<Dweller> dwellers_;
    std::unordered_set<std::uint64_t> transactions_;
};

[[nodiscard]] std::string serialize_dweller(const Dweller& dweller);
[[nodiscard]] std::optional<Dweller> deserialize_dweller(const std::string& payload);

}  // namespace deep_shelter::dwellers
