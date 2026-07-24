#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace deep_shelter::time {

enum class TrustLevel {
    Untrusted,
    ConsoleRtc,
    NetworkVerified,
};

struct ClockSnapshot {
    std::int64_t system_unix_ms = 0;
    std::int64_t trusted_unix_ms = 0;
    TrustLevel trust = TrustLevel::Untrusted;
};

struct OfflineProgress {
    std::int64_t elapsed_ms = 0;
    bool clock_moved_backwards = false;
    bool large_jump_limited = false;
    bool network_verified = false;
    std::string detail;
};

class TrustedClock {
public:
    using WallClockSource = std::function<std::int64_t()>;
    using MonotonicSource = std::function<std::uint64_t()>;

    TrustedClock(WallClockSource wall_clock,
                 MonotonicSource monotonic_clock,
                 std::int64_t max_unverified_offline_ms = 7LL * 24 * 60 * 60 * 1000);

    void begin_session(const ClockSnapshot& previous);
    std::int64_t trusted_now_ms() const;
    std::int64_t session_elapsed_ms() const;
    OfflineProgress calculate_offline_progress() const;
    void verify_network_time(std::int64_t network_unix_ms, std::int64_t tolerance_ms = 5 * 60 * 1000);
    ClockSnapshot snapshot() const;

private:
    WallClockSource wall_clock_;
    MonotonicSource monotonic_clock_;
    std::int64_t max_unverified_offline_ms_;
    ClockSnapshot previous_{};
    std::int64_t session_wall_start_ms_ = 0;
    std::uint64_t session_monotonic_start_ms_ = 0;
    bool network_verified_ = false;
    std::int64_t verified_network_ms_ = 0;
};

std::int64_t saturating_add_ms(std::int64_t value, std::int64_t delta) noexcept;
std::int64_t complete_deadline_units(std::int64_t now_ms,
                                     std::int64_t deadline_ms,
                                     std::int64_t interval_ms) noexcept;

}  // namespace deep_shelter::time
