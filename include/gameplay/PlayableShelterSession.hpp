#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace deep_shelter::gameplay {

constexpr int kPlayableMaxRooms = 6;
constexpr int kPlayableGridColumns = 8;
constexpr int kPlayableGridFloors = 6;
constexpr int kPlayableRoomCapacity =
    kPlayableGridColumns * kPlayableGridFloors;
constexpr int kPlayableResidentCount = 3;
constexpr int kPlayableProductionCycleSteps = 120;
constexpr int kPlayableStorageCapacity = 30;
constexpr int kPlayableMovementStepTicks = 15;

enum class PrimaryAction {
    Assign,
    Wait,
    Collect,
};

enum class BuildResult {
    Built,
    NotEnoughCredits,
    Full,
    InvalidPlacement,
};

enum class CollectResult {
    Collected,
    NothingStored,
};

enum class RoomLifecycleResult {
    Applied,
    MissingRoom,
    NotEnoughCredits,
    MaximumLevel,
    UnsafeResidents,
    UnsafeStoredResources,
    UnsafeProduction,
    LastRoom,
};

struct PlayableRoomLifecyclePreview {
    RoomLifecycleResult result = RoomLifecycleResult::MissingRoom;
    int credit_delta = 0;
    int group_width = 0;
    int residents_affected = 0;
    int stored_units_affected = 0;
    int production_steps_affected = 0;

    [[nodiscard]] bool allowed() const noexcept {
        return result == RoomLifecycleResult::Applied;
    }
};

enum class PlayableRoomType {
    Power,
    Food,
    Water,
    Workshop,
    Living,
    Elevator,
};

enum class PlayableResidentState {
    Roaming,
    Transit,
    Working,
};

enum class BuildPreviewStatus {
    Valid,
    OutOfBounds,
    Occupied,
    NotEnoughCredits,
    Full,
};

struct PlayableBuildPreview {
    BuildPreviewStatus status = BuildPreviewStatus::OutOfBounds;
    int cost = 0;

    [[nodiscard]] bool valid() const noexcept {
        return status == BuildPreviewStatus::Valid;
    }
};

struct PlayableRoomEntry {
    bool active = false;
    PlayableRoomType type = PlayableRoomType::Power;
    int column = 0;
    int floor = 0;
    int stored = 0;
    int production_steps = 0;
    std::uint64_t segment_id = 0;
    std::uint64_t group_id = 0;
    int level = 1;
};

struct PlayableResidentEntry {
    bool active = false;
    int current_column = 0;
    int current_floor = 0;
    int next_column = 0;
    int next_floor = 0;
    int destination_column = 0;
    int destination_floor = 0;
    int assigned_room = -1;
    PlayableResidentState state = PlayableResidentState::Roaming;
    int movement_ticks = 0;
    int roaming_ticks = 0;
    int roaming_sequence = 0;
};

struct PlayableResidentPosition {
    bool active = false;
    float column = 0.0f;
    float floor = 0.0f;
};

struct PlayableShelterState {
    struct RawDefaultsTag {};

    PlayableShelterState();
    explicit PlayableShelterState(RawDefaultsTag) noexcept {}

    int credits = 500;
    int power = 20;
    int food = 20;
    int water = 20;
    int rooms = 1;
    int selected_room = 0;
    int assigned_room = -1;
    std::array<int, kPlayableMaxRooms> stored{};
    std::array<int, kPlayableMaxRooms> production_steps{};
    PlayableRoomType selected_build_type = PlayableRoomType::Power;
    int build_cursor_column = 1;
    int build_cursor_floor = 0;
    std::array<PlayableRoomEntry, kPlayableRoomCapacity> room_entries{};
    std::array<PlayableResidentEntry, kPlayableResidentCount> residents{};
    std::uint64_t next_segment_id = 1;
};

class PlayableShelterSession {
public:
    PlayableShelterSession();
    explicit PlayableShelterSession(PlayableShelterState state);

    [[nodiscard]] const PlayableShelterState& state() const noexcept;
    [[nodiscard]] bool select_previous_room() noexcept;
    [[nodiscard]] bool select_next_room() noexcept;
    [[nodiscard]] BuildResult build_room() noexcept;
    void assign_selected_room() noexcept;
    [[nodiscard]] CollectResult collect_selected_room() noexcept;
    void fixed_step() noexcept;

    [[nodiscard]] bool select_build_type(
        PlayableRoomType type) noexcept;
    [[nodiscard]] PlayableRoomType selected_build_type() const noexcept;
    [[nodiscard]] bool set_build_cursor(
        int column, int floor) noexcept;
    [[nodiscard]] bool move_build_cursor(
        int column_delta, int floor_delta) noexcept;
    [[nodiscard]] int build_cursor_column() const noexcept;
    [[nodiscard]] int build_cursor_floor() const noexcept;
    [[nodiscard]] PlayableBuildPreview preview_build() const noexcept;
    [[nodiscard]] BuildResult confirm_build() noexcept;
    [[nodiscard]] int room_index_at(
        int column, int floor) const noexcept;
    [[nodiscard]] bool assign_resident_to_room(
        std::size_t resident_index, int room_index) noexcept;
    [[nodiscard]] PlayableResidentPosition resident_position(
        std::size_t resident_index) const noexcept;
    [[nodiscard]] int selected_group_width() const noexcept;
    [[nodiscard]] PlayableRoomLifecyclePreview
    preview_upgrade_selected() const noexcept;
    [[nodiscard]] RoomLifecycleResult
    confirm_upgrade_selected() noexcept;
    [[nodiscard]] PlayableRoomLifecyclePreview
    preview_demolish_selected() const noexcept;
    [[nodiscard]] RoomLifecycleResult
    confirm_demolish_selected() noexcept;

    [[nodiscard]] int selected_stored() const noexcept;
    [[nodiscard]] int selected_progress() const noexcept;
    [[nodiscard]] bool selected_has_worker() const noexcept;
    [[nodiscard]] PrimaryAction primary_action() const noexcept;
    [[nodiscard]] const char* next_step() const noexcept;

private:
    void sync_legacy_view() noexcept;

    PlayableShelterState state_{PlayableShelterState::RawDefaultsTag{}};
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
    bool migrated_from_v1 = false;
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
