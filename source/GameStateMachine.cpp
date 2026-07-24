#include "core/GameStateMachine.hpp"

namespace deep_shelter::core {

GameState GameStateMachine::current() const noexcept { return current_; }
std::optional<GameState> GameStateMachine::previous() const noexcept { return previous_; }

bool GameStateMachine::can_transition(GameState next) const noexcept {
    if (current_ == next || current_ == GameState::Exiting) {
        return false;
    }
    if (next == GameState::Error || next == GameState::Exiting) {
        return true;
    }

    switch (current_) {
        case GameState::Boot:
            return next == GameState::MainMenu;
        case GameState::MainMenu:
            return next == GameState::Shelter || next == GameState::Settings;
        case GameState::Shelter:
            return next == GameState::MainMenu || next == GameState::Quest ||
                   next == GameState::Settings;
        case GameState::Quest:
            return next == GameState::Shelter;
        case GameState::Settings:
            return next == GameState::MainMenu || next == GameState::Shelter;
        case GameState::Error:
            return next == GameState::MainMenu;
        case GameState::Exiting:
            return false;
    }
    return false;
}

bool GameStateMachine::transition_to(GameState next) noexcept {
    if (!can_transition(next)) {
        return false;
    }
    previous_ = current_;
    current_ = next;
    return true;
}

}  // namespace deep_shelter::core
