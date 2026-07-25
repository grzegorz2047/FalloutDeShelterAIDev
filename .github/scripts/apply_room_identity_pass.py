from pathlib import Path
p=Path('source/Scene3DRenderer.cpp')
s=p.read_text()
old='''    const u32 frame = selected ? rgba(201, 145, 48) : rgba(63, 72, 72);
    const u32 accent = room_accent(room_index);
    const u32 wall = room_back_wall(room_index);
    const u32 floor = room_floor(room_index);

    constexpr float wall_inset = 5.0f;
    constexpr float frame_depth = 18.0f;
    constexpr float inner_width = layout::kRoomWidth - 2.0f * wall_inset;
    constexpr float inner_height = layout::kRoomHeight - 2.0f * wall_inset;

    mesh_.append_box({x + wall_inset, y + wall_inset, -18.0f,
                      inner_width, inner_height, 5.0f, wall,
                      assets::GeneratedMaterial::Steel});
    mesh_.append_box({x, y, -14.0f, layout::kRoomWidth, 5.0f, frame_depth, frame,
                      assets::GeneratedMaterial::Steel});
    mesh_.append_box({x, y + layout::kRoomHeight - 5.0f, -14.0f,
                      layout::kRoomWidth, 5.0f, frame_depth, frame,
                      assets::GeneratedMaterial::Steel});
    mesh_.append_box({x, y, -14.0f, 5.0f, layout::kRoomHeight, frame_depth, frame,
                      assets::GeneratedMaterial::Steel});
    mesh_.append_box({x + layout::kRoomWidth - 5.0f, y, -14.0f,
                      5.0f, layout::kRoomHeight, frame_depth, frame,
                      assets::GeneratedMaterial::Steel});
    mesh_.append_box({x + 5.0f, y + layout::kRoomHeight - 13.0f, -9.0f,
                      layout::kRoomWidth - 10.0f, 8.0f, 13.0f, floor,
                      assets::GeneratedMaterial::Grating});
'''
new='''    const int profile = (room_index % 6 + 6) % 6;
    const u32 frame = selected ? rgba(162, 113, 38) : rgba(55, 65, 66);
    const u32 accent = room_accent(room_index);
    const u32 wall = room_back_wall(room_index);
    const u32 floor = room_floor(room_index);
    const float left_post = profile == 3 ? 9.0f : (profile == 5 ? 7.0f : 5.0f);
    const float right_post = profile == 2 ? 9.0f : (profile == 4 ? 7.0f : 5.0f);
    const float canopy_inset = profile == 1 ? 16.0f : (profile == 5 ? 11.0f : 7.0f);
    const float canopy_height = profile == 0 ? 7.0f : 4.0f;

    constexpr float wall_inset = 5.0f;
    constexpr float frame_depth = 18.0f;
    constexpr float inner_width = layout::kRoomWidth - 2.0f * wall_inset;
    constexpr float inner_height = layout::kRoomHeight - 2.0f * wall_inset;

    mesh_.append_box({x + wall_inset, y + wall_inset, -18.0f,
                      inner_width, inner_height, 5.0f, wall,
                      assets::GeneratedMaterial::Steel});
    mesh_.append_box({x + canopy_inset, y + 1.0f, -11.0f,
                      layout::kRoomWidth - 2.0f * canopy_inset, canopy_height, 12.0f,
                      selected ? rgba(229, 180, 76) : accent,
                      assets::GeneratedMaterial::ControlPanel});
    mesh_.append_box({x + 2.0f, y + layout::kRoomHeight - 6.0f, -14.0f,
                      layout::kRoomWidth - 4.0f, 6.0f, frame_depth, frame,
                      assets::GeneratedMaterial::Steel});
    mesh_.append_box({x, y + 4.0f, -14.0f, left_post,
                      layout::kRoomHeight - 4.0f, frame_depth, frame,
                      assets::GeneratedMaterial::Steel});
    mesh_.append_box({x + layout::kRoomWidth - right_post, y, -14.0f,
                      right_post, layout::kRoomHeight - 7.0f, frame_depth, frame,
                      assets::GeneratedMaterial::Steel});
    mesh_.append_box({x + left_post, y + layout::kRoomHeight - 14.0f, -9.0f,
                      layout::kRoomWidth - left_post - right_post, 8.0f, 13.0f, floor,
                      assets::GeneratedMaterial::Grating});
'''
if old not in s: raise SystemExit('base block missing')
s=s.replace(old,new)
repls={
'''            mesh_.append_box({x + 12.0f, y + 20.0f, -8.0f, 31.0f, 31.0f, 11.0f,
                              accent, assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 50.0f, y + 14.0f, -7.0f, 22.0f, 37.0f, 10.0f,
                              rgba(106, 93, 63), assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 80.0f, y + 22.0f, -8.0f, 38.0f, 29.0f, 11.0f,
                              accent, assets::GeneratedMaterial::ControlPanel});''':
'''            mesh_.append_box({x + 10.0f, y + 25.0f, -8.0f, 28.0f, 26.0f, 11.0f,
                              rgba(145, 105, 42), assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 43.0f, y + 12.0f, -9.0f, 48.0f, 39.0f, 12.0f,
                              accent, assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 96.0f, y + 27.0f, -8.0f, 25.0f, 24.0f, 11.0f,
                              rgba(145, 105, 42), assets::GeneratedMaterial::ControlPanel});''',
'''            mesh_.append_box({x + 11.0f, y + 39.0f, -7.0f, 110.0f, 12.0f, 9.0f,
                              rgba(105, 181, 103), assets::GeneratedMaterial::Grating});
            mesh_.append_box({x + 19.0f, y + 24.0f, -4.0f, 16.0f, 15.0f, 5.0f,
                              accent, assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 57.0f, y + 19.0f, -4.0f, 18.0f, 20.0f, 5.0f,
                              accent, assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 96.0f, y + 26.0f, -4.0f, 14.0f, 13.0f, 5.0f,
                              accent, assets::GeneratedMaterial::ControlPanel});''':
'''            mesh_.append_box({x + 10.0f, y + 35.0f, -8.0f, 112.0f, 16.0f, 10.0f,
                              rgba(73, 132, 78), assets::GeneratedMaterial::Grating});
            mesh_.append_box({x + 19.0f, y + 18.0f, -5.0f, 24.0f, 17.0f, 6.0f,
                              accent, assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 51.0f, y + 13.0f, -5.0f, 31.0f, 22.0f, 6.0f,
                              rgba(145, 224, 137), assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 91.0f, y + 20.0f, -5.0f, 22.0f, 15.0f, 6.0f,
                              accent, assets::GeneratedMaterial::ControlPanel});''',
'''            mesh_.append_box({x + 12.0f, y + 15.0f, -9.0f, 32.0f, 36.0f, 11.0f,
                              accent, assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 52.0f, y + 26.0f, -8.0f, 57.0f, 25.0f, 10.0f,
                              rgba(103, 190, 222), assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 66.0f, y + 10.0f, -5.0f, 8.0f, 16.0f, 5.0f,
                              rgba(191, 229, 237), assets::GeneratedMaterial::Steel});''':
'''            mesh_.append_box({x + 11.0f, y + 24.0f, -8.0f, 27.0f, 27.0f, 11.0f,
                              rgba(55, 126, 153), assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 43.0f, y + 11.0f, -10.0f, 68.0f, 40.0f, 13.0f,
                              accent, assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 67.0f, y + 5.0f, -5.0f, 11.0f, 9.0f, 5.0f,
                              rgba(191, 229, 237), assets::GeneratedMaterial::ControlPanel});''',
'''            mesh_.append_box({x + 10.0f, y + 38.0f, -8.0f, 112.0f, 13.0f, 10.0f,
                              rgba(165, 130, 94), assets::GeneratedMaterial::Grating});
            mesh_.append_box({x + 18.0f, y + 20.0f, -6.0f, 36.0f, 18.0f, 8.0f,
                              accent, assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 79.0f, y + 15.0f, -7.0f, 29.0f, 23.0f, 9.0f,
                              rgba(222, 188, 123), assets::GeneratedMaterial::Steel});''':
'''            mesh_.append_box({x + 13.0f, y + 39.0f, -8.0f, 108.0f, 12.0f, 10.0f,
                              rgba(132, 99, 69), assets::GeneratedMaterial::Grating});
            mesh_.append_box({x + 18.0f, y + 23.0f, -6.0f, 43.0f, 16.0f, 8.0f,
                              rgba(162, 91, 47), assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 77.0f, y + 10.0f, -9.0f, 34.0f, 29.0f, 11.0f,
                              accent, assets::GeneratedMaterial::Steel});''',
'''            for (int crate = 0; crate < 5; ++crate) {
                mesh_.append_box({x + 10.0f + crate * 23.0f, y + 31.0f, -7.0f,
                                  18.0f, 20.0f, 8.0f, accent,
                                  assets::GeneratedMaterial::Steel});
            }
            mesh_.append_box({x + 17.0f, y + 16.0f, -5.0f, 98.0f, 6.0f, 5.0f,
                              rgba(205, 211, 183), assets::GeneratedMaterial::Grating});''':
'''            mesh_.append_box({x + 12.0f, y + 33.0f, -8.0f, 26.0f, 18.0f, 9.0f,
                              accent, assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 41.0f, y + 20.0f, -9.0f, 31.0f, 31.0f, 10.0f,
                              rgba(177, 143, 72), assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 75.0f, y + 28.0f, -8.0f, 21.0f, 23.0f, 9.0f,
                              accent, assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 99.0f, y + 15.0f, -9.0f, 23.0f, 36.0f, 10.0f,
                              rgba(177, 143, 72), assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 18.0f, y + 11.0f, -5.0f, 96.0f, 5.0f, 5.0f,
                              rgba(220, 205, 153), assets::GeneratedMaterial::Grating});''',
'''            mesh_.append_box({x + 10.0f, y + 37.0f, -7.0f, 48.0f, 14.0f, 8.0f,
                              rgba(183, 145, 111), assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 74.0f, y + 36.0f, -7.0f, 45.0f, 15.0f, 8.0f,
                              accent, assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 54.0f, y + 20.0f, -5.0f, 23.0f, 12.0f, 6.0f,
                              rgba(237, 217, 166), assets::GeneratedMaterial::ControlPanel});''':
'''            mesh_.append_box({x + 11.0f, y + 34.0f, -8.0f, 70.0f, 17.0f, 9.0f,
                              rgba(164, 119, 92), assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 84.0f, y + 22.0f, -8.0f, 35.0f, 29.0f, 9.0f,
                              accent, assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 18.0f, y + 15.0f, -5.0f, 21.0f, 18.0f, 6.0f,
                              rgba(237, 217, 166), assets::GeneratedMaterial::ControlPanel});'''
}
for a,b in repls.items():
    if a not in s: raise SystemExit('prop block missing')
    s=s.replace(a,b)
s=s.replace('{22.0f, 5.0f, -28.0f, 118.0f, 22.0f, 17.0f,','{18.0f, 9.0f, -28.0f, 126.0f, 18.0f, 17.0f,')
s=s.replace('{132.0f, 1.0f, -31.0f, 139.0f, 18.0f, 20.0f,','{127.0f, -4.0f, -31.0f, 151.0f, 17.0f, 20.0f,')
s=s.replace('{263.0f, 7.0f, -27.0f, 116.0f, 23.0f, 16.0f,','{272.0f, 8.0f, -27.0f, 109.0f, 18.0f, 16.0f,')
s=s.replace('{18.0f, 218.0f, -30.0f, 151.0f, 19.0f, 19.0f,','{11.0f, 220.0f, -30.0f, 161.0f, 18.0f, 19.0f,')
s=s.replace('{160.0f, 221.0f, -26.0f, 128.0f, 16.0f, 15.0f,','{154.0f, 226.0f, -26.0f, 139.0f, 12.0f, 15.0f,')
s=s.replace('{278.0f, 216.0f, -31.0f, 104.0f, 22.0f, 20.0f,','{287.0f, 214.0f, -31.0f, 98.0f, 24.0f, 20.0f,')
p.write_text(s)
