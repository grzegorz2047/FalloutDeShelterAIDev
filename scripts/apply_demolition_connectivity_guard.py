from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    file_path = Path(path)
    text = file_path.read_text()
    if old not in text:
        raise SystemExit(f"expected block missing in {path}")
    file_path.write_text(text.replace(old, new, 1))


replace_once(
    "include/gameplay/PlayableShelterSession.hpp",
    """    UnsafeProduction,\n    LastRoom,\n""",
    """    UnsafeProduction,\n    DisconnectedShelter,\n    LastRoom,\n""",
)

replace_once(
    "source/PlayableShelterSession.cpp",
    """bool vertical_edge_allowed(const PlayableShelterState& state,\n                           int from_column,\n                           int from_floor,\n                           int to_column,\n                           int to_floor) noexcept {\n    return from_column == to_column &&\n           std::abs(from_floor - to_floor) == 1 &&\n           is_elevator_cell(state, from_column, from_floor) &&\n           is_elevator_cell(state, to_column, to_floor);\n}\n\nbool next_route_step(const PlayableShelterState& state,\n""",
    """bool vertical_edge_allowed(const PlayableShelterState& state,\n                           int from_column,\n                           int from_floor,\n                           int to_column,\n                           int to_floor) noexcept {\n    return from_column == to_column &&\n           std::abs(from_floor - to_floor) == 1 &&\n           is_elevator_cell(state, from_column, from_floor) &&\n           is_elevator_cell(state, to_column, to_floor);\n}\n\nint connected_component_count(const PlayableShelterState& state) noexcept {\n    constexpr int kCellCount =\n        kPlayableGridColumns * kPlayableGridFloors;\n    std::array<bool, kCellCount> visited{};\n    std::array<int, kCellCount> queue{};\n    constexpr std::array<int, 4> kColumnDelta{{-1, 1, 0, 0}};\n    constexpr std::array<int, 4> kFloorDelta{{0, 0, -1, 1}};\n    int components = 0;\n\n    for (int floor = 0; floor < kPlayableGridFloors; ++floor) {\n        for (int column = 0; column < kPlayableGridColumns; ++column) {\n            const int start = grid_index(column, floor);\n            if (visited[static_cast<std::size_t>(start)] ||\n                !traversable_cell(state, column, floor)) {\n                continue;\n            }\n            ++components;\n            int head = 0;\n            int tail = 0;\n            queue[static_cast<std::size_t>(tail++)] = start;\n            visited[static_cast<std::size_t>(start)] = true;\n            while (head < tail) {\n                const int current = queue[static_cast<std::size_t>(head++)];\n                const int current_column = current % kPlayableGridColumns;\n                const int current_floor = current / kPlayableGridColumns;\n                for (std::size_t direction = 0;\n                     direction < kColumnDelta.size(); ++direction) {\n                    const int candidate_column =\n                        current_column + kColumnDelta[direction];\n                    const int candidate_floor =\n                        current_floor + kFloorDelta[direction];\n                    if (!in_grid(candidate_column, candidate_floor) ||\n                        !traversable_cell(\n                            state, candidate_column, candidate_floor)) {\n                        continue;\n                    }\n                    if (candidate_floor != current_floor &&\n                        !vertical_edge_allowed(\n                            state, current_column, current_floor,\n                            candidate_column, candidate_floor)) {\n                        continue;\n                    }\n                    const int candidate =\n                        grid_index(candidate_column, candidate_floor);\n                    if (visited[static_cast<std::size_t>(candidate)]) {\n                        continue;\n                    }\n                    visited[static_cast<std::size_t>(candidate)] = true;\n                    queue[static_cast<std::size_t>(tail++)] = candidate;\n                }\n            }\n        }\n    }\n    return components;\n}\n\nbool next_route_step(const PlayableShelterState& state,\n""",
)

replace_once(
    "source/PlayableShelterSession.cpp",
    """    PlayableShelterState candidate = state_;\n    candidate.room_entries[static_cast<std::size_t>(selected_index)] = {};\n    normalize_room_groups(candidate);\n    for (const auto& resident : state_.residents) {\n""",
    """    PlayableShelterState candidate = state_;\n    candidate.room_entries[static_cast<std::size_t>(selected_index)] = {};\n    normalize_room_groups(candidate);\n    const bool disconnects_shelter =\n        connected_component_count(candidate) >\n        connected_component_count(state_);\n    for (const auto& resident : state_.residents) {\n""",
)

replace_once(
    "source/PlayableShelterSession.cpp",
    """    } else if (preview.production_steps_affected > 0) {\n        preview.result = RoomLifecycleResult::UnsafeProduction;\n    } else {\n        preview.result = RoomLifecycleResult::Applied;\n    }\n""",
    """    } else if (preview.production_steps_affected > 0) {\n        preview.result = RoomLifecycleResult::UnsafeProduction;\n    } else if (disconnects_shelter) {\n        preview.result = RoomLifecycleResult::DisconnectedShelter;\n    } else {\n        preview.result = RoomLifecycleResult::Applied;\n    }\n""",
)

replace_once(
    "source/main.cpp",
    """        case RoomLifecycleResult::UnsafeProduction:\n            return \"Najpierw zakoncz aktywna produkcje.\";\n        case RoomLifecycleResult::LastRoom:\n""",
    """        case RoomLifecycleResult::UnsafeProduction:\n            return \"Najpierw zakoncz aktywna produkcje.\";\n        case RoomLifecycleResult::DisconnectedShelter:\n            return \"Ten segment laczy czesci schronu. Zbuduj inne przejscie.\";\n        case RoomLifecycleResult::LastRoom:\n""",
)

replace_once(
    "tests/playable_shelter_session_tests.cpp",
    """    assert(blocked.state().room_entries[static_cast<std::size_t>(blocked_room)].active);\n    assert(blocked.state().room_entries[static_cast<std::size_t>(blocked_room)].stored == 1);\n    assert(valid_playable_state(blocked.state()));\n}\n""",
    """    assert(blocked.state().room_entries[static_cast<std::size_t>(blocked_room)].active);\n    assert(blocked.state().room_entries[static_cast<std::size_t>(blocked_room)].stored == 1);\n    assert(valid_playable_state(blocked.state()));\n\n    PlayableShelterState bridge_state;\n    bridge_state.credits = 1000;\n    PlayableShelterSession bridge_builder(bridge_state);\n    assert(bridge_builder.select_build_type(PlayableRoomType::Workshop));\n    assert(bridge_builder.set_build_cursor(1, 0));\n    assert(bridge_builder.confirm_build() == BuildResult::Built);\n    assert(bridge_builder.set_build_cursor(2, 0));\n    assert(bridge_builder.confirm_build() == BuildResult::Built);\n    const int bridge_room = bridge_builder.room_index_at(1, 0);\n    assert(bridge_room >= 0);\n    PlayableShelterState bridge_selected = bridge_builder.state();\n    bridge_selected.selected_room = bridge_room;\n    PlayableShelterSession guarded_bridge(bridge_selected);\n    const int bridge_credits = guarded_bridge.state().credits;\n    const auto bridge_preview = guarded_bridge.preview_demolish_selected();\n    assert(bridge_preview.result ==\n           RoomLifecycleResult::DisconnectedShelter);\n    assert(guarded_bridge.confirm_demolish_selected() ==\n           RoomLifecycleResult::DisconnectedShelter);\n    assert(guarded_bridge.state().credits == bridge_credits);\n    assert(guarded_bridge.room_index_at(1, 0) == bridge_room);\n    assert(guarded_bridge.room_index_at(2, 0) >= 0);\n    assert(valid_playable_state(guarded_bridge.state()));\n}\n""",
)
