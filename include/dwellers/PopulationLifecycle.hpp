#pragma once

#include "dwellers/Dweller.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace deep_shelter::dwellers {

enum class LifeStage { Adult, Pregnant, Child };
enum class PopulationError {
    None,
    UnknownDweller,
    InvalidPair,
    CloseRelative,
    PopulationFull,
    QuartersFull,
    AlreadyPregnant,
    NotPregnant,
    ClockRollback
};

struct PopulationRecord {
    LifeStage stage = LifeStage::Adult;
    std::uint64_t partner_id = 0;
    std::uint64_t mother_id = 0;
    std::uint64_t father_id = 0;
    std::int64_t pregnancy_due = 0;
    std::int64_t adulthood_due = 0;
    bool birth_completed = false;
};

struct PopulationConfig {
    int population_limit = 0;
    int quarters_capacity = 0;
    std::int64_t pregnancy_seconds = 0;
    std::int64_t childhood_seconds = 0;
};

struct PopulationEvent {
    std::int64_t timestamp = 0;
    std::string type;
    std::uint64_t subject_id = 0;
    std::string detail;
};

class PopulationLifecycleService {
public:
    PopulationLifecycleService(DwellerService& dwellers, PopulationConfig config);

    void migrate_missing_records();
    PopulationError pair(std::uint64_t first, std::uint64_t second);
    PopulationError start_pregnancy(std::uint64_t mother, std::uint64_t father,
                                    std::int64_t trusted_now);
    void advance(std::int64_t trusted_now);

    [[nodiscard]] bool can_work(std::uint64_t dweller_id) const;
    [[nodiscard]] const PopulationRecord* record(std::uint64_t dweller_id) const;
    [[nodiscard]] const std::vector<PopulationEvent>& events() const noexcept;
    [[nodiscard]] std::string capacity_reason() const;

private:
    [[nodiscard]] bool close_relative(const Dweller& first, const Dweller& second) const;
    [[nodiscard]] bool has_capacity_for_birth() const;
    [[nodiscard]] std::string generated_name(std::uint64_t mother, std::uint64_t father) const;

    DwellerService& dwellers_;
    PopulationConfig config_;
    std::unordered_map<std::uint64_t, PopulationRecord> records_;
    std::unordered_set<std::uint64_t> completed_births_;
    std::vector<PopulationEvent> events_;
    std::int64_t last_trusted_now_ = 0;
};

}  // namespace deep_shelter::dwellers
