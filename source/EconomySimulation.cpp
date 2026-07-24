#include "economy/EconomySimulation.hpp"

#include <algorithm>
#include <limits>

namespace deep_shelter::economy {

namespace {
std::int64_t safe_add(std::int64_t value, std::int64_t delta) noexcept {
    if (delta > 0 && value > std::numeric_limits<std::int64_t>::max() - delta)
        return std::numeric_limits<std::int64_t>::max();
    if (delta < 0 && value < std::numeric_limits<std::int64_t>::min() - delta)
        return std::numeric_limits<std::int64_t>::min();
    return value + delta;
}
}

EconomySimulation::EconomySimulation(EconomyConfig config, ResourcePool power,
                                     ResourcePool food, ResourcePool water,
                                     ResourcePool credits)
    : config_(config), power_(power), food_(food), water_(water), credits_(credits) {
    config_.power_use_per_hour = std::max<std::int64_t>(0, config_.power_use_per_hour);
    config_.food_use_per_hour = std::max<std::int64_t>(0, config_.food_use_per_hour);
    config_.water_use_per_hour = std::max<std::int64_t>(0, config_.water_use_per_hour);
    config_.max_step_seconds = std::max<std::int64_t>(1, config_.max_step_seconds);
    clamp_pool(power_); clamp_pool(food_); clamp_pool(water_); clamp_pool(credits_);
}

bool EconomySimulation::add_room(ProductionRoom room) {
    if (room.id == 0 || room.units_per_cycle < 0 || room.cycle_seconds <= 0 || room.workers < 0)
        return false;
    for (const auto& existing : rooms_) if (existing.id == room.id) return false;
    room.progress_seconds = std::clamp<std::int64_t>(room.progress_seconds, 0, room.cycle_seconds - 1);
    room.pending_units = std::max<std::int64_t>(0, room.pending_units);
    rooms_.push_back(room);
    rebalance_power();
    return true;
}

ResourcePool& EconomySimulation::mutable_pool(ResourceKind kind) noexcept {
    if (kind == ResourceKind::Power) return power_;
    if (kind == ResourceKind::Food) return food_;
    if (kind == ResourceKind::Water) return water_;
    return credits_;
}
const ResourcePool& EconomySimulation::const_pool(ResourceKind kind) const noexcept {
    return const_cast<EconomySimulation*>(this)->mutable_pool(kind);
}
void EconomySimulation::clamp_pool(ResourcePool& pool) noexcept {
    pool.capacity = std::max<std::int64_t>(0, pool.capacity);
    pool.amount = std::clamp<std::int64_t>(pool.amount, 0, pool.capacity);
}

bool EconomySimulation::collect(std::uint64_t room_id, std::uint64_t transaction_id,
                                std::int64_t timestamp) {
    if (transaction_id == 0 || transactions_.count(transaction_id) != 0) return false;
    auto it = std::find_if(rooms_.begin(), rooms_.end(), [room_id](const auto& room){ return room.id == room_id; });
    if (it == rooms_.end() || it->pending_units <= 0) return false;
    auto& target = mutable_pool(it->output);
    const auto room_available = target.capacity - target.amount;
    const auto accepted = std::min(room_available, it->pending_units);
    if (accepted <= 0) return false;
    target.amount += accepted;
    it->pending_units -= accepted;
    transactions_.insert(transaction_id);
    journal_.push_back({timestamp, "collect", it->output, accepted, room_id});
    return true;
}

bool EconomySimulation::apply_credit_delta(std::int64_t delta, std::uint64_t transaction_id,
                                           std::int64_t timestamp) {
    if (transaction_id == 0 || transactions_.count(transaction_id) != 0) return false;
    if (delta < 0 && credits_.amount < -delta) return false;
    const auto before = credits_.amount;
    credits_.amount = safe_add(credits_.amount, delta);
    clamp_pool(credits_);
    const auto applied = credits_.amount - before;
    if (applied != delta) return false;
    transactions_.insert(transaction_id);
    journal_.push_back({timestamp, "credits", ResourceKind::Credits, delta, 0});
    return true;
}

void EconomySimulation::set_capacity(ResourceKind kind, std::int64_t capacity) {
    auto& target = mutable_pool(kind);
    const auto before = target.amount;
    target.capacity = std::max<std::int64_t>(0, capacity);
    clamp_pool(target);
    if (target.amount != before)
        journal_.push_back({0, "capacity_overflow_discard", kind, target.amount - before, 0});
}

void EconomySimulation::consume(std::int64_t seconds, std::int64_t timestamp) {
    auto consume_one = [&](ResourcePool& pool, std::int64_t rate, ResourceKind kind) {
        const auto required = (rate * seconds) / 3600;
        const auto taken = std::min(pool.amount, required);
        pool.amount -= taken;
        if (taken > 0) journal_.push_back({timestamp, "consume", kind, -taken, 0});
        return required - taken;
    };
    const auto power_missing = consume_one(power_, config_.power_use_per_hour, ResourceKind::Power);
    const auto food_missing = consume_one(food_, config_.food_use_per_hour, ResourceKind::Food);
    const auto water_missing = consume_one(water_, config_.water_use_per_hour, ResourceKind::Water);
    if (food_missing > 0) {
        resident_impact_.hunger_seconds += seconds;
        resident_impact_.health_penalty += std::max<std::int64_t>(1, food_missing);
    }
    if (water_missing > 0) {
        resident_impact_.thirst_seconds += seconds;
        resident_impact_.contamination += std::max<std::int64_t>(1, water_missing);
    }
    if (power_missing > 0) rebalance_power();
}

void EconomySimulation::advance_rooms(std::int64_t seconds) {
    for (auto& room : rooms_) {
        if (!room.enabled || room.workers == 0) continue;
        const auto worker_multiplier = std::max(1, room.workers);
        const auto scaled = seconds * worker_multiplier;
        room.progress_seconds += scaled;
        const auto cycles = room.progress_seconds / room.cycle_seconds;
        room.progress_seconds %= room.cycle_seconds;
        room.pending_units = safe_add(room.pending_units, cycles * room.units_per_cycle);
    }
}

void EconomySimulation::rebalance_power() {
    std::vector<ProductionRoom*> powered;
    for (auto& room : rooms_) if (room.requires_power) powered.push_back(&room);
    std::sort(powered.begin(), powered.end(), [](const auto* left, const auto* right) {
        if (left->power_priority != right->power_priority)
            return left->power_priority > right->power_priority;
        return left->id < right->id;
    });
    const bool shortage = power_.amount == 0;
    for (std::size_t index = 0; index < powered.size(); ++index)
        powered[index]->enabled = !shortage || index == 0;
}

void EconomySimulation::advance(std::int64_t seconds, std::int64_t start_timestamp) {
    if (seconds <= 0) return;
    std::int64_t elapsed = 0;
    while (elapsed < seconds) {
        const auto step = std::min(config_.max_step_seconds, seconds - elapsed);
        advance_rooms(step);
        consume(step, start_timestamp + elapsed + step);
        elapsed += step;
    }
}

ResourcePool EconomySimulation::pool(ResourceKind kind) const noexcept { return const_pool(kind); }
const std::vector<ProductionRoom>& EconomySimulation::rooms() const noexcept { return rooms_; }
const std::vector<EconomyEntry>& EconomySimulation::journal() const noexcept { return journal_; }
const ResidentImpact& EconomySimulation::resident_impact() const noexcept { return resident_impact_; }

std::int64_t EconomySimulation::production_rate_per_hour(ResourceKind kind) const noexcept {
    std::int64_t rate = 0;
    for (const auto& room : rooms_) {
        if (room.output != kind || !room.enabled || room.workers == 0 || room.cycle_seconds <= 0) continue;
        rate = safe_add(rate, room.units_per_cycle * room.workers * 3600 / room.cycle_seconds);
    }
    return rate;
}

EconomyForecast EconomySimulation::forecast() const noexcept {
    EconomyForecast value;
    value.net_power_per_hour = production_rate_per_hour(ResourceKind::Power) - config_.power_use_per_hour;
    value.net_food_per_hour = production_rate_per_hour(ResourceKind::Food) - config_.food_use_per_hour;
    value.net_water_per_hour = production_rate_per_hour(ResourceKind::Water) - config_.water_use_per_hour;
    auto consider = [&](ResourceKind kind, const ResourcePool& pool, std::int64_t net) {
        if (net >= 0) return;
        const auto seconds = pool.amount * 3600 / -net;
        if (value.seconds_to_next_shortage < 0 || seconds < value.seconds_to_next_shortage) {
            value.seconds_to_next_shortage = seconds;
            value.next_shortage = kind;
        }
    };
    consider(ResourceKind::Power, power_, value.net_power_per_hour);
    consider(ResourceKind::Food, food_, value.net_food_per_hour);
    consider(ResourceKind::Water, water_, value.net_water_per_hour);
    return value;
}

}  // namespace deep_shelter::economy
