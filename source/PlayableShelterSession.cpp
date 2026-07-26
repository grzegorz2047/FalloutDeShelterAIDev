#include "gameplay/PlayableShelterSession.hpp"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>

#include "persistence/SaveData.hpp"

namespace deep_shelter::gameplay {
namespace {

constexpr std::uint32_t kPlayableSaveMagic = 0x33505344u;  // DSP3
constexpr std::uint16_t kPlayableSaveVersionV1 = 1u;
constexpr std::uint16_t kPlayableSaveVersionV2 = 2u;
constexpr std::uint16_t kPlayableSaveVersionV3 = 3u;
constexpr std::size_t kPlayableSaveHeaderSize = 16u;
constexpr std::size_t kPlayableSavePayloadSizeV1 = 76u;
constexpr std::size_t kPlayableSaveScalarCountV2 = 10u;
constexpr std::size_t kPlayableRoomFieldCountV2 = 6u;
constexpr std::size_t kPlayableResidentFieldCountV2 = 12u;
constexpr std::size_t kPlayableSavePayloadSizeV2 =
    (kPlayableSaveScalarCountV2 +
     static_cast<std::size_t>(kPlayableMaxRooms) * 2u +
     static_cast<std::size_t>(kPlayableRoomCapacity) *
         kPlayableRoomFieldCountV2 +
     static_cast<std::size_t>(kPlayableResidentCount) *
         kPlayableResidentFieldCountV2) *
    sizeof(std::int32_t);
constexpr std::size_t kPlayableSavePayloadSizeV3 =
    kPlayableSavePayloadSizeV2 + sizeof(std::uint64_t) +
    static_cast<std::size_t>(kPlayableRoomCapacity) *
        (sizeof(std::uint64_t) * 2u + sizeof(std::int32_t));
constexpr std::size_t kMaximumPlayableSaveSize = 4096u;
constexpr int kRoamingPauseTicks = 60;

bool valid_room_type(PlayableRoomType type) noexcept {
    const int value = static_cast<int>(type);
    return value >= static_cast<int>(PlayableRoomType::Power) &&
           value <= static_cast<int>(PlayableRoomType::Elevator);
}

bool valid_resident_state(PlayableResidentState state) noexcept {
    const int value = static_cast<int>(state);
    return value >= static_cast<int>(PlayableResidentState::Roaming) &&
           value <= static_cast<int>(PlayableResidentState::Working);
}

bool in_grid(int column, int floor) noexcept {
    return column >= 0 && column < kPlayableGridColumns &&
           floor >= 0 && floor < kPlayableGridFloors;
}

int grid_index(int column, int floor) noexcept {
    return floor * kPlayableGridColumns + column;
}

int room_cost(PlayableRoomType type) noexcept {
    switch (type) {
        case PlayableRoomType::Power:
            return 100;
        case PlayableRoomType::Food:
        case PlayableRoomType::Water:
            return 120;
        case PlayableRoomType::Workshop:
            return 140;
        case PlayableRoomType::Living:
            return 150;
        case PlayableRoomType::Elevator:
            return 50;
    }
    return 0;
}

bool room_produces(PlayableRoomType type) noexcept {
    return type != PlayableRoomType::Living &&
           type != PlayableRoomType::Elevator;
}

int room_index_at_state(const PlayableShelterState& state,
                        int column,
                        int floor) noexcept;

constexpr int kPlayableMaximumRoomLevel = 3;
constexpr int kPlayableMaximumGroupWidth = 3;

int upgrade_cost(const PlayableRoomEntry& room, int group_width) noexcept {
    return 75 * group_width * room.level;
}

int demolition_refund(const PlayableRoomEntry& room) noexcept {
    return room_cost(room.type) * room.level / 2;
}

void normalize_room_groups(PlayableShelterState& state) noexcept {
    std::uint64_t highest_id = 0;
    for (const auto& room : state.room_entries) {
        if (room.active) highest_id = std::max(highest_id, room.segment_id);
    }
    state.next_segment_id = std::max(state.next_segment_id, highest_id + 1);
    for (int floor = 0; floor < kPlayableGridFloors; ++floor) {
        for (int column = 0; column < kPlayableGridColumns; ++column) {
            const int index = room_index_at_state(state, column, floor);
            if (index < 0) continue;
            auto& room = state.room_entries[static_cast<std::size_t>(index)];
            if (room.segment_id == 0) room.segment_id = state.next_segment_id++;
            room.group_id = room.segment_id;
            room.level = std::max(1, std::min(room.level, kPlayableMaximumRoomLevel));
        }
    }

    for (int floor = 0; floor < kPlayableGridFloors; ++floor) {
        int column = 0;
        while (column < kPlayableGridColumns) {
            const int first_index = room_index_at_state(state, column, floor);
            if (first_index < 0) {
                ++column;
                continue;
            }
            auto& first = state.room_entries[static_cast<std::size_t>(first_index)];
            std::uint64_t stable_group = first.segment_id;
            int width = 1;
            while (width < kPlayableMaximumGroupWidth &&
                   column + width < kPlayableGridColumns) {
                const int candidate_index = room_index_at_state(
                    state, column + width, floor);
                if (candidate_index < 0) break;
                const auto& candidate = state.room_entries[
                    static_cast<std::size_t>(candidate_index)];
                if (candidate.type != first.type || candidate.level != first.level) break;
                stable_group = std::min(stable_group, candidate.segment_id);
                ++width;
            }
            for (int offset = 0; offset < width; ++offset) {
                const int member_index = room_index_at_state(
                    state, column + offset, floor);
                state.room_entries[static_cast<std::size_t>(member_index)].group_id =
                    stable_group;
            }
            column += width;
        }
    }
}

PlayableRoomType legacy_room_type(int room) noexcept {
    switch (room) {
        case 0:
            return PlayableRoomType::Power;
        case 1:
            return PlayableRoomType::Food;
        case 2:
            return PlayableRoomType::Water;
        case 3:
            return PlayableRoomType::Workshop;
        case 4:
            return PlayableRoomType::Living;
        default:
            return PlayableRoomType::Workshop;
    }
}

std::array<int, 2> legacy_room_position(int room) noexcept {
    return {{room, 0}};
}

int room_index_at_state(const PlayableShelterState& state,
                        int column,
                        int floor) noexcept {
    if (!in_grid(column, floor)) return -1;
    for (int room = 0; room < kPlayableRoomCapacity; ++room) {
        const auto& entry =
            state.room_entries[static_cast<std::size_t>(room)];
        if (entry.active && entry.column == column &&
            entry.floor == floor) {
            return room;
        }
    }
    return -1;
}

bool is_elevator_cell(const PlayableShelterState& state,
                      int column,
                      int floor) noexcept {
    const int room = room_index_at_state(state, column, floor);
    return room >= 0 &&
           state.room_entries[static_cast<std::size_t>(room)].type ==
               PlayableRoomType::Elevator;
}

bool traversable_cell(const PlayableShelterState& state,
                      int column,
                      int floor) noexcept {
    return room_index_at_state(state, column, floor) >= 0;
}

void seed_residents(PlayableShelterState& state) noexcept {
    for (int index = 0; index < kPlayableResidentCount; ++index) {
        auto& resident =
            state.residents[static_cast<std::size_t>(index)];
        resident.active = true;
        resident.current_column = 0;
        resident.current_floor = 0;
        resident.next_column = resident.current_column;
        resident.next_floor = resident.current_floor;
        resident.destination_column = resident.current_column;
        resident.destination_floor = 0;
        resident.assigned_room = -1;
        resident.state = PlayableResidentState::Roaming;
        resident.movement_ticks = 0;
        resident.roaming_ticks = 0;
        resident.roaming_sequence = index;
    }
}

void sync_legacy_fields(PlayableShelterState& state) noexcept {
    int active_rooms = 0;
    for (const auto& room : state.room_entries) {
        if (room.active) ++active_rooms;
    }
    state.rooms = active_rooms;
    for (int room = 0; room < kPlayableMaxRooms; ++room) {
        const auto& entry =
            state.room_entries[static_cast<std::size_t>(room)];
        state.stored[static_cast<std::size_t>(room)] =
            entry.active ? entry.stored : 0;
        state.production_steps[static_cast<std::size_t>(room)] =
            entry.active ? entry.production_steps : 0;
    }
    state.assigned_room =
        state.residents[0].active ? state.residents[0].assigned_room : -1;
}

void upgrade_legacy_shape(PlayableShelterState& state) noexcept {
    bool has_runtime_room = false;
    for (const auto& room : state.room_entries) {
        has_runtime_room = has_runtime_room || room.active;
    }
    if (!has_runtime_room) {
        const int legacy_rooms =
            std::max(1, std::min(state.rooms, kPlayableMaxRooms));
        for (int room = 0; room < legacy_rooms; ++room) {
            auto& entry =
                state.room_entries[static_cast<std::size_t>(room)];
            const auto position = legacy_room_position(room);
            entry.active = true;
            entry.type = legacy_room_type(room);
            entry.column = position[0];
            entry.floor = position[1];
            entry.stored =
                state.stored[static_cast<std::size_t>(room)];
            entry.production_steps =
                state.production_steps[static_cast<std::size_t>(room)];
        }
    }

    bool has_resident = false;
    for (const auto& resident : state.residents) {
        has_resident = has_resident || resident.active;
    }
    if (!has_resident) seed_residents(state);

    if (!has_resident &&
        state.assigned_room >= 0 &&
        state.assigned_room < kPlayableRoomCapacity &&
        state.room_entries[
            static_cast<std::size_t>(state.assigned_room)].active) {
        auto& resident = state.residents[0];
        const auto& room = state.room_entries[
            static_cast<std::size_t>(state.assigned_room)];
        resident.assigned_room = state.assigned_room;
        resident.current_column = room.column;
        resident.current_floor = room.floor;
        resident.next_column = room.column;
        resident.next_floor = room.floor;
        resident.destination_column = room.column;
        resident.destination_floor = room.floor;
        resident.state = PlayableResidentState::Working;
    }
    normalize_room_groups(state);
    sync_legacy_fields(state);
}

PlayableShelterState normalized_state(
    PlayableShelterState state) noexcept {
    upgrade_legacy_shape(state);
    normalize_room_groups(state);
    return state;
}

bool vertical_edge_allowed(const PlayableShelterState& state,
                           int from_column,
                           int from_floor,
                           int to_column,
                           int to_floor) noexcept {
    return from_column == to_column &&
           std::abs(from_floor - to_floor) == 1 &&
           is_elevator_cell(state, from_column, from_floor) &&
           is_elevator_cell(state, to_column, to_floor);
}

bool next_route_step(const PlayableShelterState& state,
                     int start_column,
                     int start_floor,
                     int goal_column,
                     int goal_floor,
                     int& next_column,
                     int& next_floor) noexcept {
    if (!in_grid(start_column, start_floor) ||
        !in_grid(goal_column, goal_floor) ||
        !traversable_cell(state, start_column, start_floor) ||
        !traversable_cell(state, goal_column, goal_floor)) {
        return false;
    }
    if (start_column == goal_column && start_floor == goal_floor) {
        next_column = start_column;
        next_floor = start_floor;
        return true;
    }

    constexpr int kCellCount =
        kPlayableGridColumns * kPlayableGridFloors;
    std::array<int, kCellCount> queue{};
    std::array<int, kCellCount> previous{};
    previous.fill(-1);
    const int start = grid_index(start_column, start_floor);
    const int goal = grid_index(goal_column, goal_floor);
    previous[static_cast<std::size_t>(start)] = start;
    queue[0] = start;
    int head = 0;
    int tail = 1;
    constexpr std::array<int, 4> kColumnDelta{{-1, 1, 0, 0}};
    constexpr std::array<int, 4> kFloorDelta{{0, 0, -1, 1}};

    while (head < tail && previous[static_cast<std::size_t>(goal)] < 0) {
        const int current = queue[static_cast<std::size_t>(head++)];
        const int column = current % kPlayableGridColumns;
        const int floor = current / kPlayableGridColumns;
        for (std::size_t direction = 0;
             direction < kColumnDelta.size(); ++direction) {
            const int candidate_column =
                column + kColumnDelta[direction];
            const int candidate_floor =
                floor + kFloorDelta[direction];
            if (!in_grid(candidate_column, candidate_floor) ||
                !traversable_cell(
                    state, candidate_column, candidate_floor)) {
                continue;
            }
            if (candidate_floor != floor &&
                !vertical_edge_allowed(
                    state, column, floor,
                    candidate_column, candidate_floor)) {
                continue;
            }
            const int candidate =
                grid_index(candidate_column, candidate_floor);
            if (previous[static_cast<std::size_t>(candidate)] >= 0) {
                continue;
            }
            previous[static_cast<std::size_t>(candidate)] = current;
            queue[static_cast<std::size_t>(tail++)] = candidate;
        }
    }
    if (previous[static_cast<std::size_t>(goal)] < 0) return false;

    int step = goal;
    while (previous[static_cast<std::size_t>(step)] != start) {
        step = previous[static_cast<std::size_t>(step)];
    }
    next_column = step % kPlayableGridColumns;
    next_floor = step / kPlayableGridColumns;
    return true;
}

bool route_exists(const PlayableShelterState& state,
                  const PlayableResidentEntry& resident,
                  const PlayableRoomEntry& room) noexcept {
    int ignored_column = 0;
    int ignored_floor = 0;
    return next_route_step(
        state, resident.current_column, resident.current_floor,
        room.column, room.floor, ignored_column, ignored_floor);
}

void step_resident(PlayableShelterState& state,
                   PlayableResidentEntry& resident) noexcept {
    if (!resident.active) return;

    if (resident.state == PlayableResidentState::Roaming &&
        resident.current_column == resident.destination_column &&
        resident.current_floor == resident.destination_floor) {
        ++resident.roaming_ticks;
        if (resident.roaming_ticks < kRoamingPauseTicks) return;
        resident.roaming_ticks = 0;
        ++resident.roaming_sequence;
        const int first_slot =
            resident.roaming_sequence % kPlayableRoomCapacity;
        bool found_destination = false;
        for (int offset = 0;
             offset < kPlayableRoomCapacity; ++offset) {
            const int slot =
                (first_slot + offset) % kPlayableRoomCapacity;
            const auto& room =
                state.room_entries[static_cast<std::size_t>(slot)];
            if (!room.active ||
                (room.column == resident.current_column &&
                 room.floor == resident.current_floor)) {
                continue;
            }
            int ignored_column = 0;
            int ignored_floor = 0;
            if (next_route_step(
                    state,
                    resident.current_column,
                    resident.current_floor,
                    room.column,
                    room.floor,
                    ignored_column,
                    ignored_floor)) {
                resident.destination_column = room.column;
                resident.destination_floor = room.floor;
                found_destination = true;
                break;
            }
        }
        if (!found_destination) return;
    }

    if (resident.state == PlayableResidentState::Working) return;
    if (resident.current_column == resident.destination_column &&
        resident.current_floor == resident.destination_floor) {
        if (resident.state == PlayableResidentState::Transit) {
            resident.state = PlayableResidentState::Working;
            resident.movement_ticks = 0;
        }
        return;
    }

    if (resident.next_column == resident.current_column &&
        resident.next_floor == resident.current_floor) {
        if (!next_route_step(
                state, resident.current_column, resident.current_floor,
                resident.destination_column, resident.destination_floor,
                resident.next_column, resident.next_floor)) {
            return;
        }
    }

    ++resident.movement_ticks;
    if (resident.movement_ticks < kPlayableMovementStepTicks) return;
    resident.movement_ticks = 0;
    resident.current_column = resident.next_column;
    resident.current_floor = resident.next_floor;
    resident.next_column = resident.current_column;
    resident.next_floor = resident.current_floor;
    if (resident.current_column == resident.destination_column &&
        resident.current_floor == resident.destination_floor &&
        resident.state == PlayableResidentState::Transit) {
        resident.state = PlayableResidentState::Working;
    }
}

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

void append_u64(std::vector<std::uint8_t>& output,
                std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8) {
        output.push_back(static_cast<std::uint8_t>(value >> shift));
    }
}

void append_i32(std::vector<std::uint8_t>& output, int value) {
    append_u32(output, static_cast<std::uint32_t>(
                           static_cast<std::int32_t>(value)));
}

bool read_u16(const std::vector<std::uint8_t>& input,
              std::size_t& offset,
              std::uint16_t& value) noexcept {
    if (offset + 2u > input.size()) return false;
    value = static_cast<std::uint16_t>(input[offset]) |
            static_cast<std::uint16_t>(input[offset + 1u] << 8u);
    offset += 2u;
    return true;
}

bool read_u32(const std::vector<std::uint8_t>& input,
              std::size_t& offset,
              std::uint32_t& value) noexcept {
    if (offset + 4u > input.size()) return false;
    value = 0u;
    for (int shift = 0; shift < 32; shift += 8) {
        value |= static_cast<std::uint32_t>(input[offset++]) << shift;
    }
    return true;
}

bool read_u64(const std::vector<std::uint8_t>& input,
              std::size_t& offset,
              std::uint64_t& value) noexcept {
    if (offset + 8u > input.size()) return false;
    value = 0u;
    for (int shift = 0; shift < 64; shift += 8) {
        value |= static_cast<std::uint64_t>(input[offset++]) << shift;
    }
    return true;
}

bool read_i32(const std::vector<std::uint8_t>& input,
              std::size_t& offset,
              int& value) noexcept {
    std::uint32_t encoded = 0u;
    if (!read_u32(input, offset, encoded)) return false;
    value = static_cast<int>(static_cast<std::int32_t>(encoded));
    return true;
}

bool read_enum(const std::vector<std::uint8_t>& input,
               std::size_t& offset,
               PlayableRoomType& value) noexcept {
    int encoded = 0;
    if (!read_i32(input, offset, encoded)) return false;
    value = static_cast<PlayableRoomType>(encoded);
    return valid_room_type(value);
}

bool read_enum(const std::vector<std::uint8_t>& input,
               std::size_t& offset,
               PlayableResidentState& value) noexcept {
    int encoded = 0;
    if (!read_i32(input, offset, encoded)) return false;
    value = static_cast<PlayableResidentState>(encoded);
    return valid_resident_state(value);
}

PlayableSaveStatus read_file(const std::string& path,
                             std::vector<std::uint8_t>& bytes) {
    FILE* file = std::fopen(path.c_str(), "rb");
    if (file == nullptr) {
        return errno == ENOENT ? PlayableSaveStatus::Missing
                              : PlayableSaveStatus::IoError;
    }
    if (std::fseek(file, 0, SEEK_END) != 0) {
        std::fclose(file);
        return PlayableSaveStatus::IoError;
    }
    const long size = std::ftell(file);
    if (size < 0 ||
        static_cast<std::size_t>(size) > kMaximumPlayableSaveSize ||
        std::fseek(file, 0, SEEK_SET) != 0) {
        std::fclose(file);
        return PlayableSaveStatus::Corrupt;
    }
    bytes.resize(static_cast<std::size_t>(size));
    const bool read_ok =
        bytes.empty() ||
        std::fread(bytes.data(), 1u, bytes.size(), file) == bytes.size();
    const bool close_ok = std::fclose(file) == 0;
    return read_ok && close_ok ? PlayableSaveStatus::Ok
                               : PlayableSaveStatus::IoError;
}

PlayableLoadResult load_one(const std::string& path) {
    std::vector<std::uint8_t> bytes;
    const PlayableSaveStatus read_status = read_file(path, bytes);
    if (read_status != PlayableSaveStatus::Ok) {
        PlayableLoadResult result;
        result.status = read_status;
        return result;
    }
    return decode_playable_state(bytes);
}

bool read_v1_payload(const std::vector<std::uint8_t>& body,
                     PlayableShelterState& state) noexcept {
    std::size_t offset = 0u;
    if (!read_i32(body, offset, state.credits) ||
        !read_i32(body, offset, state.power) ||
        !read_i32(body, offset, state.food) ||
        !read_i32(body, offset, state.water) ||
        !read_i32(body, offset, state.rooms) ||
        !read_i32(body, offset, state.selected_room) ||
        !read_i32(body, offset, state.assigned_room)) {
        return false;
    }
    for (int& value : state.stored) {
        if (!read_i32(body, offset, value)) return false;
    }
    for (int& value : state.production_steps) {
        if (!read_i32(body, offset, value)) return false;
    }
    if (offset != body.size() ||
        state.credits < 0 ||
        state.power < 0 || state.power > 100 ||
        state.food < 0 || state.food > 100 ||
        state.water < 0 || state.water > 100 ||
        state.rooms < 1 || state.rooms > kPlayableMaxRooms ||
        state.selected_room < 0 ||
        state.selected_room >= state.rooms ||
        state.assigned_room < -1 ||
        state.assigned_room >= state.rooms) {
        return false;
    }
    for (std::size_t room = 0; room < state.stored.size(); ++room) {
        if (state.stored[room] < 0 ||
            state.stored[room] > kPlayableStorageCapacity ||
            state.production_steps[room] < 0 ||
            state.production_steps[room] >=
                kPlayableProductionCycleSteps) {
            return false;
        }
    }
    upgrade_legacy_shape(state);
    return valid_playable_state(state);
}

bool read_v2_payload(const std::vector<std::uint8_t>& body,
                     PlayableShelterState& state) noexcept {
    std::size_t offset = 0u;
    if (!read_i32(body, offset, state.credits) ||
        !read_i32(body, offset, state.power) ||
        !read_i32(body, offset, state.food) ||
        !read_i32(body, offset, state.water) ||
        !read_i32(body, offset, state.rooms) ||
        !read_i32(body, offset, state.selected_room) ||
        !read_i32(body, offset, state.assigned_room) ||
        !read_enum(body, offset, state.selected_build_type) ||
        !read_i32(body, offset, state.build_cursor_column) ||
        !read_i32(body, offset, state.build_cursor_floor)) {
        return false;
    }
    for (int& value : state.stored) {
        if (!read_i32(body, offset, value)) return false;
    }
    for (int& value : state.production_steps) {
        if (!read_i32(body, offset, value)) return false;
    }
    for (auto& room : state.room_entries) {
        int active = 0;
        if (!read_i32(body, offset, active) ||
            (active != 0 && active != 1) ||
            !read_enum(body, offset, room.type) ||
            !read_i32(body, offset, room.column) ||
            !read_i32(body, offset, room.floor) ||
            !read_i32(body, offset, room.stored) ||
            !read_i32(body, offset, room.production_steps)) {
            return false;
        }
        room.active = active != 0;
    }
    for (auto& resident : state.residents) {
        int active = 0;
        if (!read_i32(body, offset, active) ||
            (active != 0 && active != 1) ||
            !read_i32(body, offset, resident.current_column) ||
            !read_i32(body, offset, resident.current_floor) ||
            !read_i32(body, offset, resident.next_column) ||
            !read_i32(body, offset, resident.next_floor) ||
            !read_i32(body, offset, resident.destination_column) ||
            !read_i32(body, offset, resident.destination_floor) ||
            !read_i32(body, offset, resident.assigned_room) ||
            !read_enum(body, offset, resident.state) ||
            !read_i32(body, offset, resident.movement_ticks) ||
            !read_i32(body, offset, resident.roaming_ticks) ||
            !read_i32(body, offset, resident.roaming_sequence)) {
            return false;
        }
        resident.active = active != 0;
    }
    if (offset != body.size()) return false;
    normalize_room_groups(state);
    return valid_playable_state(state);
}

bool read_v3_payload(const std::vector<std::uint8_t>& body,
                     PlayableShelterState& state) noexcept {
    std::size_t offset = 0u;
    if (!read_i32(body, offset, state.credits) ||
        !read_i32(body, offset, state.power) ||
        !read_i32(body, offset, state.food) ||
        !read_i32(body, offset, state.water) ||
        !read_i32(body, offset, state.rooms) ||
        !read_i32(body, offset, state.selected_room) ||
        !read_i32(body, offset, state.assigned_room) ||
        !read_enum(body, offset, state.selected_build_type) ||
        !read_i32(body, offset, state.build_cursor_column) ||
        !read_i32(body, offset, state.build_cursor_floor) ||
        !read_u64(body, offset, state.next_segment_id)) return false;
    for (int& value : state.stored) if (!read_i32(body, offset, value)) return false;
    for (int& value : state.production_steps) if (!read_i32(body, offset, value)) return false;
    for (auto& room : state.room_entries) {
        int active = 0;
        if (!read_i32(body, offset, active) || (active != 0 && active != 1) ||
            !read_enum(body, offset, room.type) ||
            !read_i32(body, offset, room.column) ||
            !read_i32(body, offset, room.floor) ||
            !read_i32(body, offset, room.stored) ||
            !read_i32(body, offset, room.production_steps) ||
            !read_u64(body, offset, room.segment_id) ||
            !read_u64(body, offset, room.group_id) ||
            !read_i32(body, offset, room.level)) return false;
        room.active = active != 0;
    }
    for (auto& resident : state.residents) {
        int active = 0;
        if (!read_i32(body, offset, active) || (active != 0 && active != 1) ||
            !read_i32(body, offset, resident.current_column) ||
            !read_i32(body, offset, resident.current_floor) ||
            !read_i32(body, offset, resident.next_column) ||
            !read_i32(body, offset, resident.next_floor) ||
            !read_i32(body, offset, resident.destination_column) ||
            !read_i32(body, offset, resident.destination_floor) ||
            !read_i32(body, offset, resident.assigned_room) ||
            !read_enum(body, offset, resident.state) ||
            !read_i32(body, offset, resident.movement_ticks) ||
            !read_i32(body, offset, resident.roaming_ticks) ||
            !read_i32(body, offset, resident.roaming_sequence)) return false;
        resident.active = active != 0;
    }
    return offset == body.size() && valid_playable_state(state);
}

}  // namespace

PlayableShelterSession::PlayableShelterSession()
    : PlayableShelterSession(PlayableShelterState{}) {}

PlayableShelterSession::PlayableShelterSession(PlayableShelterState state)
    : state_(normalized_state(state)) {
    if (!valid_playable_state(state_)) {
        state_ = normalized_state(PlayableShelterState{});
    }
}

const PlayableShelterState& PlayableShelterSession::state() const noexcept {
    return state_;
}

bool PlayableShelterSession::select_previous_room() noexcept {
    for (int room = state_.selected_room - 1; room >= 0; --room) {
        if (state_.room_entries[static_cast<std::size_t>(room)].active) {
            state_.selected_room = room;
            return true;
        }
    }
    return false;
}

bool PlayableShelterSession::select_next_room() noexcept {
    for (int room = state_.selected_room + 1;
         room < kPlayableRoomCapacity; ++room) {
        if (state_.room_entries[static_cast<std::size_t>(room)].active) {
            state_.selected_room = room;
            return true;
        }
    }
    return false;
}

BuildResult PlayableShelterSession::build_room() noexcept {
    if (state_.rooms >= kPlayableMaxRooms) return BuildResult::Full;
    constexpr int kLegacyRoomCost = 100;
    if (state_.credits < kLegacyRoomCost) {
        return BuildResult::NotEnoughCredits;
    }
    const int room_index = state_.rooms;
    const auto position = legacy_room_position(room_index);
    if (room_index_at(position[0], position[1]) >= 0) {
        return BuildResult::InvalidPlacement;
    }
    auto& room =
        state_.room_entries[static_cast<std::size_t>(room_index)];
    room.active = true;
    room.type = legacy_room_type(room_index);
    room.column = position[0];
    room.floor = position[1];
    room.stored = 0;
    room.production_steps = 0;
    state_.credits -= kLegacyRoomCost;
    state_.selected_room = room_index;
    sync_legacy_view();
    return BuildResult::Built;
}

void PlayableShelterSession::assign_selected_room() noexcept {
    (void)assign_resident_to_room(0u, state_.selected_room);
}

CollectResult PlayableShelterSession::collect_selected_room() noexcept {
    auto& room = state_.room_entries[
        static_cast<std::size_t>(state_.selected_room)];
    const int amount = room.stored;
    if (amount <= 0) return CollectResult::NothingStored;
    room.stored = 0;
    switch (room.type) {
        case PlayableRoomType::Power:
            state_.power = std::min(100, state_.power + amount);
            break;
        case PlayableRoomType::Food:
            state_.food = std::min(100, state_.food + amount);
            break;
        case PlayableRoomType::Water:
            state_.water = std::min(100, state_.water + amount);
            break;
        case PlayableRoomType::Workshop:
            state_.credits += amount * 2;
            break;
        case PlayableRoomType::Living:
        case PlayableRoomType::Elevator:
            break;
    }
    state_.credits += amount * 3;
    sync_legacy_view();
    return CollectResult::Collected;
}

void PlayableShelterSession::fixed_step() noexcept {
    for (auto& resident : state_.residents) {
        step_resident(state_, resident);
    }

    for (int room_index = 0;
         room_index < kPlayableRoomCapacity; ++room_index) {
        auto& room =
            state_.room_entries[static_cast<std::size_t>(room_index)];
        if (!room.active || !room_produces(room.type)) continue;
        bool working = false;
        for (const auto& resident : state_.residents) {
            if (resident.active &&
                resident.state == PlayableResidentState::Working &&
                resident.assigned_room == room_index &&
                resident.current_column == room.column &&
                resident.current_floor == room.floor) {
                working = true;
                break;
            }
        }
        if (!working) continue;
        ++room.production_steps;
        if (room.production_steps >=
            kPlayableProductionCycleSteps) {
            room.production_steps = 0;
            room.stored = std::min(
                kPlayableStorageCapacity, room.stored + 5);
        }
    }
    sync_legacy_view();
}

bool PlayableShelterSession::select_build_type(
    PlayableRoomType type) noexcept {
    if (!valid_room_type(type)) return false;
    state_.selected_build_type = type;
    return true;
}

PlayableRoomType
PlayableShelterSession::selected_build_type() const noexcept {
    return state_.selected_build_type;
}

bool PlayableShelterSession::set_build_cursor(
    int column, int floor) noexcept {
    if (!in_grid(column, floor)) return false;
    state_.build_cursor_column = column;
    state_.build_cursor_floor = floor;
    return true;
}

bool PlayableShelterSession::move_build_cursor(
    int column_delta, int floor_delta) noexcept {
    return set_build_cursor(
        state_.build_cursor_column + column_delta,
        state_.build_cursor_floor + floor_delta);
}

int PlayableShelterSession::build_cursor_column() const noexcept {
    return state_.build_cursor_column;
}

int PlayableShelterSession::build_cursor_floor() const noexcept {
    return state_.build_cursor_floor;
}

PlayableBuildPreview
PlayableShelterSession::preview_build() const noexcept {
    PlayableBuildPreview preview;
    preview.cost = room_cost(state_.selected_build_type);
    if (!in_grid(
            state_.build_cursor_column,
            state_.build_cursor_floor)) {
        preview.status = BuildPreviewStatus::OutOfBounds;
    } else if (room_index_at(
                   state_.build_cursor_column,
                   state_.build_cursor_floor) >= 0) {
        preview.status = BuildPreviewStatus::Occupied;
    } else if (state_.rooms >= kPlayableRoomCapacity) {
        preview.status = BuildPreviewStatus::Full;
    } else if (state_.credits < preview.cost) {
        preview.status = BuildPreviewStatus::NotEnoughCredits;
    } else {
        preview.status = BuildPreviewStatus::Valid;
    }
    return preview;
}

BuildResult PlayableShelterSession::confirm_build() noexcept {
    const PlayableBuildPreview preview = preview_build();
    switch (preview.status) {
        case BuildPreviewStatus::NotEnoughCredits:
            return BuildResult::NotEnoughCredits;
        case BuildPreviewStatus::Full:
            return BuildResult::Full;
        case BuildPreviewStatus::OutOfBounds:
        case BuildPreviewStatus::Occupied:
            return BuildResult::InvalidPlacement;
        case BuildPreviewStatus::Valid:
            break;
    }

    int slot = -1;
    for (int room = 0; room < kPlayableRoomCapacity; ++room) {
        if (!state_.room_entries[
                 static_cast<std::size_t>(room)].active) {
            slot = room;
            break;
        }
    }
    if (slot < 0) return BuildResult::Full;
    auto& room =
        state_.room_entries[static_cast<std::size_t>(slot)];
    room.active = true;
    room.type = state_.selected_build_type;
    room.column = state_.build_cursor_column;
    room.floor = state_.build_cursor_floor;
    room.stored = 0;
    room.production_steps = 0;
    room.segment_id = state_.next_segment_id++;
    room.group_id = room.segment_id;
    room.level = 1;
    state_.credits -= preview.cost;
    state_.selected_room = slot;
    normalize_room_groups(state_);
    sync_legacy_view();
    return BuildResult::Built;
}

int PlayableShelterSession::selected_group_width() const noexcept {
    const auto& selected = state_.room_entries[
        static_cast<std::size_t>(state_.selected_room)];
    int width = 0;
    for (const auto& room : state_.room_entries) {
        if (room.active && room.group_id == selected.group_id) ++width;
    }
    return width;
}

PlayableRoomLifecyclePreview
PlayableShelterSession::preview_upgrade_selected() const noexcept {
    PlayableRoomLifecyclePreview preview;
    const auto& selected = state_.room_entries[
        static_cast<std::size_t>(state_.selected_room)];
    if (!selected.active) return preview;
    preview.group_width = selected_group_width();
    if (selected.level >= kPlayableMaximumRoomLevel) {
        preview.result = RoomLifecycleResult::MaximumLevel;
        return preview;
    }
    const int cost = upgrade_cost(selected, preview.group_width);
    preview.credit_delta = -cost;
    if (state_.credits < cost) {
        preview.result = RoomLifecycleResult::NotEnoughCredits;
        return preview;
    }
    for (int index = 0; index < kPlayableRoomCapacity; ++index) {
        const auto& room = state_.room_entries[static_cast<std::size_t>(index)];
        if (!room.active || room.group_id != selected.group_id) continue;
        preview.stored_units_affected += room.stored;
        preview.production_steps_affected += room.production_steps;
        for (const auto& resident : state_.residents) {
            if (resident.active && resident.assigned_room == index) {
                ++preview.residents_affected;
            }
        }
    }
    preview.result = RoomLifecycleResult::Applied;
    return preview;
}

RoomLifecycleResult
PlayableShelterSession::confirm_upgrade_selected() noexcept {
    const auto preview = preview_upgrade_selected();
    if (!preview.allowed()) return preview.result;
    const std::uint64_t group_id = state_.room_entries[
        static_cast<std::size_t>(state_.selected_room)].group_id;
    state_.credits += preview.credit_delta;
    for (auto& room : state_.room_entries) {
        if (room.active && room.group_id == group_id) ++room.level;
    }
    normalize_room_groups(state_);
    sync_legacy_view();
    return RoomLifecycleResult::Applied;
}

PlayableRoomLifecyclePreview
PlayableShelterSession::preview_demolish_selected() const noexcept {
    PlayableRoomLifecyclePreview preview;
    const int selected_index = state_.selected_room;
    const auto& selected = state_.room_entries[
        static_cast<std::size_t>(selected_index)];
    if (!selected.active) return preview;
    preview.group_width = selected_group_width();
    preview.credit_delta = demolition_refund(selected);
    preview.stored_units_affected = selected.stored;
    preview.production_steps_affected = selected.production_steps;

    PlayableShelterState candidate = state_;
    candidate.room_entries[static_cast<std::size_t>(selected_index)] = {};
    normalize_room_groups(candidate);
    for (const auto& resident : state_.residents) {
        if (!resident.active) continue;
        const bool touches_removed =
            resident.assigned_room == selected_index ||
            (resident.current_column == selected.column &&
             resident.current_floor == selected.floor) ||
            (resident.next_column == selected.column &&
             resident.next_floor == selected.floor) ||
            (resident.destination_column == selected.column &&
             resident.destination_floor == selected.floor);
        const bool current_survives = traversable_cell(
            candidate, resident.current_column, resident.current_floor);
        if (touches_removed || !current_survives) {
            ++preview.residents_affected;
        }
    }
    if (state_.rooms <= 1) {
        preview.result = RoomLifecycleResult::LastRoom;
    } else if (preview.stored_units_affected > 0) {
        preview.result = RoomLifecycleResult::UnsafeStoredResources;
    } else if (preview.production_steps_affected > 0) {
        preview.result = RoomLifecycleResult::UnsafeProduction;
    } else {
        preview.result = RoomLifecycleResult::Applied;
    }
    return preview;
}

RoomLifecycleResult
PlayableShelterSession::confirm_demolish_selected() noexcept {
    const auto preview = preview_demolish_selected();
    if (!preview.allowed()) return preview.result;
    const int removed = state_.selected_room;
    const PlayableRoomEntry removed_room = state_.room_entries[
        static_cast<std::size_t>(removed)];

    int replacement = -1;
    int replacement_distance = kPlayableGridColumns + kPlayableGridFloors + 1;
    std::uint64_t replacement_id = UINT64_MAX;
    for (int index = 0; index < kPlayableRoomCapacity; ++index) {
        const auto& room = state_.room_entries[static_cast<std::size_t>(index)];
        if (!room.active || index == removed) continue;
        const int distance = std::abs(room.column - removed_room.column) +
                             std::abs(room.floor - removed_room.floor);
        if (replacement < 0 || distance < replacement_distance ||
            (distance == replacement_distance && room.segment_id < replacement_id)) {
            replacement = index;
            replacement_distance = distance;
            replacement_id = room.segment_id;
        }
    }
    if (replacement < 0) return RoomLifecycleResult::LastRoom;
    const PlayableRoomEntry fallback = state_.room_entries[
        static_cast<std::size_t>(replacement)];

    state_.credits += preview.credit_delta;
    state_.room_entries[static_cast<std::size_t>(removed)] = {};
    normalize_room_groups(state_);
    for (auto& resident : state_.residents) {
        if (!resident.active) continue;
        const bool touches_removed =
            resident.assigned_room == removed ||
            (resident.current_column == removed_room.column &&
             resident.current_floor == removed_room.floor) ||
            (resident.next_column == removed_room.column &&
             resident.next_floor == removed_room.floor) ||
            (resident.destination_column == removed_room.column &&
             resident.destination_floor == removed_room.floor);
        const bool current_survives = traversable_cell(
            state_, resident.current_column, resident.current_floor);
        if (!touches_removed && current_survives) continue;

        if (!current_survives) {
            resident.current_column = fallback.column;
            resident.current_floor = fallback.floor;
        }
        resident.next_column = resident.current_column;
        resident.next_floor = resident.current_floor;
        resident.destination_column = fallback.column;
        resident.destination_floor = fallback.floor;
        resident.assigned_room = -1;
        resident.state = PlayableResidentState::Roaming;
        resident.movement_ticks = 0;
        resident.roaming_ticks = 0;
    }
    state_.selected_room = replacement;
    sync_legacy_view();
    return RoomLifecycleResult::Applied;
}

int PlayableShelterSession::room_index_at(
    int column, int floor) const noexcept {
    return room_index_at_state(state_, column, floor);
}

bool PlayableShelterSession::assign_resident_to_room(
    std::size_t resident_index, int room_index) noexcept {
    if (resident_index >= state_.residents.size() ||
        room_index < 0 || room_index >= kPlayableRoomCapacity) {
        return false;
    }
    auto& resident = state_.residents[resident_index];
    const auto& room =
        state_.room_entries[static_cast<std::size_t>(room_index)];
    if (!resident.active || !room.active ||
        room.type == PlayableRoomType::Elevator ||
        !route_exists(state_, resident, room)) {
        return false;
    }
    resident.assigned_room = room_index;
    resident.destination_column = room.column;
    resident.destination_floor = room.floor;
    resident.next_column = resident.current_column;
    resident.next_floor = resident.current_floor;
    resident.movement_ticks = 0;
    resident.roaming_ticks = 0;
    resident.state =
        resident.current_column == room.column &&
                resident.current_floor == room.floor
            ? PlayableResidentState::Working
            : PlayableResidentState::Transit;
    sync_legacy_view();
    return true;
}

PlayableResidentPosition PlayableShelterSession::resident_position(
    std::size_t resident_index) const noexcept {
    PlayableResidentPosition position;
    if (resident_index >= state_.residents.size()) return position;
    const auto& resident = state_.residents[resident_index];
    position.active = resident.active;
    if (!resident.active) return position;
    const float progress =
        static_cast<float>(resident.movement_ticks) /
        static_cast<float>(kPlayableMovementStepTicks);
    position.column =
        static_cast<float>(resident.current_column) +
        static_cast<float>(
            resident.next_column - resident.current_column) *
            progress;
    position.floor =
        static_cast<float>(resident.current_floor) +
        static_cast<float>(
            resident.next_floor - resident.current_floor) *
            progress;
    return position;
}

int PlayableShelterSession::selected_stored() const noexcept {
    return state_.room_entries[
        static_cast<std::size_t>(state_.selected_room)].stored;
}

int PlayableShelterSession::selected_progress() const noexcept {
    return state_.room_entries[
        static_cast<std::size_t>(state_.selected_room)].production_steps;
}

bool PlayableShelterSession::selected_has_worker() const noexcept {
    for (const auto& resident : state_.residents) {
        if (resident.active &&
            resident.state == PlayableResidentState::Working &&
            resident.assigned_room == state_.selected_room) {
            return true;
        }
    }
    return false;
}

PrimaryAction PlayableShelterSession::primary_action() const noexcept {
    if (selected_stored() > 0) return PrimaryAction::Collect;
    if (!selected_has_worker()) return PrimaryAction::Assign;
    return PrimaryAction::Wait;
}

const char* PlayableShelterSession::next_step() const noexcept {
    switch (primary_action()) {
        case PrimaryAction::Assign:
            return "Przypisz mieszkanca do pokoju.";
        case PrimaryAction::Wait:
            return "Produkcja trwa - poczekaj.";
        case PrimaryAction::Collect:
            return "Zasoby gotowe - odbierz.";
    }
    return "Wybierz akcje.";
}

void PlayableShelterSession::sync_legacy_view() noexcept {
    sync_legacy_fields(state_);
}

bool valid_playable_state(const PlayableShelterState& state) noexcept {
    if (state.credits < 0 ||
        state.power < 0 || state.power > 100 ||
        state.food < 0 || state.food > 100 ||
        state.water < 0 || state.water > 100 ||
        state.rooms < 1 || state.rooms > kPlayableRoomCapacity ||
        state.selected_room < 0 ||
        state.selected_room >= kPlayableRoomCapacity ||
        state.assigned_room < -1 ||
        state.assigned_room >= kPlayableRoomCapacity ||
        !valid_room_type(state.selected_build_type) ||
        !in_grid(
            state.build_cursor_column, state.build_cursor_floor)) {
        return false;
    }

    int active_rooms = 0;
    std::array<bool, kPlayableRoomCapacity> occupied{};
    for (int room_index = 0;
         room_index < kPlayableRoomCapacity; ++room_index) {
        const auto& room =
            state.room_entries[static_cast<std::size_t>(room_index)];
        if (!room.active) continue;
        ++active_rooms;
        if (!valid_room_type(room.type) ||
            !in_grid(room.column, room.floor) ||
            room.stored < 0 ||
            room.stored > kPlayableStorageCapacity ||
            room.production_steps < 0 ||
            room.production_steps >= kPlayableProductionCycleSteps) {
            return false;
        }
        const int cell = grid_index(room.column, room.floor);
        if (occupied[static_cast<std::size_t>(cell)]) return false;
        occupied[static_cast<std::size_t>(cell)] = true;
    }
    if (active_rooms != state.rooms ||
        !state.room_entries[
             static_cast<std::size_t>(state.selected_room)].active) {
        return false;
    }

    for (int room = 0; room < kPlayableMaxRooms; ++room) {
        const auto& entry =
            state.room_entries[static_cast<std::size_t>(room)];
        const int expected_stored = entry.active ? entry.stored : 0;
        const int expected_progress =
            entry.active ? entry.production_steps : 0;
        if (state.stored[static_cast<std::size_t>(room)] !=
                expected_stored ||
            state.production_steps[static_cast<std::size_t>(room)] !=
                expected_progress) {
            return false;
        }
    }

    int active_residents = 0;
    for (const auto& resident : state.residents) {
        if (!resident.active) continue;
        ++active_residents;
        if (!valid_resident_state(resident.state) ||
            !in_grid(
                resident.current_column, resident.current_floor) ||
            !in_grid(
                resident.next_column, resident.next_floor) ||
            !in_grid(
                resident.destination_column,
                resident.destination_floor) ||
            !traversable_cell(
                state,
                resident.current_column,
                resident.current_floor) ||
            !traversable_cell(
                state,
                resident.next_column,
                resident.next_floor) ||
            !traversable_cell(
                state,
                resident.destination_column,
                resident.destination_floor) ||
            resident.movement_ticks < 0 ||
            resident.movement_ticks >= kPlayableMovementStepTicks ||
            resident.roaming_ticks < 0 ||
            resident.roaming_ticks >= kRoamingPauseTicks ||
            resident.roaming_sequence < 0) {
            return false;
        }
        const int segment_column_delta =
            std::abs(resident.next_column - resident.current_column);
        const int segment_floor_delta =
            std::abs(resident.next_floor - resident.current_floor);
        if (segment_column_delta + segment_floor_delta > 1 ||
            (segment_floor_delta == 1 &&
             !vertical_edge_allowed(
                 state,
                 resident.current_column,
                 resident.current_floor,
                 resident.next_column,
                 resident.next_floor)) ||
            (segment_column_delta + segment_floor_delta == 0 &&
             resident.movement_ticks != 0)) {
            return false;
        }
        if (resident.state == PlayableResidentState::Roaming) {
            if (resident.assigned_room != -1) return false;
            continue;
        }
        if (resident.assigned_room < 0 ||
            resident.assigned_room >= kPlayableRoomCapacity) {
            return false;
        }
        const auto& assigned = state.room_entries[
            static_cast<std::size_t>(resident.assigned_room)];
        if (!assigned.active ||
            resident.destination_column != assigned.column ||
            resident.destination_floor != assigned.floor) {
            return false;
        }
        if (resident.state == PlayableResidentState::Working &&
            (resident.current_column != resident.destination_column ||
             resident.current_floor != resident.destination_floor)) {
            return false;
        }
    }
    if (active_residents < kPlayableResidentCount) return false;
    return state.assigned_room == state.residents[0].assigned_room;
}

std::vector<std::uint8_t> encode_playable_state(
    const PlayableShelterState& source_state) {
    const PlayableShelterState state = normalized_state(source_state);
    if (!valid_playable_state(state)) return {};

    std::vector<std::uint8_t> payload;
    payload.reserve(kPlayableSavePayloadSizeV3);
    append_i32(payload, state.credits);
    append_i32(payload, state.power);
    append_i32(payload, state.food);
    append_i32(payload, state.water);
    append_i32(payload, state.rooms);
    append_i32(payload, state.selected_room);
    append_i32(payload, state.assigned_room);
    append_i32(payload, static_cast<int>(state.selected_build_type));
    append_i32(payload, state.build_cursor_column);
    append_i32(payload, state.build_cursor_floor);
    append_u64(payload, state.next_segment_id);
    for (const int value : state.stored) append_i32(payload, value);
    for (const int value : state.production_steps) {
        append_i32(payload, value);
    }
    for (const auto& room : state.room_entries) {
        append_i32(payload, room.active ? 1 : 0);
        append_i32(payload, static_cast<int>(room.type));
        append_i32(payload, room.column);
        append_i32(payload, room.floor);
        append_i32(payload, room.stored);
        append_i32(payload, room.production_steps);
        append_u64(payload, room.segment_id);
        append_u64(payload, room.group_id);
        append_i32(payload, room.level);
    }
    for (const auto& resident : state.residents) {
        append_i32(payload, resident.active ? 1 : 0);
        append_i32(payload, resident.current_column);
        append_i32(payload, resident.current_floor);
        append_i32(payload, resident.next_column);
        append_i32(payload, resident.next_floor);
        append_i32(payload, resident.destination_column);
        append_i32(payload, resident.destination_floor);
        append_i32(payload, resident.assigned_room);
        append_i32(payload, static_cast<int>(resident.state));
        append_i32(payload, resident.movement_ticks);
        append_i32(payload, resident.roaming_ticks);
        append_i32(payload, resident.roaming_sequence);
    }
    if (payload.size() != kPlayableSavePayloadSizeV3) return {};

    std::vector<std::uint8_t> output;
    output.reserve(kPlayableSaveHeaderSize + payload.size());
    append_u32(output, kPlayableSaveMagic);
    append_u16(output, kPlayableSaveVersionV3);
    append_u16(output, 0u);
    append_u32(output, static_cast<std::uint32_t>(payload.size()));
    append_u32(output, persistence::crc32(
                           payload.data(), payload.size()));
    output.insert(output.end(), payload.begin(), payload.end());
    return output;
}

PlayableLoadResult decode_playable_state(
    const std::vector<std::uint8_t>& bytes) {
    PlayableLoadResult result;
    if (bytes.size() < kPlayableSaveHeaderSize) {
        result.status = PlayableSaveStatus::Corrupt;
        return result;
    }
    std::size_t offset = 0u;
    std::uint32_t magic = 0u;
    std::uint16_t version = 0u;
    std::uint16_t reserved = 0u;
    std::uint32_t payload_size = 0u;
    std::uint32_t checksum = 0u;
    if (!read_u32(bytes, offset, magic) ||
        !read_u16(bytes, offset, version) ||
        !read_u16(bytes, offset, reserved) ||
        !read_u32(bytes, offset, payload_size) ||
        !read_u32(bytes, offset, checksum) ||
        magic != kPlayableSaveMagic ||
        (version != kPlayableSaveVersionV1 &&
         version != kPlayableSaveVersionV2 &&
         version != kPlayableSaveVersionV3) ||
        reserved != 0u ||
        (version == kPlayableSaveVersionV1 &&
         payload_size != kPlayableSavePayloadSizeV1) ||
        (version == kPlayableSaveVersionV2 &&
         payload_size != kPlayableSavePayloadSizeV2) ||
        (version == kPlayableSaveVersionV3 &&
         payload_size != kPlayableSavePayloadSizeV3) ||
        bytes.size() != kPlayableSaveHeaderSize + payload_size) {
        result.status = PlayableSaveStatus::Corrupt;
        return result;
    }
    const std::uint8_t* payload =
        bytes.data() + kPlayableSaveHeaderSize;
    if (persistence::crc32(payload, payload_size) != checksum) {
        result.status = PlayableSaveStatus::Corrupt;
        return result;
    }

    std::vector<std::uint8_t> body(
        payload, payload + payload_size);
    PlayableShelterState state;
    const bool parsed =
        version == kPlayableSaveVersionV1
            ? read_v1_payload(body, state)
            : (version == kPlayableSaveVersionV2
                   ? read_v2_payload(body, state)
                   : read_v3_payload(body, state));
    if (!parsed) {
        result.status = PlayableSaveStatus::Corrupt;
        return result;
    }
    result.status = PlayableSaveStatus::Ok;
    result.state = state;
    result.migrated_from_v1 = version == kPlayableSaveVersionV1;
    result.migrated_from_v2 = version == kPlayableSaveVersionV2;
    return result;
}

PlayableSaveStatus save_playable_state(
    const std::string& path_without_suffix,
    const PlayableShelterState& state) {
    const std::vector<std::uint8_t> bytes =
        encode_playable_state(state);
    if (bytes.empty()) return PlayableSaveStatus::Corrupt;
    const std::string main = path_without_suffix + ".sav";
    const std::string temp = path_without_suffix + ".tmp";
    const std::string backup = path_without_suffix + ".bak";

    FILE* file = std::fopen(temp.c_str(), "wb");
    if (file == nullptr) return PlayableSaveStatus::IoError;
    const bool write_ok =
        std::fwrite(bytes.data(), 1u, bytes.size(), file) ==
            bytes.size() &&
        std::fflush(file) == 0;
    const bool close_ok = std::fclose(file) == 0;
    if (!write_ok || !close_ok) {
        std::remove(temp.c_str());
        return PlayableSaveStatus::IoError;
    }

    const PlayableLoadResult verification = load_one(temp);
    if (verification.status != PlayableSaveStatus::Ok) {
        std::remove(temp.c_str());
        return PlayableSaveStatus::IoError;
    }

    const PlayableLoadResult current = load_one(main);
    if (current.status == PlayableSaveStatus::Ok) {
        std::remove(backup.c_str());
        if (std::rename(main.c_str(), backup.c_str()) != 0) {
            std::remove(temp.c_str());
            return PlayableSaveStatus::IoError;
        }
    } else if (current.status != PlayableSaveStatus::Missing) {
        std::remove(temp.c_str());
        return PlayableSaveStatus::IoError;
    }
    if (std::rename(temp.c_str(), main.c_str()) != 0) {
        std::rename(backup.c_str(), main.c_str());
        std::remove(temp.c_str());
        return PlayableSaveStatus::IoError;
    }
    return PlayableSaveStatus::Ok;
}

PlayableLoadResult load_playable_state(
    const std::string& path_without_suffix) {
    const std::string main = path_without_suffix + ".sav";
    const std::string backup = path_without_suffix + ".bak";
    PlayableLoadResult result = load_one(main);
    if (result.status == PlayableSaveStatus::Ok) return result;
    PlayableLoadResult fallback = load_one(backup);
    if (fallback.status == PlayableSaveStatus::Ok) {
        fallback.used_backup = true;
        return fallback;
    }
    return result.status == PlayableSaveStatus::Missing
               ? fallback
               : result;
}

}  // namespace deep_shelter::gameplay
