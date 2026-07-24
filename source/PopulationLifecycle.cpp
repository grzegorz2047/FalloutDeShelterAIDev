#include "dwellers/PopulationLifecycle.hpp"

#include <algorithm>
#include <limits>

namespace deep_shelter::dwellers {

namespace {
PopulationRecord* find_record(std::vector<std::pair<std::uint64_t, PopulationRecord>>& records,
                              std::uint64_t id) {
    const auto it = std::find_if(records.begin(), records.end(), [id](const auto& value) {
        return value.first == id;
    });
    return it == records.end() ? nullptr : &it->second;
}

const PopulationRecord* find_record(
    const std::vector<std::pair<std::uint64_t, PopulationRecord>>& records,
    std::uint64_t id) {
    const auto it = std::find_if(records.begin(), records.end(), [id](const auto& value) {
        return value.first == id;
    });
    return it == records.end() ? nullptr : &it->second;
}

std::int64_t safe_deadline(std::int64_t now, std::int64_t duration) {
    return time::saturating_add_ms(now, std::max<std::int64_t>(1, duration));
}
}  // namespace

PopulationLifecycleService::PopulationLifecycleService(DwellerService& dwellers,
                                                       time::TrustedClock& clock,
                                                       PopulationConfig config)
    : dwellers_(dwellers), clock_(clock), config_(config) {
    set_capacities(config.population_limit, config.quarters_capacity);
    config_.pregnancy_ms = std::max<std::int64_t>(1, config_.pregnancy_ms);
    config_.childhood_ms = std::max<std::int64_t>(1, config_.childhood_ms);
    migrate_missing_records();
}

void PopulationLifecycleService::set_capacities(int population_limit,
                                                int quarters_capacity) noexcept {
    config_.population_limit = std::max(0, population_limit);
    config_.quarters_capacity = std::max(0, quarters_capacity);
}

void PopulationLifecycleService::migrate_missing_records() {
    for (const auto& dweller : dwellers_.all()) {
        if (find_record(records_, dweller.id) == nullptr)
            records_.push_back({dweller.id, {}});
    }
    std::sort(records_.begin(), records_.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
}

const PopulationRecord* PopulationLifecycleService::record(std::uint64_t id) const {
    return find_record(records_, id);
}

bool PopulationLifecycleService::is_ancestor(std::uint64_t ancestor,
                                             std::uint64_t descendant,
                                             int depth) const {
    if (ancestor == 0 || descendant == 0 || depth > 8) return false;
    if (ancestor == descendant) return true;
    const auto* value = dwellers_.find(descendant);
    if (value == nullptr) return false;
    return is_ancestor(ancestor, value->parent_a, depth + 1) ||
           is_ancestor(ancestor, value->parent_b, depth + 1);
}

bool PopulationLifecycleService::close_relative(const Dweller& first,
                                                const Dweller& second) const {
    if (first.id == second.id || is_ancestor(first.id, second.id) ||
        is_ancestor(second.id, first.id))
        return true;

    const bool shared_parent =
        (first.parent_a != 0 &&
         (first.parent_a == second.parent_a || first.parent_a == second.parent_b)) ||
        (first.parent_b != 0 &&
         (first.parent_b == second.parent_a || first.parent_b == second.parent_b));
    return shared_parent;
}

bool PopulationLifecycleService::adult_and_alive(std::uint64_t id) const {
    const auto* dweller = dwellers_.find(id);
    const auto* value = record(id);
    return dweller != nullptr && dweller->alive() && value != nullptr &&
           value->stage == LifeStage::Adult;
}

std::size_t PopulationLifecycleService::pending_births(std::uint64_t excluded_mother) const {
    return static_cast<std::size_t>(std::count_if(records_.begin(), records_.end(),
        [excluded_mother](const auto& value) {
            return value.first != excluded_mother &&
                   value.second.stage == LifeStage::Pregnant &&
                   !value.second.birth_completed;
        }));
}

PopulationError PopulationLifecycleService::capacity_error(std::uint64_t excluded_mother) const {
    const std::size_t committed = dwellers_.all().size() + pending_births(excluded_mother) + 1;
    if (committed > static_cast<std::size_t>(config_.population_limit))
        return PopulationError::PopulationFull;
    if (committed > static_cast<std::size_t>(config_.quarters_capacity))
        return PopulationError::QuartersFull;
    return PopulationError::None;
}

PopulationError PopulationLifecycleService::pair(std::uint64_t first_id,
                                                 std::uint64_t second_id) {
    migrate_missing_records();
    auto* first = dwellers_.find(first_id);
    auto* second = dwellers_.find(second_id);
    if (first == nullptr || second == nullptr) return PopulationError::UnknownDweller;
    if (!adult_and_alive(first_id) || !adult_and_alive(second_id) || first_id == second_id)
        return PopulationError::InvalidPair;
    if (close_relative(*first, *second)) return PopulationError::CloseRelative;

    auto* first_record = find_record(records_, first_id);
    auto* second_record = find_record(records_, second_id);
    first_record->partner_id = second_id;
    second_record->partner_id = first_id;
    return PopulationError::None;
}

PopulationError PopulationLifecycleService::start_pregnancy(std::uint64_t mother_id,
                                                            std::uint64_t father_id) {
    migrate_missing_records();
    const std::int64_t now = clock_.trusted_now_ms();
    if (now < last_trusted_now_ms_) return PopulationError::ClockRollback;

    auto* mother = dwellers_.find(mother_id);
    auto* father = dwellers_.find(father_id);
    if (mother == nullptr || father == nullptr) return PopulationError::UnknownDweller;
    if (!adult_and_alive(mother_id) || !adult_and_alive(father_id))
        return PopulationError::NotAdult;
    if (mother_id == father_id) return PopulationError::InvalidPair;
    if (close_relative(*mother, *father)) return PopulationError::CloseRelative;

    auto* value = find_record(records_, mother_id);
    if (value->stage == LifeStage::Pregnant) return PopulationError::AlreadyPregnant;
    const auto capacity = capacity_error(mother_id);
    if (capacity != PopulationError::None) return capacity;

    value->stage = LifeStage::Pregnant;
    value->partner_id = father_id;
    value->pregnancy_due_ms = safe_deadline(now, config_.pregnancy_ms);
    value->birth_completed = false;
    last_trusted_now_ms_ = now;
    events_.push_back({now, "pregnancy_started", mother_id, std::to_string(father_id)});
    return PopulationError::None;
}

std::string PopulationLifecycleService::generated_name(std::uint64_t mother,
                                                       std::uint64_t father,
                                                       std::uint64_t ordinal) const {
    return "Child-" + std::to_string(mother) + "-" + std::to_string(father) + "-" +
           std::to_string(ordinal);
}

SpecialStats PopulationLifecycleService::generated_special(std::uint64_t mother,
                                                           std::uint64_t father,
                                                           std::int64_t due_ms) const {
    SpecialStats result;
    std::uint64_t seed = mother * 0x9E3779B185EBCA87ULL ^
                         father * 0xC2B2AE3D27D4EB4FULL ^
                         static_cast<std::uint64_t>(due_ms);
    for (auto& value : result.values) {
        seed ^= seed >> 12;
        seed ^= seed << 25;
        seed ^= seed >> 27;
        value = 1 + static_cast<int>((seed * 2685821657736338717ULL) % 3ULL);
    }
    return result;
}

void PopulationLifecycleService::advance() {
    const std::int64_t now = clock_.trusted_now_ms();
    if (now < last_trusted_now_ms_) return;
    last_trusted_now_ms_ = now;

    std::vector<std::uint64_t> mothers;
    for (const auto& value : records_) {
        if (value.second.stage == LifeStage::Pregnant &&
            !value.second.birth_completed && value.second.pregnancy_due_ms <= now)
            mothers.push_back(value.first);
    }
    std::sort(mothers.begin(), mothers.end());

    for (const auto mother_id : mothers) {
        auto* pregnancy = find_record(records_, mother_id);
        if (pregnancy == nullptr || pregnancy->birth_completed) continue;
        const auto capacity = capacity_error(mother_id);
        if (capacity != PopulationError::None) {
            events_.push_back({now, "birth_blocked", mother_id, capacity_reason()});
            continue;
        }

        const auto* mother = dwellers_.find(mother_id);
        const auto* father = dwellers_.find(pregnancy->partner_id);
        if (mother == nullptr || father == nullptr) {
            events_.push_back({now, "birth_blocked", mother_id, "missing_parent"});
            continue;
        }

        Dweller child;
        child.name = generated_name(mother_id, pregnancy->partner_id,
                                    static_cast<std::uint64_t>(dwellers_.all().size() + 1));
        child.origin = "shelter_birth";
        child.parent_a = mother_id;
        child.parent_b = pregnancy->partner_id;
        child.base_special = generated_special(mother_id, pregnancy->partner_id,
                                               pregnancy->pregnancy_due_ms);
        const auto child_id = dwellers_.add_with_unique_id(std::move(child));
        if (child_id == 0) {
            events_.push_back({now, "birth_blocked", mother_id, "id_exhausted"});
            continue;
        }

        records_.push_back({child_id,
                            {LifeStage::Child, 0, mother_id, pregnancy->partner_id, 0,
                             safe_deadline(now, config_.childhood_ms), false}});
        std::sort(records_.begin(), records_.end(), [](const auto& a, const auto& b) {
            return a.first < b.first;
        });
        if (auto* parent = dwellers_.find(mother_id)) parent->children.push_back(child_id);
        if (auto* parent = dwellers_.find(pregnancy->partner_id)) parent->children.push_back(child_id);
        pregnancy = find_record(records_, mother_id);
        pregnancy->stage = LifeStage::Adult;
        pregnancy->birth_completed = true;
        pregnancy->pregnancy_due_ms = 0;
        events_.push_back({now, "birth", child_id, std::to_string(mother_id)});
    }

    for (auto& value : records_) {
        if (value.second.stage == LifeStage::Child &&
            value.second.adulthood_due_ms <= now) {
            value.second.stage = LifeStage::Adult;
            value.second.adulthood_due_ms = 0;
            events_.push_back({now, "adult", value.first, {}});
        }
    }
}

bool PopulationLifecycleService::can_work(std::uint64_t id) const {
    return adult_and_alive(id);
}

bool PopulationLifecycleService::can_fight(std::uint64_t id) const {
    return adult_and_alive(id);
}

bool PopulationLifecycleService::can_travel(std::uint64_t id) const {
    return adult_and_alive(id);
}

const std::vector<PopulationEvent>& PopulationLifecycleService::events() const noexcept {
    return events_;
}

std::string PopulationLifecycleService::capacity_reason() const {
    const std::size_t committed = dwellers_.all().size() + pending_births();
    if (committed >= static_cast<std::size_t>(config_.population_limit))
        return "population_limit: increase the shelter population limit";
    if (committed >= static_cast<std::size_t>(config_.quarters_capacity))
        return "quarters_full: build or upgrade living quarters";
    return {};
}

PopulationSnapshot PopulationLifecycleService::snapshot() const {
    PopulationSnapshot result;
    result.records = records_;
    result.last_trusted_now_ms = last_trusted_now_ms_;
    return result;
}

bool PopulationLifecycleService::restore(const PopulationSnapshot& snapshot_value) {
    if (snapshot_value.schema_version != PopulationSnapshot::kSchemaVersion ||
        snapshot_value.last_trusted_now_ms < 0)
        return false;

    std::vector<std::pair<std::uint64_t, PopulationRecord>> restored = snapshot_value.records;
    std::sort(restored.begin(), restored.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    for (std::size_t index = 0; index < restored.size(); ++index) {
        if (restored[index].first == 0 || dwellers_.find(restored[index].first) == nullptr)
            return false;
        if (index > 0 && restored[index - 1].first == restored[index].first) return false;
        const auto& value = restored[index].second;
        if (value.stage == LifeStage::Pregnant && value.pregnancy_due_ms <= 0) return false;
        if (value.stage == LifeStage::Child && value.adulthood_due_ms <= 0) return false;
    }

    records_ = std::move(restored);
    last_trusted_now_ms_ = snapshot_value.last_trusted_now_ms;
    migrate_missing_records();
    return true;
}

}  // namespace deep_shelter::dwellers
