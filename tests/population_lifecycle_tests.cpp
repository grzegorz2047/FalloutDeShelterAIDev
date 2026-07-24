#include "dwellers/PopulationLifecycle.hpp"

#include <cassert>

using namespace deep_shelter::dwellers;

int main() {
    DwellerService dwellers({{0, 100, 300}, 3});

    Dweller mother;
    mother.id = 1;
    mother.name = "Mara";
    assert(dwellers.add(mother));

    Dweller father;
    father.id = 2;
    father.name = "Ivo";
    assert(dwellers.add(father));

    PopulationLifecycleService population(dwellers, {4, 4, 10, 20});
    assert(population.pair(1, 2) == PopulationError::None);
    assert(population.start_pregnancy(1, 2, 100) == PopulationError::None);
    assert(population.start_pregnancy(1, 2, 101) == PopulationError::AlreadyPregnant);
    population.advance(109);
    assert(dwellers.all().size() == 2);
    population.advance(110);
    assert(dwellers.all().size() == 3);
    population.advance(110);
    assert(dwellers.all().size() == 3);

    const auto child_id = dwellers.find(1)->children.front();
    assert(dwellers.find(2)->children.front() == child_id);
    assert(!population.can_work(child_id));
    population.advance(129);
    assert(!population.can_work(child_id));
    population.advance(130);
    assert(population.can_work(child_id));

    assert(population.start_pregnancy(1, 2, 140) == PopulationError::None);
    population.advance(150);
    assert(dwellers.all().size() == 4);
    assert(population.start_pregnancy(1, 2, 151) == PopulationError::PopulationFull);
    assert(!population.capacity_reason().empty());

    Dweller sibling;
    sibling.name = "Sibling";
    sibling.parent_a = 1;
    sibling.parent_b = 2;
    const auto sibling_id = dwellers.add_with_unique_id(sibling);
    PopulationLifecycleService migrated(dwellers, {10, 10, 10, 20});
    assert(migrated.pair(child_id, sibling_id) == PopulationError::CloseRelative);
    assert(migrated.start_pregnancy(1, 2, 100) == PopulationError::None);
    assert(migrated.start_pregnancy(1, 2, 99) == PopulationError::ClockRollback);
    return 0;
}
