#include "gameplay/PlayableShelterSession.hpp"

#if defined(__3DS__)
#include <cmath>
#include <cstdio>
#endif

namespace deep_shelter::gameplay {
namespace {

#if defined(__3DS__)
constexpr const char* kSmokeFlagPath =
    "sdmc:/DeepShelter3D_playable_smoke.flag";
constexpr const char* kSmokeResumeFlagPath =
    "sdmc:/DeepShelter3D_playable_smoke_resume.flag";
constexpr const char* kSmokePhaseOneLogPath =
    "sdmc:/DeepShelter3D_playable_smoke.log";
constexpr const char* kSmokePhaseTwoLogPath =
    "sdmc:/DeepShelter3D_playable_smoke_resume.log";
constexpr const char* kSmokeSavePath = "sdmc:/DS3D_smoke";

bool consume_flag(const char* path) noexcept {
    FILE* flag = std::fopen(path, "rb");
    if (flag == nullptr) return false;
    std::fclose(flag);
    std::remove(path);
    return true;
}

bool create_flag(const char* path) noexcept {
    FILE* flag = std::fopen(path, "wb");
    if (flag == nullptr) return false;
    return std::fclose(flag) == 0;
}

const char* resident_state_label(PlayableResidentState state) noexcept {
    switch (state) {
        case PlayableResidentState::Roaming: return "roaming";
        case PlayableResidentState::Transit: return "transit";
        case PlayableResidentState::Working: return "working";
    }
    return "unknown";
}

void run_phase_one(PlayableShelterState& output) noexcept {
    std::remove(kSmokePhaseOneLogPath);
    std::remove(kSmokePhaseTwoLogPath);
    std::remove((std::string(kSmokeSavePath) + ".sav").c_str());
    std::remove((std::string(kSmokeSavePath) + ".bak").c_str());
    std::remove((std::string(kSmokeSavePath) + ".tmp").c_str());

    PlayableShelterSession smoke{
        PlayableShelterState{PlayableShelterState::RawDefaultsTag{}}};
    const auto build = [&](PlayableRoomType type,
                           int column,
                           int floor) noexcept {
        return smoke.select_build_type(type) &&
               smoke.set_build_cursor(column, floor) &&
               smoke.confirm_build() == BuildResult::Built;
    };

    bool ok = build(PlayableRoomType::Elevator, 1, 0) &&
              build(PlayableRoomType::Elevator, 1, 1) &&
              build(PlayableRoomType::Power, 2, 1) &&
              build(PlayableRoomType::Food, 3, 1);

    const int credits_before_invalid = smoke.state().credits;
    const int rooms_before_invalid = smoke.state().rooms;
    const bool selected_invalid_cell =
        smoke.select_build_type(PlayableRoomType::Water) &&
        smoke.set_build_cursor(0, 0);
    const BuildResult invalid_result =
        selected_invalid_cell ? smoke.confirm_build()
                              : BuildResult::Built;
    const bool invalid_unchanged =
        invalid_result == BuildResult::InvalidPlacement &&
        smoke.state().credits == credits_before_invalid &&
        smoke.state().rooms == rooms_before_invalid;

    const int food_room = smoke.room_index_at(3, 1);
    ok = ok && invalid_unchanged && food_room >= 0 &&
         smoke.assign_resident_to_room(0u, food_room);
    if (ok) {
        for (int step = 0; step < 7; ++step) smoke.fixed_step();
    }

    const PlayableSaveStatus first_save_status =
        ok ? save_playable_state(kSmokeSavePath, smoke.state())
           : PlayableSaveStatus::IoError;
    const bool first_saved =
        first_save_status == PlayableSaveStatus::Ok;
    const std::string backup_path =
        std::string(kSmokeSavePath) + ".bak";
    FILE* unexpected_backup = std::fopen(backup_path.c_str(), "rb");
    const bool backup_absent_after_first = unexpected_backup == nullptr;
    if (unexpected_backup != nullptr) std::fclose(unexpected_backup);
    const PlayableSaveStatus second_save_status =
        first_saved ? save_playable_state(kSmokeSavePath, smoke.state())
                    : PlayableSaveStatus::IoError;
    FILE* rotated_backup = std::fopen(backup_path.c_str(), "rb");
    const bool backup_rotated = rotated_backup != nullptr;
    if (rotated_backup != nullptr) std::fclose(rotated_backup);
    const bool saved =
        second_save_status == PlayableSaveStatus::Ok;
    const bool resume_armed = saved && create_flag(kSmokeResumeFlagPath);
    ok = ok && first_saved && backup_absent_after_first &&
         backup_rotated && saved && resume_armed;
    if (ok) output = smoke.state();

    int elevator_count = 0;
    int power_count = 0;
    int food_count = 0;
    for (const auto& room : smoke.state().room_entries) {
        if (!room.active) continue;
        if (room.type == PlayableRoomType::Elevator) ++elevator_count;
        if (room.type == PlayableRoomType::Power) ++power_count;
        if (room.type == PlayableRoomType::Food) ++food_count;
    }
    const auto& resident = smoke.state().residents[0];
    FILE* log = std::fopen(kSmokePhaseOneLogPath, "wb");
    if (log != nullptr) {
        std::fprintf(
            log,
            "DEEP_SHELTER_PLAYABLE phase=build status=%s rooms=%d "
            "credits=%d elevators=%d power=%d food=%d selected=%d "
            "assigned=%d resident_state=%s movement_ticks=%d "
            "invalid_result=%s invalid_unchanged=%d first_saved=%d "
            "backup_absent_after_first=%d backup_rotated=%d saved=%d "
            "first_save_status=%d save_status=%d\n",
            ok ? "ok" : "failed",
            smoke.state().rooms,
            smoke.state().credits,
            elevator_count,
            power_count,
            food_count,
            smoke.state().selected_room,
            smoke.state().assigned_room,
            resident_state_label(resident.state),
            resident.movement_ticks,
            invalid_result == BuildResult::InvalidPlacement
                ? "invalid-placement"
                : "unexpected",
            invalid_unchanged ? 1 : 0,
            first_saved ? 1 : 0,
            backup_absent_after_first ? 1 : 0,
            backup_rotated ? 1 : 0,
            saved ? 1 : 0,
            static_cast<int>(first_save_status),
            static_cast<int>(second_save_status));
        std::fclose(log);
    }
}

void run_phase_two(PlayableShelterState& output) noexcept {
    std::remove(kSmokePhaseTwoLogPath);
    const PlayableLoadResult loaded = load_playable_state(kSmokeSavePath);
    const bool restored =
        loaded.status == PlayableSaveStatus::Ok &&
        loaded.state.rooms == 5 &&
        loaded.state.credits == 180 &&
        loaded.state.selected_room == 4 &&
        loaded.state.assigned_room == 4 &&
        loaded.state.residents[0].state == PlayableResidentState::Transit &&
        loaded.state.residents[0].movement_ticks == 7;

    PlayableShelterSession resumed(
        restored ? loaded.state
                 : PlayableShelterState{
                       PlayableShelterState::RawDefaultsTag{}});
    const PlayableResidentPosition idle_before =
        resumed.resident_position(1u);
    bool entered_lower_elevator = false;
    bool used_vertical_elevator = false;
    bool entered_upper_elevator = false;
    for (int step = 0; step < 90; ++step) {
        resumed.fixed_step();
        const auto& moving = resumed.state().residents[0];
        entered_lower_elevator = entered_lower_elevator ||
            (moving.current_column == 1 && moving.current_floor == 0);
        used_vertical_elevator = used_vertical_elevator ||
            (moving.state == PlayableResidentState::Transit &&
             moving.current_column == 1 && moving.next_column == 1 &&
             moving.current_floor == 0 && moving.next_floor == 1);
        entered_upper_elevator = entered_upper_elevator ||
            (moving.current_column == 1 && moving.current_floor == 1);
    }
    const PlayableResidentPosition idle_after =
        resumed.resident_position(1u);

    const auto& worker = resumed.state().residents[0];
    const auto& idle = resumed.state().residents[1];
    const bool arrived =
        worker.state == PlayableResidentState::Working &&
        worker.assigned_room == 4 &&
        worker.current_column == 3 &&
        worker.current_floor == 1;
    const bool idle_moved =
        idle_before.active && idle_after.active &&
        (std::fabs(idle_after.column - idle_before.column) > 0.001f ||
         std::fabs(idle_after.floor - idle_before.floor) > 0.001f);
    const bool idle_unassigned =
        idle.assigned_room == -1 &&
        idle.state == PlayableResidentState::Roaming;

    bool overview_selected = true;
    while (resumed.state().selected_room > 1) {
        if (!resumed.select_previous_room()) {
            overview_selected = false;
            break;
        }
    }
    overview_selected = overview_selected &&
        resumed.state().selected_room == 1;

    const bool ok = restored && arrived && idle_moved && idle_unassigned &&
                    entered_lower_elevator && used_vertical_elevator &&
                    entered_upper_elevator && overview_selected;
    if (ok) output = resumed.state();

    FILE* log = std::fopen(kSmokePhaseTwoLogPath, "wb");
    if (log != nullptr) {
        std::fprintf(
            log,
            "DEEP_SHELTER_PLAYABLE phase=resume status=%s restored=%d "
            "rooms=%d credits=%d selected=%d assigned=%d worker_state=%s "
            "worker_column=%d worker_floor=%d elevator_lower=%d "
            "elevator_vertical=%d elevator_upper=%d idle_moved=%d "
            "idle_assigned=%d idle_state=%s\n",
            ok ? "ok" : "failed",
            restored ? 1 : 0,
            resumed.state().rooms,
            resumed.state().credits,
            resumed.state().selected_room,
            resumed.state().assigned_room,
            resident_state_label(worker.state),
            worker.current_column,
            worker.current_floor,
            entered_lower_elevator ? 1 : 0,
            used_vertical_elevator ? 1 : 0,
            entered_upper_elevator ? 1 : 0,
            idle_moved ? 1 : 0,
            idle.assigned_room,
            resident_state_label(idle.state));
        std::fclose(log);
    }
}
#endif

}  // namespace

PlayableShelterState::PlayableShelterState() {
#if defined(__3DS__)
    if (consume_flag(kSmokeResumeFlagPath)) {
        run_phase_two(*this);
    } else if (consume_flag(kSmokeFlagPath)) {
        run_phase_one(*this);
    }
#endif
}

}  // namespace deep_shelter::gameplay
