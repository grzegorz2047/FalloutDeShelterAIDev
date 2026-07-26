#include "gameplay/PlayableShelterSession.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include "core/FixedStepClock.hpp"
#include "persistence/SaveData.hpp"

namespace {

using namespace deep_shelter::gameplay;

constexpr std::uint32_t kSaveMagic = 0x33505344u;

void append_u16(std::vector<std::uint8_t>& output,
                std::uint16_t value) {
    output.push_back(static_cast<std::uint8_t>(value));
    output.push_back(static_cast<std::uint8_t>(value >> 8u));
}

void append_u32(std::vector<std::uint8_t>& output,
                std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) {
        output.push_back(static_cast<std::uint8_t>(value >> shift));
    }
}

void append_i32(std::vector<std::uint8_t>& output, int value) {
    append_u32(output, static_cast<std::uint32_t>(
                           static_cast<std::int32_t>(value)));
}

std::vector<std::uint8_t> make_v1_save() {
    std::vector<std::uint8_t> payload;
    append_i32(payload, 345);  // credits
    append_i32(payload, 21);   // power
    append_i32(payload, 22);   // food
    append_i32(payload, 23);   // water
    append_i32(payload, 2);    // rooms
    append_i32(payload, 1);    // selected room
    append_i32(payload, 1);    // assigned room
    for (int room = 0; room < kPlayableMaxRooms; ++room) {
        append_i32(payload, room == 1 ? 7 : 0);
    }
    for (int room = 0; room < kPlayableMaxRooms; ++room) {
        append_i32(payload, room == 1 ? 73 : 0);
    }
    assert(payload.size() == 76u);

    std::vector<std::uint8_t> save;
    append_u32(save, kSaveMagic);
    append_u16(save, 1u);
    append_u16(save, 0u);
    append_u32(save, static_cast<std::uint32_t>(payload.size()));
    append_u32(save, deep_shelter::persistence::crc32(
                         payload.data(), payload.size()));
    save.insert(save.end(), payload.begin(), payload.end());
    return save;
}

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

void fixed_grid_preview_and_costed_build_are_bounded() {
    static_assert(kPlayableGridColumns >= 8);
    static_assert(kPlayableGridFloors >= 6);
    static_assert(kPlayableResidentCount >= 3);
    PlayableShelterSession session;
    assert(valid_playable_state(session.state()));
    assert(session.state().rooms == 1);
    for (const auto& resident : session.state().residents) {
        assert(resident.active);
        assert(resident.state == PlayableResidentState::Roaming);
    }

    assert(session.set_build_cursor(0, 0));
    assert(session.preview_build().status ==
           BuildPreviewStatus::Occupied);
    assert(!session.set_build_cursor(kPlayableGridColumns, 0));

    assert(session.select_build_type(PlayableRoomType::Food));
    assert(session.set_build_cursor(4, 0));
    const auto preview = session.preview_build();
    assert(preview.valid());
    assert(preview.cost == 120);
    assert(session.confirm_build() == BuildResult::Built);
    assert(session.state().credits == 380);
    assert(session.state().rooms == 2);
    const int food_room = session.room_index_at(4, 0);
    assert(food_room >= 0);
    const auto& room =
        session.state().room_entries[static_cast<std::size_t>(food_room)];
    assert(room.active);
    assert(room.type == PlayableRoomType::Food);
    assert(room.column == 4 && room.floor == 0);

    PlayableShelterState poor_state;
    poor_state.credits = 40;
    PlayableShelterSession poor(poor_state);
    assert(poor.select_build_type(PlayableRoomType::Elevator));
    assert(poor.set_build_cursor(1, 0));
    assert(poor.preview_build().status ==
           BuildPreviewStatus::NotEnoughCredits);
    assert(poor.confirm_build() == BuildResult::NotEnoughCredits);
}

void transit_is_deterministic_and_production_starts_on_arrival() {
    PlayableShelterState rich_state;
    rich_state.credits = 2000;
    PlayableShelterSession disconnected(rich_state);
    assert(disconnected.select_build_type(PlayableRoomType::Food));
    assert(disconnected.set_build_cursor(4, 0));
    assert(disconnected.confirm_build() == BuildResult::Built);
    const int disconnected_room = disconnected.room_index_at(4, 0);
    assert(!disconnected.assign_resident_to_room(
        0u, disconnected_room));
    for (int step = 0; step < 180; ++step) {
        disconnected.fixed_step();
    }
    for (const auto& resident : disconnected.state().residents) {
        assert(resident.current_column == 0);
        assert(resident.current_floor == 0);
    }

    PlayableShelterSession session(rich_state);
    assert(session.select_build_type(PlayableRoomType::Workshop));
    for (int column = 1; column <= 3; ++column) {
        assert(session.set_build_cursor(column, 0));
        assert(session.confirm_build() == BuildResult::Built);
    }
    assert(session.select_build_type(PlayableRoomType::Food));
    assert(session.set_build_cursor(4, 0));
    assert(session.confirm_build() == BuildResult::Built);
    const int food_room = session.room_index_at(4, 0);
    assert(session.assign_resident_to_room(0u, food_room));
    assert(session.state().residents[0].state ==
           PlayableResidentState::Transit);

    for (int step = 0; step < 7; ++step) session.fixed_step();
    const auto midpoint = session.resident_position(0u);
    assert(midpoint.active);
    assert(midpoint.column > 0.0f && midpoint.column < 1.0f);
    assert(std::fabs(midpoint.floor) < 0.001f);
    assert(session.state().room_entries[
               static_cast<std::size_t>(food_room)].production_steps == 0);

    assert(valid_playable_state(session.state()));
    const auto encoded_midpoint = encode_playable_state(session.state());
    assert(!encoded_midpoint.empty());
    const auto decoded_midpoint =
        decode_playable_state(encoded_midpoint);
    assert(decoded_midpoint.status == PlayableSaveStatus::Ok);
    assert(!decoded_midpoint.migrated_from_v1);
    PlayableShelterSession resumed(decoded_midpoint.state);
    const auto resumed_position = resumed.resident_position(0u);
    assert(std::fabs(
               resumed_position.column - midpoint.column) < 0.001f);
    assert(resumed.state().residents[0].next_column ==
           session.state().residents[0].next_column);
    assert(resumed.state().residents[0].movement_ticks == 7);

    for (int step = 7; step < 59; ++step) resumed.fixed_step();
    assert(resumed.state().residents[0].state ==
           PlayableResidentState::Transit);
    assert(resumed.state().room_entries[
               static_cast<std::size_t>(food_room)].production_steps == 0);
    resumed.fixed_step();
    assert(resumed.state().residents[0].state ==
           PlayableResidentState::Working);
    assert(resumed.state().room_entries[
               static_cast<std::size_t>(food_room)].production_steps == 1);
    for (int step = 1; step < kPlayableProductionCycleSteps; ++step) {
        resumed.fixed_step();
    }
    assert(resumed.state().room_entries[
               static_cast<std::size_t>(food_room)].stored == 5);
    assert(resumed.collect_selected_room() == CollectResult::Collected);
    assert(resumed.state().food == 25);

    PlayableShelterSession at_sixty_hz;
    at_sixty_hz.assign_selected_room();
    deep_shelter::core::FixedStepClock clock(1.0 / 60.0, 15, 0.25);
    for (int frame = 0; frame < 60; ++frame) {
        clock.advance(1.0 / 60.0, [&](double) {
            at_sixty_hz.fixed_step();
        });
    }
    assert(at_sixty_hz.selected_progress() == 60);
}

void changing_floors_requires_a_connected_elevator_shaft() {
    PlayableShelterState rich_state;
    rich_state.credits = 1000;
    PlayableShelterSession session(rich_state);

    assert(session.select_build_type(PlayableRoomType::Water));
    assert(session.set_build_cursor(6, 1));
    assert(session.confirm_build() == BuildResult::Built);
    const int water_room = session.room_index_at(6, 1);
    assert(!session.assign_resident_to_room(0u, water_room));
    assert(session.state().residents[0].state ==
           PlayableResidentState::Roaming);

    assert(session.select_build_type(PlayableRoomType::Workshop));
    assert(session.set_build_cursor(1, 0));
    assert(session.confirm_build() == BuildResult::Built);
    assert(session.set_build_cursor(2, 0));
    assert(session.confirm_build() == BuildResult::Built);

    assert(session.select_build_type(PlayableRoomType::Elevator));
    assert(session.set_build_cursor(3, 0));
    assert(session.preview_build().valid());
    assert(session.confirm_build() == BuildResult::Built);
    assert(session.set_build_cursor(3, 1));
    assert(session.preview_build().valid());
    assert(session.confirm_build() == BuildResult::Built);
    assert(session.select_build_type(PlayableRoomType::Workshop));
    assert(session.set_build_cursor(4, 1));
    assert(session.confirm_build() == BuildResult::Built);
    assert(session.set_build_cursor(5, 1));
    assert(session.confirm_build() == BuildResult::Built);

    assert(session.assign_resident_to_room(0u, water_room));
    for (int step = 0; step < 44; ++step) session.fixed_step();
    assert(session.state().residents[0].current_floor == 0);
    session.fixed_step();
    assert(session.state().residents[0].current_column == 3);
    assert(session.state().residents[0].current_floor == 0);
    for (int step = 0; step < 14; ++step) session.fixed_step();
    const auto lift_position = session.resident_position(0u);
    assert(lift_position.floor > 0.0f &&
           lift_position.floor < 1.0f);
    session.fixed_step();
    assert(session.state().residents[0].current_column == 3);
    assert(session.state().residents[0].current_floor == 1);
    assert(session.state().room_entries[
               static_cast<std::size_t>(water_room)].production_steps == 0);
}


void playable_room_groups_upgrade_and_demolish_are_atomic() {
    PlayableShelterState rich_state;
    rich_state.credits = 3000;
    PlayableShelterSession session(rich_state);
    assert(session.select_build_type(PlayableRoomType::Food));
    for (int column = 1; column <= 3; ++column) {
        assert(session.set_build_cursor(column, 0));
        assert(session.confirm_build() == BuildResult::Built);
    }
    const int first = session.room_index_at(1, 0);
    const int middle = session.room_index_at(2, 0);
    const int last = session.room_index_at(3, 0);
    assert(first >= 0 && middle >= 0 && last >= 0);
    const auto group = session.state().room_entries[
        static_cast<std::size_t>(first)].group_id;
    assert(group != 0);
    assert(session.state().room_entries[static_cast<std::size_t>(middle)].group_id == group);
    assert(session.state().room_entries[static_cast<std::size_t>(last)].group_id == group);
    assert(session.selected_group_width() == 3);

    const int credits_before_upgrade = session.state().credits;
    const auto upgrade = session.preview_upgrade_selected();
    assert(upgrade.allowed());
    assert(upgrade.group_width == 3);
    assert(upgrade.credit_delta == -225);
    assert(session.confirm_upgrade_selected() == RoomLifecycleResult::Applied);
    assert(session.state().credits == credits_before_upgrade - 225);
    assert(session.state().room_entries[static_cast<std::size_t>(first)].level == 2);
    assert(session.state().room_entries[static_cast<std::size_t>(middle)].level == 2);
    assert(session.state().room_entries[static_cast<std::size_t>(last)].level == 2);

    const int credits_before_demolish = session.state().credits;
    const auto demolition = session.preview_demolish_selected();
    assert(demolition.allowed());
    assert(demolition.credit_delta == 120);
    assert(session.confirm_demolish_selected() == RoomLifecycleResult::Applied);
    assert(session.state().credits == credits_before_demolish + 120);
    assert(session.room_index_at(3, 0) < 0);
    assert(session.state().room_entries[static_cast<std::size_t>(first)].group_id ==
           session.state().room_entries[static_cast<std::size_t>(middle)].group_id);

    PlayableShelterState evacuation_state = session.state();
    PlayableShelterSession evacuation(evacuation_state);
    const int evacuation_room = evacuation.room_index_at(2, 0);
    assert(evacuation_room >= 0);
    assert(evacuation.assign_resident_to_room(0u, evacuation_room));
    const auto evacuation_preview = evacuation.preview_demolish_selected();
    assert(evacuation_preview.allowed());
    assert(evacuation_preview.residents_affected == 1);
    assert(evacuation.confirm_demolish_selected() ==
           RoomLifecycleResult::Applied);
    assert(evacuation.room_index_at(2, 0) < 0);
    const auto& evacuated = evacuation.state().residents[0];
    assert(evacuated.active);
    assert(evacuated.assigned_room == -1);
    assert(evacuated.state == PlayableResidentState::Roaming);
    assert(evacuation.room_index_at(evacuated.current_column,
                                    evacuated.current_floor) >= 0);
    assert(valid_playable_state(evacuation.state()));

    PlayableShelterState blocked_state = session.state();
    const int blocked_room = blocked_state.selected_room;
    blocked_state.room_entries[static_cast<std::size_t>(blocked_room)].stored = 1;
    PlayableShelterSession blocked(blocked_state);
    assert(blocked.preview_demolish_selected().result ==
           RoomLifecycleResult::UnsafeStoredResources);
    assert(blocked.confirm_demolish_selected() ==
           RoomLifecycleResult::UnsafeStoredResources);
    assert(blocked.state().room_entries[static_cast<std::size_t>(blocked_room)].active);
    assert(blocked.state().room_entries[static_cast<std::size_t>(blocked_room)].stored == 1);
    assert(valid_playable_state(blocked.state()));
}

void legacy_saves_migrate_to_v3_and_atomic_backup_recovers() {
    const auto migrated = decode_playable_state(make_v1_save());
    assert(migrated.status == PlayableSaveStatus::Ok);
    assert(migrated.migrated_from_v1);
    assert(migrated.state.rooms == 2);
    assert(migrated.state.room_entries[1].type ==
           PlayableRoomType::Food);
    assert(migrated.state.room_entries[1].stored == 7);
    assert(migrated.state.room_entries[1].production_steps == 73);
    assert(migrated.state.residents[0].state ==
           PlayableResidentState::Working);
    assert(migrated.state.residents[0].assigned_room == 1);
    const auto encoded_v3 = encode_playable_state(migrated.state);
    assert(encoded_v3.size() > make_v1_save().size());
    assert(encoded_v3[4] == 3u && encoded_v3[5] == 0u);

    PlayableShelterSession upgraded(migrated.state);
    assert(upgraded.preview_upgrade_selected().allowed());
    assert(upgraded.confirm_upgrade_selected() == RoomLifecycleResult::Applied);
    const auto upgraded_bytes = encode_playable_state(upgraded.state());
    const auto upgraded_roundtrip = decode_playable_state(upgraded_bytes);
    assert(upgraded_roundtrip.status == PlayableSaveStatus::Ok);
    assert(!upgraded_roundtrip.migrated_from_v1);
    assert(!upgraded_roundtrip.migrated_from_v2);
    assert(upgraded_roundtrip.state.next_segment_id == upgraded.state().next_segment_id);
    for (std::size_t index = 0; index < upgraded.state().room_entries.size(); ++index) {
        const auto& before = upgraded.state().room_entries[index];
        const auto& after = upgraded_roundtrip.state.room_entries[index];
        assert(after.segment_id == before.segment_id);
        assert(after.group_id == before.group_id);
        assert(after.level == before.level);
    }

    auto corrupt = encoded_v3;
    corrupt.back() ^= 0x7fu;
    assert(decode_playable_state(corrupt).status ==
           PlayableSaveStatus::Corrupt);
    PlayableShelterState invalid = migrated.state;
    invalid.room_entries[1].stored = kPlayableStorageCapacity + 1;
    assert(encode_playable_state(invalid).empty());

    const std::string path = make_temp_path();
    assert(access((path + ".sav").c_str(), F_OK) != 0);
    assert(access((path + ".bak").c_str(), F_OK) != 0);
    assert(save_playable_state(path, migrated.state) ==
           PlayableSaveStatus::Ok);
    assert(access((path + ".sav").c_str(), F_OK) == 0);
    assert(access((path + ".bak").c_str(), F_OK) != 0);
    PlayableShelterSession newer(migrated.state);
    for (int step = 0; step < 10; ++step) newer.fixed_step();
    assert(save_playable_state(path, newer.state()) ==
           PlayableSaveStatus::Ok);
    assert(access((path + ".bak").c_str(), F_OK) == 0);
    std::ifstream backup_file(path + ".bak", std::ios::binary);
    const std::vector<std::uint8_t> backup_bytes(
        (std::istreambuf_iterator<char>(backup_file)),
        std::istreambuf_iterator<char>());
    const auto decoded_backup = decode_playable_state(backup_bytes);
    assert(decoded_backup.status == PlayableSaveStatus::Ok);
    assert(decoded_backup.state.room_entries[1].production_steps == 73);

    assert(std::remove((path + ".sav").c_str()) == 0);
    assert(save_playable_state(path, newer.state()) ==
           PlayableSaveStatus::Ok);
    assert(access((path + ".sav").c_str(), F_OK) == 0);
    assert(access((path + ".bak").c_str(), F_OK) == 0);
    std::ifstream preserved_backup_file(path + ".bak", std::ios::binary);
    const std::vector<std::uint8_t> preserved_backup_bytes(
        (std::istreambuf_iterator<char>(preserved_backup_file)),
        std::istreambuf_iterator<char>());
    const auto preserved_backup = decode_playable_state(
        preserved_backup_bytes);
    assert(preserved_backup.status == PlayableSaveStatus::Ok);
    assert(preserved_backup.state.room_entries[1].production_steps == 73);

    std::ofstream broken(path + ".sav",
                         std::ios::binary | std::ios::trunc);
    broken << "broken";
    broken.close();
    const auto recovered = load_playable_state(path);
    assert(recovered.status == PlayableSaveStatus::Ok);
    assert(recovered.used_backup);
    assert(recovered.state.room_entries[1].production_steps == 73);
    remove_temp_path(path);
}

}  // namespace

int main() {
    fixed_grid_preview_and_costed_build_are_bounded();
    transit_is_deterministic_and_production_starts_on_arrival();
    changing_floors_requires_a_connected_elevator_shaft();
    playable_room_groups_upgrade_and_demolish_are_atomic();
    legacy_saves_migrate_to_v3_and_atomic_backup_recovers();
    return 0;
}
