#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace deep_shelter::gameplay {

constexpr int kPlayableMaxRooms = 6;
constexpr int kPlayableProductionCycleSteps = 120;
constexpr int kPlayableStorageCapacity = 30;

enum class PrimaryAction {
    Assign,
    Wait,
    Collect,
};

enum class BuildResult {
    Built,
    NotEnoughCredits,
    Full,
};

enum class CollectResult {
    Collected,
    NothingStored,
};

struct PlayableShelterState {
    int credits = 500;
    int power = 20;
    int food = 20;
    int water = 20;
    int rooms = 1;
    int selected_room = 0;
    int assigned_room = -1;
    std::array<int, kPlayableMaxRooms> stored{};
    std::array<int, kPlayableMaxRooms> production_steps{};
};

class PlayableShelterSession {
public:
    PlayableShelterSession() = default;
    explicit PlayableShelterSession(PlayableShelterState state);

    [[nodiscard]] const PlayableShelterState& state() const noexcept;
    [[nodiscard]] bool select_previous_room() noexcept;
    [[nodiscard]] bool select_next_room() noexcept;
    [[nodiscard]] BuildResult build_room() noexcept;
    void assign_selected_room() noexcept;
    [[nodiscard]] CollectResult collect_selected_room() noexcept;
    void fixed_step() noexcept;

    [[nodiscard]] int selected_stored() const noexcept;
    [[nodiscard]] int selected_progress() const noexcept;
    [[nodiscard]] bool selected_has_worker() const noexcept;
    [[nodiscard]] PrimaryAction primary_action() const noexcept;
    [[nodiscard]] const char* next_step() const noexcept;

private:
    PlayableShelterState state_{};
};

enum class PlayableSaveStatus {
    Ok,
    Missing,
    Corrupt,
    IoError,
};

struct PlayableLoadResult {
    PlayableSaveStatus status = PlayableSaveStatus::Missing;
    PlayableShelterState state{};
    bool used_backup = false;
};

[[nodiscard]] bool valid_playable_state(
    const PlayableShelterState& state) noexcept;
[[nodiscard]] std::vector<std::uint8_t> encode_playable_state(
    const PlayableShelterState& state);
[[nodiscard]] PlayableLoadResult decode_playable_state(
    const std::vector<std::uint8_t>& bytes);
[[nodiscard]] PlayableSaveStatus save_playable_state(
    const std::string& path_without_suffix,
    const PlayableShelterState& state);
[[nodiscard]] PlayableLoadResult load_playable_state(
    const std::string& path_without_suffix);

}  // namespace deep_shelter::gameplay
