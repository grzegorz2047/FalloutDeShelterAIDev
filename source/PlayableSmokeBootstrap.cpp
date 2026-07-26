#include "gameplay/PlayableShelterSession.hpp"

#if defined(__3DS__)
#include <cstdio>
#endif

namespace deep_shelter::gameplay {

PlayableShelterState::PlayableShelterState() {
#if defined(__3DS__)
    constexpr const char* kSmokeFlagPath =
        "sdmc:/DeepShelter3D_playable_smoke.flag";
    constexpr const char* kSmokeLogPath =
        "sdmc:/DeepShelter3D_playable_smoke.log";

    FILE* flag = std::fopen(kSmokeFlagPath, "rb");
    if (flag == nullptr) return;
    std::fclose(flag);
    std::remove(kSmokeFlagPath);
    std::remove(kSmokeLogPath);

    PlayableShelterSession smoke{
        PlayableShelterState{RawDefaultsTag{}}};
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
    const int food_room = smoke.room_index_at(3, 1);
    ok = ok && food_room >= 0 &&
         smoke.assign_resident_to_room(0u, food_room);
    if (ok) {
        for (int step = 0; step < 7; ++step) smoke.fixed_step();
        *this = smoke.state();
    }

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
    FILE* log = std::fopen(kSmokeLogPath, "wb");
    if (log != nullptr) {
        std::fprintf(
            log,
            "DEEP_SHELTER_PLAYABLE status=%s rooms=%d credits=%d "
            "elevators=%d power=%d food=%d selected=%d assigned=%d "
            "resident_state=%s movement_ticks=%d\n",
            ok ? "ok" : "failed",
            smoke.state().rooms,
            smoke.state().credits,
            elevator_count,
            power_count,
            food_count,
            smoke.state().selected_room,
            smoke.state().assigned_room,
            resident.state == PlayableResidentState::Transit
                ? "transit"
                : (resident.state == PlayableResidentState::Working
                       ? "working"
                       : "roaming"),
            resident.movement_ticks);
        std::fclose(log);
    }
#endif
}

}  // namespace deep_shelter::gameplay
