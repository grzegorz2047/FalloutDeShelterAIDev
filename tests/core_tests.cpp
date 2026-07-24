#include "core/CommandQueue.hpp"
#include "core/FixedStepClock.hpp"
#include "core/GameStateMachine.hpp"

#include <cassert>
#include <cmath>
#include <stdexcept>

using deep_shelter::core::Command;
using deep_shelter::core::CommandQueue;
using deep_shelter::core::CommandType;
using deep_shelter::core::FixedStepClock;
using deep_shelter::core::GameState;
using deep_shelter::core::GameStateMachine;

namespace {

void fixed_step_is_frame_partition_independent() {
    double first = 0.0;
    FixedStepClock clock_a(0.1, 10, 1.0);
    clock_a.advance(0.04, [&](double dt) { first += dt; });
    clock_a.advance(0.06, [&](double dt) { first += dt; });
    clock_a.advance(0.20, [&](double dt) { first += dt; });

    double second = 0.0;
    FixedStepClock clock_b(0.1, 10, 1.0);
    clock_b.advance(0.30, [&](double dt) { second += dt; });

    assert(clock_a.total_steps() == 3);
    assert(clock_b.total_steps() == 3);
    assert(std::abs(first - second) < 1e-9);
}

void long_frame_is_bounded() {
    FixedStepClock clock(0.01, 4, 0.25);
    const auto executed = clock.advance(10.0, [](double) {});
    assert(executed == 4);
    assert(clock.dropped_steps() == 21);
    assert(clock.accumulator_seconds() < clock.step_seconds());
}

void invalid_time_is_rejected() {
    FixedStepClock clock;
    bool rejected = false;
    try {
        clock.advance(-1.0, [](double) {});
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);
}

void state_transitions_are_explicit() {
    GameStateMachine machine;
    assert(machine.current() == GameState::Boot);
    assert(!machine.transition_to(GameState::Quest));
    assert(machine.transition_to(GameState::MainMenu));
    assert(machine.transition_to(GameState::Shelter));
    assert(machine.transition_to(GameState::Quest));
    assert(machine.transition_to(GameState::Error));
    assert(machine.transition_to(GameState::MainMenu));
    assert(machine.transition_to(GameState::Exiting));
    assert(!machine.transition_to(GameState::MainMenu));
}

void command_queue_is_fifo_and_bounded() {
    CommandQueue queue(2);
    assert(queue.push({CommandType::Confirm, 1, 2}));
    assert(queue.push({CommandType::Cancel, 3, 4}));
    assert(!queue.push({CommandType::Pause, 5, 6}));

    const auto first = queue.pop();
    const auto second = queue.pop();
    assert(first && first->type == CommandType::Confirm && first->subject_id == 1);
    assert(second && second->type == CommandType::Cancel && second->target_id == 4);
    assert(!queue.pop());
}

}  // namespace

int main() {
    fixed_step_is_frame_partition_independent();
    long_frame_is_bounded();
    invalid_time_is_rejected();
    state_transitions_are_explicit();
    command_queue_is_fifo_and_bounded();
    return 0;
}
