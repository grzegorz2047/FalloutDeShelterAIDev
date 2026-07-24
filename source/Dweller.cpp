#include "dwellers/Dweller.hpp"

#include <algorithm>

namespace deep_shelter::dwellers {

void SpecialStats::clamp() noexcept {
    for (auto& value : values) value = std::clamp(value, 0, 10);
}

void Dweller::normalize() noexcept {
    base_special.clamp();
    outfit_bonus.clamp();
    level = std::clamp(level, 1, 50);
    xp = std::max<std::int64_t>(0, xp);
    max_hp = std::max(1, max_hp);
    radiation = std::clamp(radiation, 0, max_hp);
    hp = std::clamp(hp, 0, effective_max_hp());
    happiness = std::clamp(happiness, 0, 100);
    if (hp == 0) status = ActivityStatus::Dead;
}

SpecialStats Dweller::effective_special() const noexcept {
    SpecialStats result = base_special;
    for (std::size_t index = 0; index < result.values.size(); ++index)
        result.values[index] = std::clamp(result.values[index] + outfit_bonus.values[index], 0, 10);
    return result;
}

int Dweller::effective_max_hp() const noexcept { return std::max(1, max_hp - radiation); }
bool Dweller::alive() const noexcept { return hp > 0 && status != ActivityStatus::Dead; }

bool XpTable::valid() const noexcept {
    if (max_level < 1 || thresholds.empty() || thresholds.front() != 0) return false;
    if (static_cast<int>(thresholds.size()) < max_level) return false;
    for (std::size_t index = 1; index < thresholds.size(); ++index)
        if (thresholds[index] <= thresholds[index - 1]) return false;
    return true;
}

int XpTable::level_for_xp(std::int64_t value) const noexcept {
    if (!valid()) return 1;
    int result = 1;
    for (int level = 2; level <= max_level; ++level) {
        if (value < thresholds[static_cast<std::size_t>(level - 1)]) break;
        result = level;
    }
    return result;
}

DwellerService::DwellerService(XpTable table) : table_(std::move(table)) {}

bool DwellerService::add(Dweller dweller) {
    if (dweller.id == 0 || dweller.name.empty() || !table_.valid()) return false;
    if (find(dweller.id) != nullptr) return false;
    dweller.normalize();
    dwellers_.push_back(std::move(dweller));
    return true;
}

Dweller* DwellerService::find(std::uint64_t id) noexcept {
    auto it = std::find_if(dwellers_.begin(), dwellers_.end(),
                           [id](const Dweller& value) { return value.id == id; });
    return it == dwellers_.end() ? nullptr : &*it;
}

const std::vector<Dweller>& DwellerService::all() const noexcept { return dwellers_; }

bool DwellerService::grant_xp(std::uint64_t id, std::int64_t amount,
                              std::uint64_t transaction_id, std::int64_t timestamp) {
    if (amount <= 0 || transaction_id == 0 || transactions_.count(transaction_id) != 0)
        return false;
    auto* dweller = find(id);
    if (dweller == nullptr || !dweller->alive()) return false;
    dweller->xp += amount;
    const int target = table_.level_for_xp(dweller->xp);
    while (dweller->level < target) {
        const int next_level = dweller->level + 1;
        if (dweller->awarded_levels.insert(next_level).second) {
            dweller->level = next_level;
            dweller->max_hp += 5;
            dweller->hp = std::min(dweller->effective_max_hp(), dweller->hp + 5);
            dweller->history.push_back({timestamp, "level_up", std::to_string(next_level)});
        } else {
            dweller->level = next_level;
        }
    }
    transactions_.insert(transaction_id);
    return true;
}

void DwellerService::resolve_equipment(const std::unordered_set<std::string>& weapons,
                                       const std::unordered_set<std::string>& outfits,
                                       const std::unordered_set<std::string>& companions) {
    for (auto& dweller : dwellers_) {
        if (!dweller.weapon_id.empty() && weapons.count(dweller.weapon_id) == 0)
            dweller.weapon_id = "weapon.unknown";
        if (!dweller.outfit_id.empty() && outfits.count(dweller.outfit_id) == 0) {
            dweller.outfit_id = "outfit.unknown";
            dweller.outfit_bonus = {};
        }
        if (!dweller.companion_id.empty() && companions.count(dweller.companion_id) == 0)
            dweller.companion_id = "companion.unknown";
        dweller.normalize();
    }
}

bool DwellerService::valid_unique_ids() const noexcept {
    std::unordered_set<std::uint64_t> ids;
    for (const auto& dweller : dwellers_) {
        if (dweller.id == 0 || !ids.insert(dweller.id).second) return false;
    }
    return true;
}

}  // namespace deep_shelter::dwellers
