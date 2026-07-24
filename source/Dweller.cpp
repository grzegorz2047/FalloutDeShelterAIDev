#include "dwellers/Dweller.hpp"

#include <algorithm>
#include <charconv>
#include <sstream>
#include <utility>

namespace deep_shelter::dwellers {

namespace {
std::string escape_field(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (const char ch : value) {
        if (ch == '\\' || ch == '|' || ch == ',' || ch == ';') result.push_back('\\');
        result.push_back(ch);
    }
    return result;
}

std::vector<std::string> split_escaped(const std::string& value, char separator) {
    std::vector<std::string> result;
    std::string current;
    bool escaped = false;
    for (const char ch : value) {
        if (escaped) {
            current.push_back(ch);
            escaped = false;
        } else if (ch == '\\') {
            escaped = true;
        } else if (ch == separator) {
            result.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    if (escaped) return {};
    result.push_back(current);
    return result;
}

template <typename T>
bool parse_integer(const std::string& value, T& output) {
    const char* begin = value.data();
    const char* end = begin + value.size();
    const auto parsed = std::from_chars(begin, end, output);
    return parsed.ec == std::errc{} && parsed.ptr == end;
}

std::string join_stats(const SpecialStats& stats) {
    std::ostringstream stream;
    for (std::size_t index = 0; index < stats.values.size(); ++index) {
        if (index != 0) stream << ',';
        stream << stats.values[index];
    }
    return stream.str();
}

bool parse_stats(const std::string& value, SpecialStats& stats) {
    const auto fields = split_escaped(value, ',');
    if (fields.size() != stats.values.size()) return false;
    for (std::size_t index = 0; index < fields.size(); ++index) {
        if (!parse_integer(fields[index], stats.values[index])) return false;
    }
    return true;
}

std::string join_ids(const std::vector<std::uint64_t>& ids) {
    std::ostringstream stream;
    for (std::size_t index = 0; index < ids.size(); ++index) {
        if (index != 0) stream << ',';
        stream << ids[index];
    }
    return stream.str();
}

bool parse_ids(const std::string& value, std::vector<std::uint64_t>& ids) {
    ids.clear();
    if (value.empty()) return true;
    for (const auto& field : split_escaped(value, ',')) {
        std::uint64_t id = 0;
        if (!parse_integer(field, id)) return false;
        ids.push_back(id);
    }
    return true;
}

std::string join_awards(const std::unordered_set<int>& levels) {
    std::vector<int> sorted(levels.begin(), levels.end());
    std::sort(sorted.begin(), sorted.end());
    std::ostringstream stream;
    for (std::size_t index = 0; index < sorted.size(); ++index) {
        if (index != 0) stream << ',';
        stream << sorted[index];
    }
    return stream.str();
}

bool parse_awards(const std::string& value, std::unordered_set<int>& levels) {
    levels.clear();
    if (value.empty()) return true;
    for (const auto& field : split_escaped(value, ',')) {
        int level = 0;
        if (!parse_integer(field, level)) return false;
        levels.insert(level);
    }
    return true;
}
}  // namespace

void SpecialStats::clamp(int minimum) noexcept {
    minimum = std::clamp(minimum, 0, 10);
    for (auto& value : values) value = std::clamp(value, minimum, 10);
}

void Dweller::normalize(int max_level) noexcept {
    base_special.clamp(1);
    outfit_bonus.clamp(0);
    level = std::clamp(level, 1, std::max(1, max_level));
    xp = std::max<std::int64_t>(0, xp);
    max_hp = std::max(1, max_hp);
    radiation = std::clamp(radiation, 0, max_hp);
    hp = std::clamp(hp, 0, effective_max_hp());
    happiness = std::clamp(happiness, 0, 100);
    if (hp == 0) status = ActivityStatus::Dead;
}

SpecialStats Dweller::effective_special() const noexcept {
    SpecialStats result = base_special;
    for (std::size_t index = 0; index < result.values.size(); ++index) {
        result.values[index] = std::clamp(result.values[index] + outfit_bonus.values[index], 1, 10);
    }
    return result;
}

int Dweller::effective_max_hp() const noexcept { return std::max(1, max_hp - radiation); }
bool Dweller::alive() const noexcept { return hp > 0 && status != ActivityStatus::Dead; }

bool XpTable::valid() const noexcept {
    if (max_level < 1 || thresholds.empty() || thresholds.front() != 0) return false;
    if (static_cast<int>(thresholds.size()) < max_level) return false;
    for (std::size_t index = 1; index < thresholds.size(); ++index) {
        if (thresholds[index] <= thresholds[index - 1]) return false;
    }
    return true;
}

int XpTable::level_for_xp(std::int64_t value) const noexcept {
    if (!valid()) return 1;
    int result = 1;
    for (int candidate = 2; candidate <= max_level; ++candidate) {
        if (value < thresholds[static_cast<std::size_t>(candidate - 1)]) break;
        result = candidate;
    }
    return result;
}

DwellerService::DwellerService(XpTable table) : table_(std::move(table)) {}

bool DwellerService::add(Dweller dweller) {
    if (dweller.id == 0 || dweller.name.empty() || !table_.valid()) return false;
    if (find(dweller.id) != nullptr) return false;
    dweller.normalize(table_.max_level);
    dwellers_.push_back(std::move(dweller));
    return true;
}

std::uint64_t DwellerService::next_unique_id() const noexcept {
    std::uint64_t candidate = 1;
    for (const auto& dweller : dwellers_) candidate = std::max(candidate, dweller.id + 1);
    return candidate;
}

std::uint64_t DwellerService::add_with_unique_id(Dweller dweller) {
    if (dweller.id == 0 || find(dweller.id) != nullptr) dweller.id = next_unique_id();
    const auto assigned = dweller.id;
    return add(std::move(dweller)) ? assigned : 0;
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
            dweller.outfit_bonus.values.fill(0);
        }
        if (!dweller.companion_id.empty() && companions.count(dweller.companion_id) == 0)
            dweller.companion_id = "companion.unknown";
        dweller.normalize(table_.max_level);
    }
}

bool DwellerService::valid_unique_ids() const noexcept {
    std::unordered_set<std::uint64_t> ids;
    for (const auto& dweller : dwellers_) {
        if (dweller.id == 0 || !ids.insert(dweller.id).second) return false;
    }
    return true;
}

std::string serialize_dweller(const Dweller& dweller) {
    std::ostringstream stream;
    stream << Dweller::kSchemaVersion << '|'
           << dweller.id << '|'
           << escape_field(dweller.name) << '|'
           << escape_field(dweller.appearance_id) << '|'
           << escape_field(dweller.presentation) << '|'
           << escape_field(dweller.origin) << '|'
           << join_stats(dweller.base_special) << '|'
           << join_stats(dweller.outfit_bonus) << '|'
           << dweller.level << '|'
           << dweller.xp << '|'
           << dweller.max_hp << '|'
           << dweller.hp << '|'
           << dweller.radiation << '|'
           << dweller.happiness << '|'
           << static_cast<int>(dweller.status) << '|'
           << dweller.room_id << '|'
           << escape_field(dweller.weapon_id) << '|'
           << escape_field(dweller.outfit_id) << '|'
           << escape_field(dweller.companion_id) << '|'
           << dweller.parent_a << '|'
           << dweller.parent_b << '|'
           << join_ids(dweller.children) << '|'
           << join_awards(dweller.awarded_levels) << '|'
           << dweller.history.size();
    for (const auto& event : dweller.history) {
        stream << '|' << event.timestamp << ',' << escape_field(event.type) << ','
               << escape_field(event.detail);
    }
    return stream.str();
}

std::optional<Dweller> deserialize_dweller(const std::string& payload) {
    const auto fields = split_escaped(payload, '|');
    if (fields.size() < 24) return std::nullopt;
    int schema = 0;
    if (!parse_integer(fields[0], schema) || schema != Dweller::kSchemaVersion) return std::nullopt;

    Dweller dweller;
    int status = 0;
    std::size_t history_count = 0;
    if (!parse_integer(fields[1], dweller.id) ||
        !parse_stats(fields[6], dweller.base_special) ||
        !parse_stats(fields[7], dweller.outfit_bonus) ||
        !parse_integer(fields[8], dweller.level) ||
        !parse_integer(fields[9], dweller.xp) ||
        !parse_integer(fields[10], dweller.max_hp) ||
        !parse_integer(fields[11], dweller.hp) ||
        !parse_integer(fields[12], dweller.radiation) ||
        !parse_integer(fields[13], dweller.happiness) ||
        !parse_integer(fields[14], status) ||
        !parse_integer(fields[15], dweller.room_id) ||
        !parse_integer(fields[19], dweller.parent_a) ||
        !parse_integer(fields[20], dweller.parent_b) ||
        !parse_ids(fields[21], dweller.children) ||
        !parse_awards(fields[22], dweller.awarded_levels) ||
        !parse_integer(fields[23], history_count)) {
        return std::nullopt;
    }
    if (status < static_cast<int>(ActivityStatus::Idle) ||
        status > static_cast<int>(ActivityStatus::Dead) ||
        fields.size() != 24 + history_count) {
        return std::nullopt;
    }

    dweller.name = fields[2];
    dweller.appearance_id = fields[3];
    dweller.presentation = fields[4];
    dweller.origin = fields[5];
    dweller.status = static_cast<ActivityStatus>(status);
    dweller.weapon_id = fields[16];
    dweller.outfit_id = fields[17];
    dweller.companion_id = fields[18];

    for (std::size_t index = 0; index < history_count; ++index) {
        const auto event_fields = split_escaped(fields[24 + index], ',');
        if (event_fields.size() != 3) return std::nullopt;
        DwellerEvent event;
        if (!parse_integer(event_fields[0], event.timestamp)) return std::nullopt;
        event.type = event_fields[1];
        event.detail = event_fields[2];
        dweller.history.push_back(std::move(event));
    }
    dweller.normalize();
    return dweller;
}

}  // namespace deep_shelter::dwellers
