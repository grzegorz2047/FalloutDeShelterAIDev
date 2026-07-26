#include "gameplay/PlayableShelterSession.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>

#include "persistence/SaveData.hpp"

namespace deep_shelter::gameplay {
namespace {

constexpr std::uint32_t kPlayableSaveMagic = 0x33505344u;  // DSP3
constexpr std::uint16_t kPlayableSaveVersion = 1u;
constexpr std::size_t kPlayableSaveHeaderSize = 16u;
constexpr std::size_t kPlayableSavePayloadSize = 76u;
constexpr std::size_t kMaximumPlayableSaveSize = 4096u;

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

bool read_i32(const std::vector<std::uint8_t>& input,
              std::size_t& offset,
              int& value) noexcept {
    std::uint32_t encoded = 0u;
    if (!read_u32(input, offset, encoded)) return false;
    value = static_cast<int>(static_cast<std::int32_t>(encoded));
    return true;
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

}  // namespace

PlayableShelterSession::PlayableShelterSession(PlayableShelterState state)
    : state_(valid_playable_state(state) ? state : PlayableShelterState{}) {}

const PlayableShelterState& PlayableShelterSession::state() const noexcept {
    return state_;
}

bool PlayableShelterSession::select_previous_room() noexcept {
    if (state_.selected_room <= 0) return false;
    --state_.selected_room;
    return true;
}

bool PlayableShelterSession::select_next_room() noexcept {
    if (state_.selected_room + 1 >= state_.rooms) return false;
    ++state_.selected_room;
    return true;
}

BuildResult PlayableShelterSession::build_room() noexcept {
    constexpr int kRoomCost = 100;
    if (state_.rooms >= kPlayableMaxRooms) return BuildResult::Full;
    if (state_.credits < kRoomCost) {
        return BuildResult::NotEnoughCredits;
    }
    state_.credits -= kRoomCost;
    ++state_.rooms;
    state_.selected_room = state_.rooms - 1;
    return BuildResult::Built;
}

void PlayableShelterSession::assign_selected_room() noexcept {
    state_.assigned_room = state_.selected_room;
}

CollectResult PlayableShelterSession::collect_selected_room() noexcept {
    const std::size_t room =
        static_cast<std::size_t>(state_.selected_room);
    const int amount = state_.stored[room];
    if (amount <= 0) return CollectResult::NothingStored;
    state_.stored[room] = 0;
    switch (state_.selected_room) {
        case 0:
            state_.power = std::min(100, state_.power + amount);
            break;
        case 1:
            state_.food = std::min(100, state_.food + amount);
            break;
        case 2:
            state_.water = std::min(100, state_.water + amount);
            break;
        default:
            state_.credits += amount * 2;
            break;
    }
    state_.credits += amount * 3;
    return CollectResult::Collected;
}

void PlayableShelterSession::fixed_step() noexcept {
    if (state_.assigned_room < 0 ||
        state_.assigned_room >= state_.rooms) {
        return;
    }
    const std::size_t room =
        static_cast<std::size_t>(state_.assigned_room);
    ++state_.production_steps[room];
    if (state_.production_steps[room] >=
        kPlayableProductionCycleSteps) {
        state_.production_steps[room] = 0;
        state_.stored[room] = std::min(
            kPlayableStorageCapacity, state_.stored[room] + 5);
    }
}

int PlayableShelterSession::selected_stored() const noexcept {
    return state_.stored[static_cast<std::size_t>(
        state_.selected_room)];
}

int PlayableShelterSession::selected_progress() const noexcept {
    return state_.production_steps[static_cast<std::size_t>(
        state_.selected_room)];
}

bool PlayableShelterSession::selected_has_worker() const noexcept {
    return state_.assigned_room == state_.selected_room;
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

bool valid_playable_state(const PlayableShelterState& state) noexcept {
    if (state.credits < 0 ||
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
    return true;
}

std::vector<std::uint8_t> encode_playable_state(
    const PlayableShelterState& state) {
    if (!valid_playable_state(state)) return {};

    std::vector<std::uint8_t> payload;
    payload.reserve(kPlayableSavePayloadSize);
    append_i32(payload, state.credits);
    append_i32(payload, state.power);
    append_i32(payload, state.food);
    append_i32(payload, state.water);
    append_i32(payload, state.rooms);
    append_i32(payload, state.selected_room);
    append_i32(payload, state.assigned_room);
    for (const int value : state.stored) append_i32(payload, value);
    for (const int value : state.production_steps) {
        append_i32(payload, value);
    }

    std::vector<std::uint8_t> output;
    output.reserve(kPlayableSaveHeaderSize + payload.size());
    append_u32(output, kPlayableSaveMagic);
    append_u16(output, kPlayableSaveVersion);
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
        version != kPlayableSaveVersion ||
        reserved != 0u ||
        payload_size != kPlayableSavePayloadSize ||
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
    std::size_t body_offset = 0u;
    PlayableShelterState state;
    if (!read_i32(body, body_offset, state.credits) ||
        !read_i32(body, body_offset, state.power) ||
        !read_i32(body, body_offset, state.food) ||
        !read_i32(body, body_offset, state.water) ||
        !read_i32(body, body_offset, state.rooms) ||
        !read_i32(body, body_offset, state.selected_room) ||
        !read_i32(body, body_offset, state.assigned_room)) {
        result.status = PlayableSaveStatus::Corrupt;
        return result;
    }
    for (int& value : state.stored) {
        if (!read_i32(body, body_offset, value)) {
            result.status = PlayableSaveStatus::Corrupt;
            return result;
        }
    }
    for (int& value : state.production_steps) {
        if (!read_i32(body, body_offset, value)) {
            result.status = PlayableSaveStatus::Corrupt;
            return result;
        }
    }
    if (body_offset != body.size() || !valid_playable_state(state)) {
        result.status = PlayableSaveStatus::Corrupt;
        return result;
    }
    result.status = PlayableSaveStatus::Ok;
    result.state = state;
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

    std::remove(backup.c_str());
    errno = 0;
    if (std::rename(main.c_str(), backup.c_str()) != 0 &&
        errno != ENOENT) {
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
