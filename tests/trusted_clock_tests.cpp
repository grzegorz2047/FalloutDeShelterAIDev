#include "time/TrustedClock.hpp"

#include <cassert>
#include <cstdint>
#include <limits>

using namespace deep_shelter::time;

int main() {
    std::int64_t wall = 1'000'000;
    std::uint64_t monotonic = 50;
    TrustedClock clock([&] { return wall; }, [&] { return monotonic; }, 10'000);

    ClockSnapshot previous;
    previous.system_unix_ms = 995'000;
    previous.trusted_unix_ms = 995'000;
    previous.trust = TrustLevel::ConsoleRtc;
    clock.begin_session(previous);

    auto offline = clock.calculate_offline_progress();
    assert(offline.elapsed_ms == 5'000);
    assert(!offline.clock_moved_backwards);

    monotonic = 2'050;
    assert(clock.session_elapsed_ms() == 2'000);
    assert(clock.trusted_now_ms() == 1'002'000);

    wall = 900'000;
    monotonic = 1;
    clock.begin_session(previous);
    offline = clock.calculate_offline_progress();
    assert(offline.elapsed_ms == 0);
    assert(offline.clock_moved_backwards);

    wall = 2'000'000;
    monotonic = 100;
    clock.begin_session(previous);
    offline = clock.calculate_offline_progress();
    assert(offline.elapsed_ms == 10'000);
    assert(offline.large_jump_limited);

    clock.verify_network_time(2'000'100, 500);
    offline = clock.calculate_offline_progress();
    assert(offline.network_verified);
    assert(offline.elapsed_ms == 1'005'100);
    assert(!offline.large_jump_limited);

    assert(complete_deadline_units(999, 1'000, 100) == 0);
    assert(complete_deadline_units(1'000, 1'000, 100) == 1);
    assert(complete_deadline_units(1'350, 1'000, 100) == 4);
    assert(complete_deadline_units(1'350, 1'000, 0) == 0);

    assert(saturating_add_ms(std::numeric_limits<std::int64_t>::max() - 1, 10) ==
           std::numeric_limits<std::int64_t>::max());
    return 0;
}
