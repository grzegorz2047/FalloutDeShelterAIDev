from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    file_path = Path(path)
    text = file_path.read_text()
    if old not in text:
        raise SystemExit(f"expected block missing in {path}: {old[:80]!r}")
    file_path.write_text(text.replace(old, new, 1))


replace_once(
    "include/gameplay/PlayableShelterSession.hpp",
    """struct PlayableRoomEntry {\n    bool active = false;\n    PlayableRoomType type = PlayableRoomType::Power;\n""",
    """struct PlayableRoomEntry {\n    bool active = false;\n    PlayableRoomType type = PlayableRoomType::Power;\n    std::uint32_t catalog_key = 0;\n""",
)
replace_once(
    "include/gameplay/PlayableShelterSession.hpp",
    """    PlayableRoomType selected_build_type = PlayableRoomType::Power;\n    int build_cursor_column = 1;\n""",
    """    PlayableRoomType selected_build_type = PlayableRoomType::Power;\n    std::uint32_t selected_build_key = 0;\n    int build_cursor_column = 1;\n""",
)
replace_once(
    "include/gameplay/PlayableShelterSession.hpp",
    """    bool migrated_from_v1 = false;\n    bool migrated_from_v2 = false;\n""",
    """    bool migrated_from_v1 = false;\n    bool migrated_from_v2 = false;\n    bool migrated_from_v3 = false;\n""",
)

replace_once(
    "source/PlayableShelterSession.cpp",
    """#include \"persistence/SaveData.hpp\"\n""",
    """#include \"persistence/SaveData.hpp\"\n#include \"rooms/RoomCatalog.hpp\"\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """constexpr std::uint16_t kPlayableSaveVersionV3 = 3u;\nconstexpr std::size_t kPlayableSaveHeaderSize = 16u;\n""",
    """constexpr std::uint16_t kPlayableSaveVersionV3 = 3u;\nconstexpr std::uint16_t kPlayableSaveVersionV4 = 4u;\nconstexpr std::size_t kPlayableSaveHeaderSize = 16u;\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """constexpr std::size_t kPlayableSavePayloadSizeV3 =\n    kPlayableSavePayloadSizeV2 + sizeof(std::uint64_t) +\n    static_cast<std::size_t>(kPlayableRoomCapacity) *\n        (sizeof(std::uint64_t) * 2u + sizeof(std::int32_t));\n""",
    """constexpr std::size_t kPlayableSavePayloadSizeV3 =\n    kPlayableSavePayloadSizeV2 + sizeof(std::uint64_t) +\n    static_cast<std::size_t>(kPlayableRoomCapacity) *\n        (sizeof(std::uint64_t) * 2u + sizeof(std::int32_t));\nconstexpr std::size_t kPlayableSavePayloadSizeV4 =\n    kPlayableSavePayloadSizeV3 + sizeof(std::uint32_t) +\n    static_cast<std::size_t>(kPlayableRoomCapacity) * sizeof(std::uint32_t);\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """int room_cost(PlayableRoomType type) noexcept {\n""",
    """std::uint32_t legacy_catalog_key(PlayableRoomType type) noexcept {\n    using rooms::room_stable_key;\n    switch (type) {\n        case PlayableRoomType::Power:\n            return room_stable_key(\"room.power_generator\");\n        case PlayableRoomType::Food:\n            return room_stable_key(\"room.kitchen\");\n        case PlayableRoomType::Water:\n            return room_stable_key(\"room.water_purifier\");\n        case PlayableRoomType::Workshop:\n            return room_stable_key(\"room.utility_tunnel\");\n        case PlayableRoomType::Living:\n            return room_stable_key(\"room.living_quarters\");\n        case PlayableRoomType::Elevator:\n            return room_stable_key(\"room.elevator\");\n    }\n    return 0u;\n}\n\nint room_cost(PlayableRoomType type) noexcept {\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """            if (room.segment_id == 0) room.segment_id = state.next_segment_id++;\n            room.group_id = room.segment_id;\n""",
    """            if (room.segment_id == 0) room.segment_id = state.next_segment_id++;\n            if (room.catalog_key == 0) room.catalog_key = legacy_catalog_key(room.type);\n            room.group_id = room.segment_id;\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """                if (candidate.type != first.type || candidate.level != first.level) break;\n""",
    """                if (candidate.catalog_key != first.catalog_key ||\n                    candidate.level != first.level) break;\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """    if (!has_resident) seed_residents(state);\n""",
    """    if (state.selected_build_key == 0) {\n        state.selected_build_key = legacy_catalog_key(state.selected_build_type);\n    }\n    if (!has_resident) seed_residents(state);\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """bool read_v3_payload(const std::vector<std::uint8_t>& body,\n                      PlayableShelterState& state) noexcept {\n""",
    """bool read_v3_payload(const std::vector<std::uint8_t>& body,\n                      PlayableShelterState& state) noexcept {\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """    return offset == body.size() && valid_playable_state(state);\n}\n\n}  // namespace\n""",
    """    if (offset != body.size()) return false;\n    state.selected_build_key = legacy_catalog_key(state.selected_build_type);\n    for (auto& room : state.room_entries) {\n        if (room.active) room.catalog_key = legacy_catalog_key(room.type);\n    }\n    normalize_room_groups(state);\n    return valid_playable_state(state);\n}\n\nbool read_v4_payload(const std::vector<std::uint8_t>& body,\n                     PlayableShelterState& state) noexcept {\n    std::size_t offset = 0u;\n    if (!read_i32(body, offset, state.credits) ||\n        !read_i32(body, offset, state.power) ||\n        !read_i32(body, offset, state.food) ||\n        !read_i32(body, offset, state.water) ||\n        !read_i32(body, offset, state.rooms) ||\n        !read_i32(body, offset, state.selected_room) ||\n        !read_i32(body, offset, state.assigned_room) ||\n        !read_enum(body, offset, state.selected_build_type) ||\n        !read_u32(body, offset, state.selected_build_key) ||\n        !read_i32(body, offset, state.build_cursor_column) ||\n        !read_i32(body, offset, state.build_cursor_floor) ||\n        !read_u64(body, offset, state.next_segment_id)) return false;\n    for (int& value : state.stored) if (!read_i32(body, offset, value)) return false;\n    for (int& value : state.production_steps) if (!read_i32(body, offset, value)) return false;\n    for (auto& room : state.room_entries) {\n        int active = 0;\n        if (!read_i32(body, offset, active) || (active != 0 && active != 1) ||\n            !read_enum(body, offset, room.type) ||\n            !read_u32(body, offset, room.catalog_key) ||\n            !read_i32(body, offset, room.column) ||\n            !read_i32(body, offset, room.floor) ||\n            !read_i32(body, offset, room.stored) ||\n            !read_i32(body, offset, room.production_steps) ||\n            !read_u64(body, offset, room.segment_id) ||\n            !read_u64(body, offset, room.group_id) ||\n            !read_i32(body, offset, room.level)) return false;\n        room.active = active != 0;\n    }\n    for (auto& resident : state.residents) {\n        int active = 0;\n        if (!read_i32(body, offset, active) || (active != 0 && active != 1) ||\n            !read_i32(body, offset, resident.current_column) ||\n            !read_i32(body, offset, resident.current_floor) ||\n            !read_i32(body, offset, resident.next_column) ||\n            !read_i32(body, offset, resident.next_floor) ||\n            !read_i32(body, offset, resident.destination_column) ||\n            !read_i32(body, offset, resident.destination_floor) ||\n            !read_i32(body, offset, resident.assigned_room) ||\n            !read_enum(body, offset, resident.state) ||\n            !read_i32(body, offset, resident.movement_ticks) ||\n            !read_i32(body, offset, resident.roaming_ticks) ||\n            !read_i32(body, offset, resident.roaming_sequence)) return false;\n        resident.active = active != 0;\n    }\n    return offset == body.size() && valid_playable_state(state);\n}\n\n}  // namespace\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """    state_.selected_build_type = type;\n    return true;\n""",
    """    state_.selected_build_type = type;\n    state_.selected_build_key = legacy_catalog_key(type);\n    return true;\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """    room.type = state_.selected_build_type;\n    room.column = state_.build_cursor_column;\n""",
    """    room.type = state_.selected_build_type;\n    room.catalog_key = state_.selected_build_key;\n    room.column = state_.build_cursor_column;\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """        !valid_room_type(state.selected_build_type) ||\n        !in_grid(\n""",
    """        !valid_room_type(state.selected_build_type) ||\n        state.selected_build_key == 0 ||\n        !in_grid(\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """        if (!valid_room_type(room.type) ||\n            !in_grid(room.column, room.floor) ||\n""",
    """        if (!valid_room_type(room.type) ||\n            room.catalog_key == 0 ||\n            !in_grid(room.column, room.floor) ||\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """    payload.reserve(kPlayableSavePayloadSizeV3);\n""",
    """    payload.reserve(kPlayableSavePayloadSizeV4);\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """    append_i32(payload, static_cast<int>(state.selected_build_type));\n    append_i32(payload, state.build_cursor_column);\n""",
    """    append_i32(payload, static_cast<int>(state.selected_build_type));\n    append_u32(payload, state.selected_build_key);\n    append_i32(payload, state.build_cursor_column);\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """        append_i32(payload, static_cast<int>(room.type));\n        append_i32(payload, room.column);\n""",
    """        append_i32(payload, static_cast<int>(room.type));\n        append_u32(payload, room.catalog_key);\n        append_i32(payload, room.column);\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """    if (payload.size() != kPlayableSavePayloadSizeV3) return {};\n""",
    """    if (payload.size() != kPlayableSavePayloadSizeV4) return {};\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """    append_u16(output, kPlayableSaveVersionV3);\n""",
    """    append_u16(output, kPlayableSaveVersionV4);\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """         version != kPlayableSaveVersionV2 &&\n         version != kPlayableSaveVersionV3) ||\n""",
    """         version != kPlayableSaveVersionV2 &&\n         version != kPlayableSaveVersionV3 &&\n         version != kPlayableSaveVersionV4) ||\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """        (version == kPlayableSaveVersionV3 &&\n         payload_size != kPlayableSavePayloadSizeV3) ||\n""",
    """        (version == kPlayableSaveVersionV3 &&\n         payload_size != kPlayableSavePayloadSizeV3) ||\n        (version == kPlayableSaveVersionV4 &&\n         payload_size != kPlayableSavePayloadSizeV4) ||\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """             ? read_v1_payload(body, state)\n             : (version == kPlayableSaveVersionV2\n                    ? read_v2_payload(body, state)\n                    : read_v3_payload(body, state));\n""",
    """             ? read_v1_payload(body, state)\n             : (version == kPlayableSaveVersionV2\n                    ? read_v2_payload(body, state)\n                    : (version == kPlayableSaveVersionV3\n                           ? read_v3_payload(body, state)\n                           : read_v4_payload(body, state)));\n""",
)
replace_once(
    "source/PlayableShelterSession.cpp",
    """    result.migrated_from_v2 = version == kPlayableSaveVersionV2;\n    return result;\n""",
    """    result.migrated_from_v2 = version == kPlayableSaveVersionV2;\n    result.migrated_from_v3 = version == kPlayableSaveVersionV3;\n    return result;\n""",
)

replace_once(
    "tests/playable_shelter_session_tests.cpp",
    """    assert(encoded_v3[4] == 3u && encoded_v3[5] == 0u);\n""",
    """    assert(encoded_v3[4] == 4u && encoded_v3[5] == 0u);\n""",
)
replace_once(
    "tests/playable_shelter_session_tests.cpp",
    """    assert(!upgraded_roundtrip.migrated_from_v2);\n    assert(upgraded_roundtrip.state.next_segment_id == upgraded.state().next_segment_id);\n""",
    """    assert(!upgraded_roundtrip.migrated_from_v2);\n    assert(!upgraded_roundtrip.migrated_from_v3);\n    assert(upgraded_roundtrip.state.next_segment_id == upgraded.state().next_segment_id);\n    assert(upgraded_roundtrip.state.selected_build_key != 0);\n""",
)
replace_once(
    "tests/playable_shelter_session_tests.cpp",
    """        assert(after.segment_id == before.segment_id);\n        assert(after.group_id == before.group_id);\n        assert(after.level == before.level);\n""",
    """        assert(after.segment_id == before.segment_id);\n        assert(after.group_id == before.group_id);\n        assert(after.catalog_key == before.catalog_key);\n        assert(after.level == before.level);\n""",
)
replace_once(
    "tests/playable_shelter_session_tests.cpp",
    """    auto corrupt = encoded_v3;\n""",
    """    PlayableShelterState retired_state = upgraded.state();\n    retired_state.room_entries[1].catalog_key = 0xdeadbeefu;\n    const auto retired_bytes = encode_playable_state(retired_state);\n    assert(!retired_bytes.empty());\n    const auto retired_roundtrip = decode_playable_state(retired_bytes);\n    assert(retired_roundtrip.status == PlayableSaveStatus::Ok);\n    assert(retired_roundtrip.state.room_entries[1].catalog_key == 0xdeadbeefu);\n\n    PlayableShelterState split_identity = upgraded.state();\n    split_identity.room_entries[0].type = PlayableRoomType::Workshop;\n    split_identity.room_entries[1].type = PlayableRoomType::Workshop;\n    split_identity.room_entries[0].catalog_key = 0x11111111u;\n    split_identity.room_entries[1].catalog_key = 0x22222222u;\n    split_identity.room_entries[0].column = 0;\n    split_identity.room_entries[1].column = 1;\n    PlayableShelterSession split_session(split_identity);\n    assert(split_session.state().room_entries[0].group_id !=\n           split_session.state().room_entries[1].group_id);\n\n    auto corrupt = encoded_v3;\n""",
)
