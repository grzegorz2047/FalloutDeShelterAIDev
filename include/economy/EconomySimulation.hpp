#pragma once

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace deep_shelter::economy {

enum class ResourceKind { Power, Food, Water, Credits };

struct ResourcePool {
    std::int64_t amount = 0;
    std::int64_t capacity = 0;
};

struct ProductionRoom {
    std::uint64_t id = 0;
    ResourceKind output = ResourceKind::Power;
    std::int64_t units_per_cycle = 0;
    std::int64_t cycle_seconds = 1;
    std::int64_t progress_seconds = 0;
    int workers = 0;
    int power_priority = 0;
    bool requires_power = true;
    bool enabled = true;
    std::int64_t pending_units = 0;
};

struct EconomyConfig {
    std::int64_t power_use_per_hour = 0;
    std::int64_t food_use_per_hour = 0;
    std::int64_t water_use_per_hour = 0;
    std::int64_t max_step_seconds = 3600;
};

struct ResidentImpact {
    std::int64_t hunger_seconds = 0;
    std::int64_t thirst_seconds = 0;
    std::int64_t health_penalty = 0;
    std::int64_t contamination = 0;
};

struct EconomyEntry {
    std::int64_t timestamp = 0;
    std::string operation;
    ResourceKind resource = ResourceKind::Power;
    std::int64_t delta = 0;
    std::uint64_t source_id = 0;
};

struct EconomyForecast {
    std::int64_t net_power_per_hour = 0;
    std::int64_t net_food_per_hour = 0;
    std::int64_t net_water_per_hour = 0;
    std::int64_t seconds_to_next_shortage = -1;
    ResourceKind next_shortage = ResourceKind::Power;
};

class EconomySimulation {
public:
    EconomySimulation(EconomyConfig config, ResourcePool power, ResourcePool food,
                      ResourcePool water, ResourcePool credits);

    bool add_room(ProductionRoom room);
    bool collect(std::uint64_t room_id, std::uint64_t transaction_id, std::int64_t timestamp);
    bool apply_credit_delta(std::int64_t delta, std::uint64_t transaction_id, std::int64_t timestamp);
    void set_capacity(ResourceKind kind, std::int64_t capacity);
    void advance(std::int64_t seconds, std::int64_t start_timestamp);

    [[nodiscard]] ResourcePool pool(ResourceKind kind) const noexcept;
    [[nodiscard]] const std::vector<ProductionRoom>& rooms() const noexcept;
    [[nodiscard]] const std::vector<EconomyEntry>& journal() const noexcept;
    [[nodiscard]] const ResidentImpact& resident_impact() const noexcept;
    [[nodiscard]] EconomyForecast forecast() const noexcept;

private:
    ResourcePool& mutable_pool(ResourceKind kind) noexcept;
    const ResourcePool& const_pool(ResourceKind kind) const noexcept;
    void clamp_pool(ResourcePool& pool) noexcept;
    void consume(std::int64_t seconds, std::int64_t timestamp);
    void advance_rooms(std::int64_t seconds);
    void rebalance_power();
    std::int64_t production_rate_per_hour(ResourceKind kind) const noexcept;

    EconomyConfig config_;
    ResourcePool power_;
    ResourcePool food_;
    ResourcePool water_;
    ResourcePool credits_;
    std::vector<ProductionRoom> rooms_;
    std::vector<EconomyEntry> journal_;
    std::unordered_set<std::uint64_t> transactions_;
    ResidentImpact resident_impact_;
};

}  // namespace deep_shelter::economy
