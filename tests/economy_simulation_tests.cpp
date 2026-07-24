#include "economy/EconomySimulation.hpp"

#include <cassert>

using namespace deep_shelter::economy;

int main() {
    EconomyConfig config{10, 8, 6, 3600};
    EconomySimulation economy(config, {100, 200}, {100, 200}, {100, 200}, {1000, 2000});

    assert(economy.add_room({1, ResourceKind::Power, 20, 3600, 0, 1, 10, false, true, 0}));
    assert(economy.add_room({2, ResourceKind::Food, 12, 3600, 0, 1, 5, true, true, 0}));
    assert(economy.add_room({3, ResourceKind::Water, 10, 3600, 0, 1, 4, true, true, 0}));
    assert(!economy.add_room({3, ResourceKind::Water, 10, 3600, 0, 1, 4, true, true, 0}));

    economy.advance(86400, 1000);
    assert(economy.rooms()[0].pending_units == 480);
    assert(economy.rooms()[1].pending_units == 288);
    assert(economy.rooms()[2].pending_units == 240);
    assert(economy.pool(ResourceKind::Power).amount == 0);
    assert(economy.pool(ResourceKind::Food).amount == 0);
    assert(economy.pool(ResourceKind::Water).amount == 0);
    assert(economy.resident_impact().health_penalty > 0);
    assert(economy.resident_impact().contamination > 0);

    assert(economy.collect(1, 1001, 90000));
    assert(!economy.collect(1, 1001, 90000));
    assert(economy.pool(ResourceKind::Power).amount == 200);
    assert(economy.rooms()[0].pending_units == 280);

    assert(economy.apply_credit_delta(-250, 2001, 90001));
    assert(!economy.apply_credit_delta(-250, 2001, 90001));
    assert(economy.pool(ResourceKind::Credits).amount == 750);
    assert(!economy.apply_credit_delta(-1000, 2002, 90002));

    economy.set_capacity(ResourceKind::Power, 50);
    assert(economy.pool(ResourceKind::Power).amount == 50);

    const auto forecast = economy.forecast();
    assert(forecast.net_food_per_hour >= 0);
    assert(forecast.net_water_per_hour >= 0);

    EconomySimulation online(config, {200, 500}, {200, 500}, {200, 500}, {0, 1000});
    EconomySimulation offline(config, {200, 500}, {200, 500}, {200, 500}, {0, 1000});
    online.add_room({10, ResourceKind::Food, 12, 3600, 0, 1, 1, false, true, 0});
    offline.add_room({10, ResourceKind::Food, 12, 3600, 0, 1, 1, false, true, 0});
    for (int hour = 0; hour < 24 * 30; ++hour) online.advance(3600, hour * 3600);
    offline.advance(30LL * 24 * 3600, 0);
    assert(online.pool(ResourceKind::Food).amount == offline.pool(ResourceKind::Food).amount);
    assert(online.rooms()[0].pending_units == offline.rooms()[0].pending_units);

    EconomySimulation seven_days(config, {100, 100}, {100, 100}, {100, 100}, {0, 100});
    seven_days.advance(7LL * 24 * 3600, 0);
    assert(seven_days.pool(ResourceKind::Food).amount == 0);
    assert(!seven_days.journal().empty());
    return 0;
}
