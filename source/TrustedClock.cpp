#include "time/TrustedClock.hpp"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <utility>

namespace deep_shelter::time {

std::int64_t saturating_add_ms(std::int64_t value, std::int64_t delta) noexcept {
    if (delta > 0 && value > std::numeric_limits<std::int64_t>::max() - delta) {
        return std::numeric_limits<std::int64_t>::max();
    }
    if (delta < 0 && value < std::numeric_limits<std::int64_t>::min() - delta) {
        return std::numeric_limits<std::int64_t>::min();
    }
    return value + delta;
}

std::int64_t complete_deadline_units(std::int64_t now_ms,
                                     std::int64_t deadline_ms,
                                     std::int64_t interval_ms) noexcept {
    if (interval_ms <= 0 || now_ms < deadline_ms) return 0;
    return 1 + (now_ms - deadline_ms) / interval_ms;
}

TrustedClock::TrustedClock(WallClockSource wall_clock,
                           MonotonicSource monotonic_clock,
                           std::int64_t max_unverified_offline_ms)
    : wall_clock_(std::move(wall_clock)),
      monotonic_clock_(std::move(monotonic_clock)),
      max_unverified_offline_ms_(std::max<std::int64_t>(0, max_unverified_offline_ms)) {}

void TrustedClock::begin_session(const ClockSnapshot& previous) {
    previous_ = previous;
    session_wall_start_ms_ = wall_clock_ ? wall_clock_() : 0;
    session_monotonic_start_ms_ = monotonic_clock_ ? monotonic_clock_() : 0;
    network_verified_ = false;
    verified_network_ms_ = 0;
}

std::int64_t TrustedClock::session_elapsed_ms() const {
    if (!monotonic_clock_) return 0;
    const auto now = monotonic_clock_();
    if (now < session_monotonic_start_ms_) return 0;
    const auto delta = now - session_monotonic_start_ms_;
    if (delta > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
        return std::numeric_limits<std::int64_t>::max();
    }
    return static_cast<std::int64_t>(delta);
}

std::int64_t TrustedClock::trusted_now_ms() const {
    const std::int64_t base = network_verified_ ? verified_network_ms_ : session_wall_start_ms_;
    return saturating_add_ms(base, session_elapsed_ms());
}

OfflineProgress TrustedClock::calculate_offline_progress() const {
    OfflineProgress result;
    const std::int64_t current = network_verified_ ? verified_network_ms_ : session_wall_start_ms_;
    const std::int64_t previous = previous_.trusted_unix_ms != 0
                                      ? previous_.trusted_unix_ms
                                      : previous_.system_unix_ms;
    if (previous <= 0 || current <= 0) {
        result.detail = "missing trustworthy RTC baseline";
        return result;
    }
    if (current < previous) {
        result.clock_moved_backwards = true;
        result.detail = "console clock moved backwards; offline progress set to zero";
        return result;
    }

    result.elapsed_ms = current - previous;
    result.network_verified = network_verified_;
    if (!network_verified_ && result.elapsed_ms > max_unverified_offline_ms_) {
        result.elapsed_ms = max_unverified_offline_ms_;
        result.large_jump_limited = true;
        result.detail = "large unverified clock jump was limited";
    } else {
        result.detail = network_verified_ ? "offline time verified by network" : "offline time from console RTC";
    }
    return result;
}

void TrustedClock::verify_network_time(std::int64_t network_unix_ms, std::int64_t tolerance_ms) {
    if (network_unix_ms <= 0 || tolerance_ms < 0) return;
    const auto difference = network_unix_ms >= session_wall_start_ms_
                                ? network_unix_ms - session_wall_start_ms_
                                : session_wall_start_ms_ - network_unix_ms;
    if (difference <= tolerance_ms || previous_.trust == TrustLevel::Untrusted) {
        network_verified_ = true;
        verified_network_ms_ = network_unix_ms;
    }
}

ClockSnapshot TrustedClock::snapshot() const {
    ClockSnapshot result;
    result.system_unix_ms = wall_clock_ ? wall_clock_() : session_wall_start_ms_;
    result.trusted_unix_ms = trusted_now_ms();
    result.trust = network_verified_ ? TrustLevel::NetworkVerified : TrustLevel::ConsoleRtc;
    return result;
}

}  // namespace deep_shelter::time
