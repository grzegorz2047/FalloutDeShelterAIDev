#pragma once

#include <optional>

namespace deep_shelter::core {

enum class GameState {
    Boot,
    MainMenu,
    Shelter,
    Quest,
    Settings,
    Error,
    Exiting,
};

class GameStateMachine {
public:
    [[nodiscard]] GameState current() const noexcept;
    [[nodiscard]] std::optional<GameState> previous() const noexcept;
    [[nodiscard]] bool can_transition(GameState next) const noexcept;
    bool transition_to(GameState next) noexcept;

private:
    GameState current_ = GameState::Boot;
    std::optional<GameState> previous_;
};

}  // namespace deep_shelter::core
