#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace deep_shelter::core {

class FixedStepClock {
public:
    using StepCallback = std::function<void(double)>;

    explicit FixedStepClock(double step_seconds = 1.0 / 30.0,
                            std::size_t max_steps_per_frame = 8,
                            double max_frame_seconds = 0.25);

    std::size_t advance(double frame_seconds, const StepCallback& callback);
    void reset() noexcept;

    [[nodiscard]] double step_seconds() const noexcept;
    [[nodiscard]] double accumulator_seconds() const noexcept;
    [[nodiscard]] std::uint64_t total_steps() const noexcept;
    [[nodiscard]] std::uint64_t dropped_steps() const noexcept;

private:
    double step_seconds_;
    std::size_t max_steps_per_frame_;
    double max_frame_seconds_;
    double accumulator_seconds_ = 0.0;
    std::uint64_t total_steps_ = 0;
    std::uint64_t dropped_steps_ = 0;
};

}  // namespace deep_shelter::core
