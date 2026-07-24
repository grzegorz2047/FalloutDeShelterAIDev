#include "dwellers/PopulationLifecycle.hpp"

#include <cassert>

using namespace deep_shelter;

int main() {
    std::int64_t wall_ms = 1000;
    std::uint64_t monotonic_ms = 0;
    time::TrustedClock clock([&]() { return wall_ms; }, [&]() { return monotonic_ms; });
    clock.begin_session({1000, 1000, time::TrustLevel::ConsoleRtc});

    dwellers::DwellerService dwellers({{0, 100, 300}, 3});
    dwellers::Dweller mother;
    mother.id = 1;
    mother.name = "Mara";
    assert(dwellers.add(mother));

    dwellers::Dweller father;
    father.id = 2;
    father.name = "Ivo";
    assert(dwellers.add(father));

    dwellers::PopulationLifecycleService population(
        dwellers, clock, {4, 4, 100, 200});
    assert(population.pair(1, 2) == dwellers::PopulationError::None);
    assert(population.start_pregnancy(1, 2) == dwellers::PopulationError::None);
    assert(population.start_pregnancy(1, 2) == dwellers::PopulationError::AlreadyPregnant);

    const auto saved = population.snapshot();
    dwellers::PopulationLifecycleService restored(
        dwellers, clock, {4, 4, 100, 200});
    assert(restored.restore(saved));
    assert(restored.record(1)->pregnancy_due_ms == 1100);

    monotonic_ms = 99;
    restored.advance();
    assert(dwellers.all().size() == 2);
    monotonic_ms = 100;
    restored.advance();
    assert(dwellers.all().size() == 3);
    restored.advance();
    assert(dwellers.all().size() == 3);

    const auto child_id = dwellers.find(1)->children.front();
    assert(dwellers.find(2)->children.front() == child_id);
    assert(!restored.can_work(child_id));
    assert(!restored.can_fight(child_id));
    assert(!restored.can_travel(child_id));

    monotonic_ms = 299;
    restored.advance();
    assert(!restored.can_work(child_id));
    monotonic_ms = 300;
    restored.advance();
    assert(restored.can_work(child_id));

    assert(restored.start_pregnancy(1, 2) == dwellers::PopulationError::None);
    dwellers::Dweller third;
    third.id = 4;
    third.name = "Third";
    assert(dwellers.add(third));
    restored.migrate_missing_records();
    assert(restored.start_pregnancy(4, 2) == dwellers::PopulationError::PopulationFull);
    assert(!restored.capacity_reason().empty());

    dwellers::Dweller sibling;
    sibling.name = "Sibling";
    sibling.parent_a = 1;
    sibling.parent_b = 2;
    const auto sibling_id = dwellers.add_with_unique_id(sibling);
    restored.migrate_missing_records();
    assert(restored.pair(child_id, sibling_id) == dwellers::PopulationError::CloseRelative);
    assert(restored.pair(1, child_id) == dwellers::PopulationError::CloseRelative);

    auto invalid = restored.snapshot();
    invalid.schema_version = 99;
    assert(!restored.restore(invalid));

    wall_ms = 500;
    monotonic_ms = 0;
    time::TrustedClock rollback_clock([&]() { return wall_ms; }, [&]() { return monotonic_ms; });
    rollback_clock.begin_session({1000, 1000, time::TrustLevel::ConsoleRtc});
    dwellers::PopulationLifecycleService rollback(
        dwellers, rollback_clock, {20, 20, 100, 200});
    auto rollback_snapshot = rollback.snapshot();
    rollback_snapshot.last_trusted_now_ms = 1000;
    assert(rollback.restore(rollback_snapshot));
    assert(rollback.start_pregnancy(1, 2) == dwellers::PopulationError::ClockRollback);

    return 0;
}
