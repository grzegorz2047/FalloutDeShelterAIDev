#include "dwellers/Dweller.hpp"

#include <algorithm>
#include <charconv>
#include <limits>
#include <sstream>
#include <utility>

namespace deep_shelter::dwellers {

namespace {
template <typename T>
bool parse_integer(const std::string& value, T& output) {
    const char* begin = value.data();
    const char* end = begin + value.size();
    const auto parsed = std::from_chars(begin, end, output);
    return parsed.ec == std::errc{} && parsed.ptr == end;
}

void write_token(std::ostringstream& stream, const std::string& value) {
    stream << value.size() << ':' << value;
}

template <typename T>
void write_integer(std::ostringstream& stream, T value) {
    write_token(stream, std::to_string(value));
}

bool read_token(const std::string& payload, std::size_t& offset, std::string& output) {
    if (offset >= payload.size()) return false;
    const auto separator = payload.find(':', offset);
    if (separator == std::string::npos || separator == offset) return false;

    std::size_t length = 0;
    if (!parse_integer(payload.substr(offset, separator - offset), length)) return false;
    const std::size_t content_start = separator + 1;
    if (length > payload.size() - content_start) return false;

    output.assign(payload, content_start, length);
    offset = content_start + length;
    return true;
}

template <typename T>
bool read_integer(const std::string& payload, std::size_t& offset, T& output) {
    std::string token;
    return read_token(payload, offset, token) && parse_integer(token, output);
}

std::vector<std::string> split_plain(const std::string& value, char separator) {
    std::vector<std::string> result;
    std::string current;
    for (const char ch : value) {
        if (ch == separator) {
            result.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    result.push_back(current);
    return result;
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
    const auto fields = split_plain(value, ',');
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
    for (const auto& field : split_plain(value, ',')) {
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
    for (const auto& field : split_plain(value, ',')) {
        int level = 0;
        if (!parse_integer(field, level)) return false;
        levels.insert(level);
    }
    return true;
}

std::optional<Dweller> read_legacy_v0(const std::string& payload, std::size_t offset) {
    Dweller dweller;
    int status = 0;
    std::string base_special;
    std::string children;
    if (!read_integer(payload, offset, dweller.id) ||
        !read_token(payload, offset, dweller.name) ||
        !read_token(payload, offset, base_special) ||
        !read_integer(payload, offset, dweller.level) ||
        !read_integer(payload, offset, dweller.xp) ||
        !read_integer(payload, offset, dweller.max_hp) ||
        !read_integer(payload, offset, dweller.hp) ||
        !read_integer(payload, offset, dweller.radiation) ||
        !read_integer(payload, offset, dweller.happiness) ||
        !read_integer(payload, offset, status) ||
        !read_integer(payload, offset, dweller.room_id) ||
        !read_token(payload, offset, dweller.weapon_id) ||
        !read_token(payload, offset, dweller.outfit_id) ||
        !read_token(payload, offset, dweller.companion_id) ||
        !read_integer(payload, offset, dweller.parent_a) ||
        !read_integer(payload, offset, dweller.parent_b) ||
        !read_token(payload, offset, children) ||
        !parse_stats(base_special, dweller.base_special) ||
        !parse_ids(children, dweller.children) || offset != payload.size()) {
        return std::nullopt;
    }
    if (status < static_cast<int>(ActivityStatus::Idle) ||
        status > static_cast<int>(ActivityStatus::Dead))
        return std::nullopt;

    dweller.status = static_cast<ActivityStatus>(status);
    const int migrated_level = std::clamp(dweller.level, 1, 50);
    for (int level = 2; level <= migrated_level; ++level) dweller.awarded_levels.insert(level);
    dweller.history.push_back({0, "schema_migrated", "v0_to_v1"});
    dweller.normalize();
    return dweller;
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
        result.values[index] =
            std::clamp(result.values[index] + outfit_bonus.values[index], 1, 10);
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
    std::unordered_set<std::uint64_t> used;
    for (const auto& dweller : dwellers_) used.insert(dweller.id);

    std::uint64_t candidate = 1;
    while (used.count(candidate) != 0) {
        if (candidate == std::numeric_limits<std::uint64_t>::max()) return 0;
        ++candidate;
    }
    return candidate;
}

std::uint64_t DwellerService::add_with_unique_id(Dweller dweller) {
    if (dweller.id == 0 || find(dweller.id) != nullptr) dweller.id = next_unique_id();
    if (dweller.id == 0) return 0;
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
    if (dweller == nullptr || !dweller->alive() ||
        amount > std::numeric_limits<std::int64_t>::max() - dweller->xp)
        return false;

    const std::int64_t new_xp = dweller->xp + amount;
    const int target = table_.level_for_xp(new_xp);
    int new_level_rewards = 0;
    for (int candidate = dweller->level + 1; candidate <= target; ++candidate) {
        if (dweller->awarded_levels.count(candidate) == 0) ++new_level_rewards;
    }
    if (new_level_rewards >
        (std::numeric_limits<int>::max() - dweller->max_hp) / 5)
        return false;

    dweller->xp = new_xp;
    while (dweller->level < target) {
        const int next_level = dweller->level + 1;
        if (dweller->awarded_levels.insert(next_level).second) {
            dweller->level = next_level;
            dweller->max_hp += 5;
            const auto healed = static_cast<std::int64_t>(dweller->hp) + 5;
            dweller->hp = static_cast<int>(std::min<std::int64_t>(
                dweller->effective_max_hp(), healed));
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
    write_integer(stream, Dweller::kSchemaVersion);
    write_integer(stream, dweller.id);
    write_token(stream, dweller.name);
    write_token(stream, dweller.appearance_id);
    write_token(stream, dweller.presentation);
    write_token(stream, dweller.origin);
    write_token(stream, join_stats(dweller.base_special));
    write_token(stream, join_stats(dweller.outfit_bonus));
    write_integer(stream, dweller.level);
    write_integer(stream, dweller.xp);
    write_integer(stream, dweller.max_hp);
    write_integer(stream, dweller.hp);
    write_integer(stream, dweller.radiation);
    write_integer(stream, dweller.happiness);
    write_integer(stream, static_cast<int>(dweller.status));
    write_integer(stream, dweller.room_id);
    write_token(stream, dweller.weapon_id);
    write_token(stream, dweller.outfit_id);
    write_token(stream, dweller.companion_id);
    write_integer(stream, dweller.parent_a);
    write_integer(stream, dweller.parent_b);
    write_token(stream, join_ids(dweller.children));
    write_token(stream, join_awards(dweller.awarded_levels));
    write_integer(stream, dweller.history.size());
    for (const auto& event : dweller.history) {
        write_integer(stream, event.timestamp);
        write_token(stream, event.type);
        write_token(stream, event.detail);
    }
    return stream.str();
}

std::optional<Dweller> deserialize_dweller(const std::string& payload) {
    std::size_t offset = 0;
    int schema = 0;
    if (!read_integer(payload, offset, schema)) return std::nullopt;
    if (schema == 0) return read_legacy_v0(payload, offset);
    if (schema != Dweller::kSchemaVersion) return std::nullopt;

    Dweller dweller;
    int status = 0;
    std::size_t history_count = 0;
    std::string base_special;
    std::string outfit_bonus;
    std::string children;
    std::string awarded_levels;

    if (!read_integer(payload, offset, dweller.id) ||
        !read_token(payload, offset, dweller.name) ||
        !read_token(payload, offset, dweller.appearance_id) ||
        !read_token(payload, offset, dweller.presentation) ||
        !read_token(payload, offset, dweller.origin) ||
        !read_token(payload, offset, base_special) ||
        !read_token(payload, offset, outfit_bonus) ||
        !read_integer(payload, offset, dweller.level) ||
        !read_integer(payload, offset, dweller.xp) ||
        !read_integer(payload, offset, dweller.max_hp) ||
        !read_integer(payload, offset, dweller.hp) ||
        !read_integer(payload, offset, dweller.radiation) ||
        !read_integer(payload, offset, dweller.happiness) ||
        !read_integer(payload, offset, status) ||
        !read_integer(payload, offset, dweller.room_id) ||
        !read_token(payload, offset, dweller.weapon_id) ||
        !read_token(payload, offset, dweller.outfit_id) ||
        !read_token(payload, offset, dweller.companion_id) ||
        !read_integer(payload, offset, dweller.parent_a) ||
        !read_integer(payload, offset, dweller.parent_b) ||
        !read_token(payload, offset, children) ||
        !read_token(payload, offset, awarded_levels) ||
        !read_integer(payload, offset, history_count) ||
        !parse_stats(base_special, dweller.base_special) ||
        !parse_stats(outfit_bonus, dweller.outfit_bonus) ||
        !parse_ids(children, dweller.children) ||
        !parse_awards(awarded_levels, dweller.awarded_levels)) {
        return std::nullopt;
    }
    if (status < static_cast<int>(ActivityStatus::Idle) ||
        status > static_cast<int>(ActivityStatus::Dead))
        return std::nullopt;
    dweller.status = static_cast<ActivityStatus>(status);

    for (std::size_t index = 0; index < history_count; ++index) {
        DwellerEvent event;
        if (!read_integer(payload, offset, event.timestamp) ||
            !read_token(payload, offset, event.type) ||
            !read_token(payload, offset, event.detail))
            return std::nullopt;
        dweller.history.push_back(std::move(event));
    }
    if (offset != payload.size()) return std::nullopt;

    dweller.normalize();
    return dweller;
}

}  // namespace deep_shelter::dwellers
