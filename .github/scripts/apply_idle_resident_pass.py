from pathlib import Path

path = Path("source/Scene3DRenderer.cpp")
text = path.read_text()
needle = """    for (int floor_index = 0; floor_index < 3; ++floor_index) {
        const float door_y = layout::kRoomY[floor_index * 2] + 9.0f;
        mesh_.append_box({layout::kElevatorX + 4.0f, door_y, -4.0f,
                          layout::kElevatorWidth - 8.0f, 43.0f, 6.0f,
                          floor_index == 1 ? rgba(117, 79, 35) : steel_mid,
                          assets::GeneratedMaterial::ControlPanel});
        mesh_.append_box({layout::kElevatorX + 10.0f, door_y + 8.0f, -1.0f,
                          layout::kElevatorWidth - 20.0f, 25.0f, 3.0f,
                          floor_index == 1 ? rgba(166, 122, 57) : rgba(82, 93, 90),
                          assets::GeneratedMaterial::Steel});
    }
"""
addition = needle + """    if (!state.resident_assigned) {
        constexpr float idle_y = layout::kRoomY[2] + 14.0f;
        mesh_.append_box({layout::kElevatorX + 11.0f, idle_y, -1.0f,
                          8.0f, 9.0f, 6.0f,
                          rgba(245, 210, 174), assets::GeneratedMaterial::Steel});
        mesh_.append_box({layout::kElevatorX + 8.0f, idle_y + 9.0f, -1.0f,
                          14.0f, 24.0f, 6.0f,
                          rgba(48, 119, 164), assets::GeneratedMaterial::ControlPanel});
    }
"""
if needle not in text:
    raise SystemExit("expected elevator block not found")
path.write_text(text.replace(needle, addition, 1))
