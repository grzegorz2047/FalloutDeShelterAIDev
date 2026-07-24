#include "core/FixedStepClock.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace deep_shelter::core {

FixedStepClock::FixedStepClock(double step_seconds,
                               std::size_t max_steps_per_frame,
                               double max_frame_seconds)
    : step_seconds_(step_seconds),
      max_steps_per_frame_(max_steps_per_frame),
      max_frame_seconds_(max_frame_seconds) {
    if (!std::isfinite(step_seconds_) || step_seconds_ <= 0.0) {
        throw std::invalid_argument("step_seconds must be finite and positive");
    }
    if (max_steps_per_frame_ == 0) {
        throw std::invalid_argument("max_steps_per_frame must be positive");
    }
    if (!std::isfinite(max_frame_seconds_) || max_frame_seconds_ <= 0.0) {
        throw std::invalid_argument("max_frame_seconds must be finite and positive");
    }
}

std::size_t FixedStepClock::advance(double frame_seconds, const StepCallback& callback) {
    if (!callback) {
        throw std::invalid_argument("callback must be valid");
    }
    if (!std::isfinite(frame_seconds) || frame_seconds < 0.0) {
        throw std::invalid_argument("frame_seconds must be finite and non-negative");
    }

    accumulator_seconds_ += std::min(frame_seconds, max_frame_seconds_);
    std::size_t executed = 0;
    constexpr double epsilon = 1e-12;

    while (accumulator_seconds_ + epsilon >= step_seconds_ &&
           executed < max_steps_per_frame_) {
        callback(step_seconds_);
        accumulator_seconds_ -= step_seconds_;
        if (accumulator_seconds_ < 0.0 && accumulator_seconds_ > -epsilon) {
            accumulator_seconds_ = 0.0;
        }
        ++executed;
        ++total_steps_;
    }

    if (accumulator_seconds_ + epsilon >= step_seconds_) {
        const auto skipped = static_cast<std::uint64_t>(
            std::floor((accumulator_seconds_ + epsilon) / step_seconds_));
        accumulator_seconds_ -= static_cast<double>(skipped) * step_seconds_;
        dropped_steps_ += skipped;
    }

    return executed;
}

void FixedStepClock::reset() noexcept {
    accumulator_seconds_ = 0.0;
    total_steps_ = 0;
    dropped_steps_ = 0;
}

double FixedStepClock::step_seconds() const noexcept { return step_seconds_; }
double FixedStepClock::accumulator_seconds() const noexcept { return accumulator_seconds_; }
std::uint64_t FixedStepClock::total_steps() const noexcept { return total_steps_; }
std::uint64_t FixedStepClock::dropped_steps() const noexcept { return dropped_steps_; }

}  // namespace deep_shelter::core
