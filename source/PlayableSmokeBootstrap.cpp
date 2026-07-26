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

bool seed_existing_save() noexcept {
    const std::vector<std::uint8_t> bytes = encode_playable_state(
        PlayableShelterState{PlayableShelterState::RawDefaultsTag{}});
    if (bytes.empty()) return false;
    const std::string path = std::string(kSmokeSavePath) + ".sav";
    FILE* file = std::fopen(path.c_str(), "wb");
    if (file == nullptr) return false;
    const bool written =
        std::fwrite(bytes.data(), 1u, bytes.size(), file) == bytes.size() &&
        std::fflush(file) == 0;
    const bool closed = std::fclose(file) == 0;
    return written && closed;
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

    const bool seeded = ok && seed_existing_save();
    const PlayableSaveStatus save_status =
        seeded ? save_playable_state(kSmokeSavePath, smoke.state())
               : PlayableSaveStatus::IoError;
    const bool saved = save_status == PlayableSaveStatus::Ok;
    const bool resume_armed = saved && create_flag(kSmokeResumeFlagPath);
    ok = ok && seeded && saved && resume_armed;
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
            "invalid_result=%s invalid_unchanged=%d seeded=%d saved=%d "
            "save_status=%d\n",
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
            seeded ? 1 : 0,
            saved ? 1 : 0,
            static_cast<int>(save_status));
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
    for (int step = 0; step < 90; ++step) resumed.fixed_step();
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
    const bool ok = restored && arrived && idle_moved && idle_unassigned;
    if (ok) output = resumed.state();

    FILE* log = std::fopen(kSmokePhaseTwoLogPath, "wb");
    if (log != nullptr) {
        std::fprintf(
            log,
            "DEEP_SHELTER_PLAYABLE phase=resume status=%s restored=%d "
            "rooms=%d credits=%d assigned=%d worker_state=%s "
            "worker_column=%d worker_floor=%d idle_moved=%d "
            "idle_assigned=%d idle_state=%s\n",
            ok ? "ok" : "failed",
            restored ? 1 : 0,
            resumed.state().rooms,
            resumed.state().credits,
            resumed.state().assigned_room,
            resident_state_label(worker.state),
            worker.current_column,
            worker.current_floor,
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
