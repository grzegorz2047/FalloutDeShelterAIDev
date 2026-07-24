#include "dwellers/PopulationLifecycle.hpp"

#include <algorithm>

namespace deep_shelter::dwellers {

PopulationLifecycleService::PopulationLifecycleService(DwellerService& dwellers,
                                                       PopulationConfig config)
    : dwellers_(dwellers), config_(config) {
    migrate_missing_records();
}

void PopulationLifecycleService::migrate_missing_records() {
    for (const auto& dweller : dwellers_.all()) records_.try_emplace(dweller.id);
}

const PopulationRecord* PopulationLifecycleService::record(std::uint64_t id) const {
    const auto it = records_.find(id);
    return it == records_.end() ? nullptr : &it->second;
}

bool PopulationLifecycleService::close_relative(const Dweller& first, const Dweller& second) const {
    if (first.id == second.id) return true;
    if (first.parent_a == second.id || first.parent_b == second.id ||
        second.parent_a == first.id || second.parent_b == first.id)
        return true;
    const bool shared_parent =
        (first.parent_a != 0 && (first.parent_a == second.parent_a || first.parent_a == second.parent_b)) ||
        (first.parent_b != 0 && (first.parent_b == second.parent_a || first.parent_b == second.parent_b));
    return shared_parent;
}

PopulationError PopulationLifecycleService::pair(std::uint64_t first_id,
                                                  std::uint64_t second_id) {
    auto* first = dwellers_.find(first_id);
    auto* second = dwellers_.find(second_id);
    if (first == nullptr || second == nullptr) return PopulationError::UnknownDweller;
    if (!first->alive() || !second->alive() || first_id == second_id)
        return PopulationError::InvalidPair;
    if (close_relative(*first, *second)) return PopulationError::CloseRelative;
    records_[first_id].partner_id = second_id;
    records_[second_id].partner_id = first_id;
    return PopulationError::None;
}

bool PopulationLifecycleService::has_capacity_for_birth() const {
    const auto size = static_cast<int>(dwellers_.all().size());
    return size < config_.population_limit && size < config_.quarters_capacity;
}

PopulationError PopulationLifecycleService::start_pregnancy(std::uint64_t mother_id,
                                                             std::uint64_t father_id,
                                                             std::int64_t trusted_now) {
    if (trusted_now < last_trusted_now_) return PopulationError::ClockRollback;
    auto* mother = dwellers_.find(mother_id);
    auto* father = dwellers_.find(father_id);
    if (mother == nullptr || father == nullptr) return PopulationError::UnknownDweller;
    if (close_relative(*mother, *father)) return PopulationError::CloseRelative;
    if (!has_capacity_for_birth()) {
        return static_cast<int>(dwellers_.all().size()) >= config_.population_limit
                   ? PopulationError::PopulationFull
                   : PopulationError::QuartersFull;
    }
    auto& record = records_[mother_id];
    if (record.stage == LifeStage::Pregnant) return PopulationError::AlreadyPregnant;
    record.stage = LifeStage::Pregnant;
    record.partner_id = father_id;
    record.pregnancy_due = trusted_now + std::max<std::int64_t>(1, config_.pregnancy_seconds);
    record.birth_completed = false;
    last_trusted_now_ = trusted_now;
    events_.push_back({trusted_now, "pregnancy_started", mother_id, std::to_string(father_id)});
    return PopulationError::None;
}

std::string PopulationLifecycleService::generated_name(std::uint64_t mother,
                                                       std::uint64_t father) const {
    return "Child-" + std::to_string(mother) + "-" + std::to_string(father);
}

void PopulationLifecycleService::advance(std::int64_t trusted_now) {
    if (trusted_now < last_trusted_now_) return;
    last_trusted_now_ = trusted_now;
    std::vector<std::uint64_t> mothers;
    for (const auto& [id, record] : records_) {
        if (record.stage == LifeStage::Pregnant && !record.birth_completed &&
            record.pregnancy_due <= trusted_now)
            mothers.push_back(id);
    }
    std::sort(mothers.begin(), mothers.end());
    for (const auto mother_id : mothers) {
        auto& pregnancy = records_[mother_id];
        if (!has_capacity_for_birth()) {
            events_.push_back({trusted_now, "birth_blocked", mother_id, capacity_reason()});
            continue;
        }
        Dweller child;
        child.name = generated_name(mother_id, pregnancy.partner_id);
        child.origin = "shelter_birth";
        child.parent_a = mother_id;
        child.parent_b = pregnancy.partner_id;
        child.base_special.values.fill(1);
        const auto child_id = dwellers_.add_with_unique_id(std::move(child));
        if (child_id == 0) continue;
        records_[child_id] = {LifeStage::Child, 0, mother_id, pregnancy.partner_id, 0,
                              trusted_now + std::max<std::int64_t>(1, config_.childhood_seconds), false};
        if (auto* mother = dwellers_.find(mother_id)) mother->children.push_back(child_id);
        if (auto* father = dwellers_.find(pregnancy.partner_id)) father->children.push_back(child_id);
        pregnancy.stage = LifeStage::Adult;
        pregnancy.birth_completed = true;
        completed_births_.insert(mother_id);
        events_.push_back({trusted_now, "birth", child_id, std::to_string(mother_id)});
    }

    for (auto& [id, record] : records_) {
        if (record.stage == LifeStage::Child && record.adulthood_due <= trusted_now) {
            record.stage = LifeStage::Adult;
            events_.push_back({trusted_now, "adult", id, {}});
        }
    }
}

bool PopulationLifecycleService::can_work(std::uint64_t id) const {
    const auto* value = record(id);
    const auto* dweller = dwellers_.find(id);
    return value != nullptr && value->stage == LifeStage::Adult && dweller != nullptr && dweller->alive();
}

const std::vector<PopulationEvent>& PopulationLifecycleService::events() const noexcept {
    return events_;
}

std::string PopulationLifecycleService::capacity_reason() const {
    const auto size = static_cast<int>(dwellers_.all().size());
    if (size >= config_.population_limit) return "population_limit: increase the shelter limit";
    if (size >= config_.quarters_capacity) return "quarters_full: build or upgrade quarters";
    return {};
}

}  // namespace deep_shelter::dwellers
