#pragma once

#include "dwellers/Dweller.hpp"
#include "time/TrustedClock.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
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
    NotAdult,
    ClockRollback
};

struct PopulationRecord {
    LifeStage stage = LifeStage::Adult;
    std::uint64_t partner_id = 0;
    std::uint64_t mother_id = 0;
    std::uint64_t father_id = 0;
    std::int64_t pregnancy_due_ms = 0;
    std::int64_t adulthood_due_ms = 0;
    bool birth_completed = false;
};

struct PopulationConfig {
    int population_limit = 0;
    int quarters_capacity = 0;
    std::int64_t pregnancy_ms = 0;
    std::int64_t childhood_ms = 0;
};

struct PopulationEvent {
    std::int64_t timestamp_ms = 0;
    std::string type;
    std::uint64_t subject_id = 0;
    std::string detail;
};

struct PopulationSnapshot {
    static constexpr int kSchemaVersion = 1;

    int schema_version = kSchemaVersion;
    std::vector<std::pair<std::uint64_t, PopulationRecord>> records;
    std::int64_t last_trusted_now_ms = 0;
};

class PopulationLifecycleService {
public:
    PopulationLifecycleService(DwellerService& dwellers,
                               time::TrustedClock& clock,
                               PopulationConfig config);

    void migrate_missing_records();
    void set_capacities(int population_limit, int quarters_capacity) noexcept;
    PopulationError pair(std::uint64_t first, std::uint64_t second);
    PopulationError start_pregnancy(std::uint64_t mother, std::uint64_t father);
    void advance();

    [[nodiscard]] bool can_work(std::uint64_t dweller_id) const;
    [[nodiscard]] bool can_fight(std::uint64_t dweller_id) const;
    [[nodiscard]] bool can_travel(std::uint64_t dweller_id) const;
    [[nodiscard]] const PopulationRecord* record(std::uint64_t dweller_id) const;
    [[nodiscard]] const std::vector<PopulationEvent>& events() const noexcept;
    [[nodiscard]] std::string capacity_reason() const;
    [[nodiscard]] PopulationSnapshot snapshot() const;
    bool restore(const PopulationSnapshot& snapshot);

private:
    [[nodiscard]] bool close_relative(const Dweller& first, const Dweller& second) const;
    [[nodiscard]] bool is_ancestor(std::uint64_t ancestor,
                                   std::uint64_t descendant,
                                   int depth = 0) const;
    [[nodiscard]] std::size_t pending_births(std::uint64_t excluded_mother = 0) const;
    [[nodiscard]] PopulationError capacity_error(std::uint64_t excluded_mother = 0) const;
    [[nodiscard]] std::string generated_name(std::uint64_t mother,
                                             std::uint64_t father,
                                             std::uint64_t ordinal) const;
    [[nodiscard]] SpecialStats generated_special(std::uint64_t mother,
                                                 std::uint64_t father,
                                                 std::int64_t due_ms) const;
    [[nodiscard]] bool adult_and_alive(std::uint64_t dweller_id) const;

    DwellerService& dwellers_;
    time::TrustedClock& clock_;
    PopulationConfig config_;
    std::vector<std::pair<std::uint64_t, PopulationRecord>> records_;
    std::vector<PopulationEvent> events_;
    std::int64_t last_trusted_now_ms_ = 0;
};

}  // namespace deep_shelter::dwellers
