#include "gameplay/PlayableShelterSession.hpp"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "core/FixedStepClock.hpp"

namespace {

using namespace deep_shelter::gameplay;

std::string make_temp_path() {
    char pattern[] = "/tmp/deep-shelter-playable-XXXXXX";
    char* directory = mkdtemp(pattern);
    assert(directory != nullptr);
    return std::string(directory) + "/slot0";
}

void remove_temp_path(const std::string& path) {
    std::remove((path + ".sav").c_str());
    std::remove((path + ".bak").c_str());
    std::remove((path + ".tmp").c_str());
    const std::size_t slash = path.find_last_of('/');
    assert(slash != std::string::npos);
    rmdir(path.substr(0, slash).c_str());
}

void playable_loop_is_room_aware() {
    PlayableShelterSession session;
    assert(session.primary_action() == PrimaryAction::Assign);
    assert(session.selected_stored() == 0);
    session.assign_selected_room();
    assert(session.primary_action() == PrimaryAction::Wait);
    for (int step = 0; step < 119; ++step) session.fixed_step();
    assert(session.selected_stored() == 0);
    session.fixed_step();
    assert(session.selected_stored() == 5);
    assert(session.primary_action() == PrimaryAction::Collect);
    assert(session.collect_selected_room() == CollectResult::Collected);
    assert(session.state().power == 25);
    assert(session.state().credits == 515);

    assert(session.build_room() == BuildResult::Built);
    assert(session.state().credits == 415);
    assert(session.state().selected_room == 1);
    session.assign_selected_room();
    for (int step = 0; step < 120; ++step) session.fixed_step();
    assert(session.collect_selected_room() == CollectResult::Collected);
    assert(session.state().food == 25);
    assert(session.build_room() == BuildResult::Built);
    session.assign_selected_room();
    for (int step = 0; step < 120; ++step) session.fixed_step();
    assert(session.collect_selected_room() == CollectResult::Collected);
    assert(session.state().water == 25);

    assert(session.build_room() == BuildResult::Built);
    session.assign_selected_room();
    const int credits_before_workshop = session.state().credits;
    for (int step = 0; step < 120; ++step) session.fixed_step();
    assert(session.collect_selected_room() == CollectResult::Collected);
    assert(session.state().credits == credits_before_workshop + 25);
    assert(session.select_previous_room());
    assert(!session.selected_has_worker());
    assert(session.select_next_room());
    assert(session.selected_has_worker());
}

void build_limit_and_fixed_step_partition_are_deterministic() {
    PlayableShelterSession session;
    for (int room = 1; room < kPlayableMaxRooms; ++room) {
        assert(session.build_room() == BuildResult::Built);
    }
    assert(session.build_room() == BuildResult::Full);
    assert(session.state().rooms == kPlayableMaxRooms);
    assert(session.state().credits == 0);

    PlayableShelterState low_credit_state;
    low_credit_state.credits = 50;
    PlayableShelterSession low_credit(low_credit_state);
    assert(low_credit.build_room() == BuildResult::NotEnoughCredits);

    PlayableShelterSession at_sixty_hz;
    at_sixty_hz.assign_selected_room();
    deep_shelter::core::FixedStepClock clock(1.0 / 60.0, 15, 0.25);
    for (int frame = 0; frame < 60; ++frame) {
        clock.advance(1.0 / 60.0, [&](double) {
            at_sixty_hz.fixed_step();
        });
    }
    assert(at_sixty_hz.selected_progress() == 60);
    assert(clock.advance(0.25, [&](double) {
               at_sixty_hz.fixed_step();
           }) == 15);
    assert(clock.dropped_steps() == 0);

    PlayableShelterSession at_thirty_hz;
    at_thirty_hz.assign_selected_room();
    deep_shelter::core::FixedStepClock thirty_fps(1.0 / 60.0, 15, 0.25);
    for (int frame = 0; frame < 30; ++frame) {
        thirty_fps.advance(1.0 / 30.0, [&](double) {
            at_thirty_hz.fixed_step();
        });
    }
    assert(at_thirty_hz.selected_progress() == 60);
}

void codec_and_atomic_backup_recover_state() {
    PlayableShelterSession session;
    assert(session.build_room() == BuildResult::Built);
    session.assign_selected_room();
    for (int step = 0; step < 73; ++step) session.fixed_step();

    const auto encoded = encode_playable_state(session.state());
    const auto decoded = decode_playable_state(encoded);
    assert(decoded.status == PlayableSaveStatus::Ok);
    assert(decoded.state.rooms == 2);
    assert(decoded.state.assigned_room == 1);
    assert(decoded.state.production_steps[1] == 73);

    auto corrupt = encoded;
    corrupt.back() ^= 0x7fu;
    assert(decode_playable_state(corrupt).status ==
           PlayableSaveStatus::Corrupt);
    PlayableShelterState invalid = session.state();
    invalid.stored[0] = kPlayableStorageCapacity + 1;
    assert(encode_playable_state(invalid).empty());

    const std::string path = make_temp_path();
    assert(save_playable_state(path, session.state()) ==
           PlayableSaveStatus::Ok);
    PlayableShelterSession newer(decoded.state);
    for (int step = 0; step < 10; ++step) newer.fixed_step();
    assert(save_playable_state(path, newer.state()) ==
           PlayableSaveStatus::Ok);

    std::ofstream broken(path + ".sav",
                         std::ios::binary | std::ios::trunc);
    broken << "broken";
    broken.close();
    const auto recovered = load_playable_state(path);
    assert(recovered.status == PlayableSaveStatus::Ok);
    assert(recovered.used_backup);
    assert(recovered.state.production_steps[1] == 73);
    remove_temp_path(path);
}

}  // namespace

int main() {
    playable_loop_is_room_aware();
    build_limit_and_fixed_step_partition_are_deterministic();
    codec_and_atomic_backup_recover_state();
    return 0;
}
