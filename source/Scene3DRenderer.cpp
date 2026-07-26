#include "render/Scene3DRenderer.hpp"

#include <algorithm>
#include <cstring>

#include "assets/GeneratedMaterialAtlas.hpp"
#include "assets/RoomAssetAtlas.hpp"
#include "render/RoomVisualProfile.hpp"
#include "render/ShelterSceneLayout.hpp"
#include "render/ShelterView3D.hpp"
#include "room_assets_bin.h"
#include "scene3d_v_shbin.h"

namespace deep_shelter::render {
namespace {

constexpr u32 rgba(u8 r, u8 g, u8 b, u8 a = 255) noexcept {
    return static_cast<u32>(r) | (static_cast<u32>(g) << 8) |
           (static_cast<u32>(b) << 16) | (static_cast<u32>(a) << 24);
}

constexpr int normalized_visual_profile(int visual_profile) noexcept {
    return (visual_profile % 6 + 6) % 6;
}

void append_selection(SceneMesh3D& mesh,
                      float x,
                      float y,
                      float width) noexcept {
    constexpr u32 c = 0xFF56C7F0;
    constexpr float arm = 22.0f;
    constexpr float t = 3.0f;
    const float r = x + width;
    const float b = y + layout::kRoomHeight;
    mesh.append_box({x - 2.0f, y - 2.0f, -1.0f, arm, t, 3.0f, c, assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({x - 2.0f, y - 2.0f, -1.0f, t, arm, 3.0f, c, assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({r - arm + 2.0f, y - 2.0f, -1.0f, arm, t, 3.0f, c, assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({r - 1.0f, y - 2.0f, -1.0f, t, arm, 3.0f, c, assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({x - 2.0f, b - 1.0f, -1.0f, arm, t, 3.0f, c, assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({x - 2.0f, b - arm + 2.0f, -1.0f, t, arm, 3.0f, c, assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({r - arm + 2.0f, b - 1.0f, -1.0f, arm, t, 3.0f, c, assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({r - 1.0f, b - arm + 2.0f, -1.0f, t, arm, 3.0f, c, assets::GeneratedMaterial::ControlPanel});
}

void append_shell(SceneMesh3D& mesh,
                  float x,
                  float y,
                  int visual_profile) noexcept {
    const int profile = normalized_visual_profile(visual_profile);
    const auto wall_material =
        profile == 2 ? assets::GeneratedMaterial::Water
                     : (profile == 1
                            ? assets::GeneratedMaterial::Hydroponic
                            : assets::GeneratedMaterial::VaultPanel);
    const u32 wall_tint =
        profile == 2 ? rgba(190, 224, 235)
                     : (profile == 1 ? rgba(204, 225, 193)
                                     : rgba(218, 211, 192));
    // Open-front room prism: rear wall at z=-54, structural shell reaching
    // z=+3. The oblique camera exposes floor, ceiling and both side returns.
    mesh.append_box({x + 8.0f, y + 8.0f, -56.0f, 116.0f, 43.0f, 4.0f,
                     wall_tint, wall_material});
    mesh.append_box({x + 2.0f, y + 2.0f, -54.0f, 128.0f, 7.0f, 57.0f,
                     rgba(151, 159, 157), assets::GeneratedMaterial::Steel});
    mesh.append_box({x + 2.0f, y + 50.0f, -54.0f, 128.0f, 11.0f, 58.0f,
                     rgba(175, 169, 150), assets::GeneratedMaterial::Grating});
    mesh.append_box({x + 1.0f, y + 3.0f, -54.0f, 8.0f, 58.0f, 58.0f,
                     rgba(145, 151, 150), assets::GeneratedMaterial::Steel});
    mesh.append_box({x + 123.0f, y + 3.0f, -54.0f, 8.0f, 58.0f, 58.0f,
                     rgba(145, 151, 150), assets::GeneratedMaterial::Steel});
}

void append_elevator(SceneMesh3D& mesh, float x, float y) noexcept {
    mesh.append_box({x, y + 2.0f, -56.0f,
                     layout::kElevatorWidth, layout::kRoomHeight - 4.0f, 5.0f,
                     rgba(37, 46, 48), assets::GeneratedMaterial::Steel});
    mesh.append_box({x, y, -52.0f,
                     5.0f, layout::kRoomHeight, 55.0f,
                     rgba(61, 70, 70), assets::GeneratedMaterial::Steel});
    mesh.append_box({x + layout::kElevatorWidth - 5.0f, y, -52.0f,
                     5.0f, layout::kRoomHeight, 55.0f,
                     rgba(61, 70, 70), assets::GeneratedMaterial::Steel});
    mesh.append_box({x + 5.0f, y + 10.0f, -8.0f,
                     layout::kElevatorWidth - 10.0f,
                     layout::kRoomHeight - 18.0f, 8.0f,
                     rgba(75, 89, 91), assets::GeneratedMaterial::VaultPanel});
    mesh.append_box({x + 11.0f, y + 19.0f, -1.0f,
                     layout::kElevatorWidth - 22.0f,
                     layout::kRoomHeight - 36.0f, 3.0f,
                     rgba(105, 129, 127),
                     assets::GeneratedMaterial::ControlPanel});
}

void append_build_preview(SceneMesh3D& mesh,
                          float x,
                          float y,
                          float width,
                          bool valid) noexcept {
    const u32 color =
        valid ? rgba(112, 255, 177, 220) : rgba(255, 92, 78, 235);
    constexpr float thickness = 3.0f;
    constexpr float depth = 4.0f;
    mesh.append_box({x - 2.0f, y - 2.0f, 1.5f,
                     width + 4.0f, thickness, depth, color,
                     assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({x - 2.0f, y + layout::kRoomHeight - 1.0f, 1.5f,
                     width + 4.0f, thickness, depth, color,
                     assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({x - 2.0f, y + 1.0f, 1.5f,
                     thickness, layout::kRoomHeight - 2.0f, depth, color,
                     assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({x + width - 1.0f, y + 1.0f, 1.5f,
                     thickness, layout::kRoomHeight - 2.0f, depth, color,
                     assets::GeneratedMaterial::ControlPanel});
    if (!valid) {
        // A central cross differentiates an invalid target without relying on
        // red/green perception alone.
        mesh.append_box({x + width * 0.5f - 2.0f, y + 11.0f, 2.0f,
                         4.0f, layout::kRoomHeight - 22.0f, depth, color,
                         assets::GeneratedMaterial::ControlPanel});
        mesh.append_box({x + 10.0f, y + layout::kRoomHeight * 0.5f - 2.0f,
                         2.0f, width - 20.0f, 4.0f, depth, color,
                         assets::GeneratedMaterial::ControlPanel});
    }
}

void append_prop(SceneMesh3D& mesh,
                 float x,
                 float y,
                 float z,
                 float width,
                 float height,
                 assets::RoomProp prop,
                 u32 tint = 0xffffffffu) noexcept {
    mesh.append_billboard(
        {x, y, z, width, height, tint, assets::room_prop_region(prop)});
}

void append_room_light(SceneMesh3D& mesh, float x, float y) noexcept {
    append_prop(mesh, x + 33.0f, y + 6.0f, -45.0f, 66.0f, 13.0f,
                assets::RoomProp::CeilingLight);
    mesh.append_box({x + 43.0f, y + 8.0f, -43.0f, 46.0f, 2.0f, 3.0f,
                     rgba(235, 246, 224),
                     assets::GeneratedMaterial::ControlPanel});
}

void append_power(SceneMesh3D& mesh, float x, float y) noexcept {
    append_room_light(mesh, x, y);
    append_prop(mesh, x + 42.0f, y + 14.0f, -28.0f, 48.0f, 36.0f,
                assets::RoomProp::PowerGenerator);
    append_prop(mesh, x + 8.0f, y + 28.0f, -8.0f, 43.0f, 23.0f,
                assets::RoomProp::ControlConsole);
    append_prop(mesh, x + 15.0f, y + 13.0f, -20.0f, 20.0f, 29.0f,
                assets::RoomProp::Terminal);
}

void append_hydro(SceneMesh3D& mesh, float x, float y) noexcept {
    append_room_light(mesh, x, y);
    append_prop(mesh, x + 11.0f, y + 29.0f, -7.0f, 110.0f, 23.0f,
                assets::RoomProp::HydroponicPlanter);
    append_prop(mesh, x + 14.0f, y + 16.0f, -26.0f, 42.0f, 24.0f,
                assets::RoomProp::WorkTable);
    append_prop(mesh, x + 101.0f, y + 16.0f, -18.0f, 18.0f, 28.0f,
                assets::RoomProp::Terminal);
}

void append_water(SceneMesh3D& mesh, float x, float y) noexcept {
    append_room_light(mesh, x, y);
    append_prop(mesh, x + 39.0f, y + 13.0f, -25.0f, 62.0f, 38.0f,
                assets::RoomProp::WaterMachinery);
    append_prop(mesh, x + 79.0f, y + 29.0f, -6.0f, 43.0f, 22.0f,
                assets::RoomProp::ControlConsole);
    append_prop(mesh, x + 102.0f, y + 13.0f, -16.0f, 18.0f, 28.0f,
                assets::RoomProp::Terminal);
}

void append_workshop(SceneMesh3D& mesh, float x, float y) noexcept {
    append_room_light(mesh, x, y);
    append_prop(mesh, x + 9.0f, y + 28.0f, -6.0f, 66.0f, 24.0f,
                assets::RoomProp::WorkTable);
    append_prop(mesh, x + 51.0f, y + 20.0f, -15.0f, 57.0f, 29.0f,
                assets::RoomProp::ControlConsole);
    append_prop(mesh, x + 104.0f, y + 14.0f, -4.0f, 17.0f, 28.0f,
                assets::RoomProp::Terminal);
}

void append_storage(SceneMesh3D& mesh, float x, float y) noexcept {
    append_room_light(mesh, x, y);
    append_prop(mesh, x + 8.0f, y + 14.0f, -23.0f, 52.0f, 36.0f,
                assets::RoomProp::StorageShelf);
    append_prop(mesh, x + 69.0f, y + 13.0f, -19.0f, 31.0f, 38.0f,
                assets::RoomProp::Lockers);
    append_prop(mesh, x + 98.0f, y + 34.0f, -3.0f, 25.0f, 17.0f,
                assets::RoomProp::StorageCrate);
    append_prop(mesh, x + 77.0f, y + 39.0f, -1.0f, 22.0f, 13.0f,
                assets::RoomProp::StorageCrate, rgba(196, 183, 151));
}

void append_living(SceneMesh3D& mesh, float x, float y) noexcept {
    append_room_light(mesh, x, y);
    append_prop(mesh, x + 8.0f, y + 13.0f, -22.0f, 61.0f, 38.0f,
                assets::RoomProp::BunkBeds);
    append_prop(mesh, x + 71.0f, y + 30.0f, -4.0f, 48.0f, 22.0f,
                assets::RoomProp::Sofa);
    append_prop(mesh, x + 79.0f, y + 16.0f, -15.0f, 34.0f, 21.0f,
                assets::RoomProp::WorkTable);
}

void append_dominant_prop(SceneMesh3D& mesh,
                          float x,
                          float y,
                          int visual_profile) noexcept {
    switch (normalized_visual_profile(visual_profile)) {
        case 0:
            append_prop(mesh, x + 42.0f, y + 14.0f, -28.0f, 48.0f, 36.0f,
                        assets::RoomProp::PowerGenerator);
            break;
        case 1:
            append_prop(mesh, x + 11.0f, y + 29.0f, -7.0f, 110.0f, 23.0f,
                        assets::RoomProp::HydroponicPlanter);
            break;
        case 2:
            append_prop(mesh, x + 39.0f, y + 13.0f, -25.0f, 62.0f, 38.0f,
                        assets::RoomProp::WaterMachinery);
            break;
        case 3:
            append_prop(mesh, x + 9.0f, y + 28.0f, -6.0f, 66.0f, 24.0f,
                        assets::RoomProp::WorkTable);
            break;
        case 4:
            append_prop(mesh, x + 8.0f, y + 14.0f, -23.0f, 52.0f, 36.0f,
                        assets::RoomProp::StorageShelf);
            break;
        default:
            append_prop(mesh, x + 8.0f, y + 13.0f, -22.0f, 61.0f, 38.0f,
                        assets::RoomProp::BunkBeds);
            break;
    }
}

void append_props(SceneMesh3D& mesh,
                  float x,
                  float y,
                  int visual_profile) noexcept {
    switch (normalized_visual_profile(visual_profile)) {
        case 0: append_power(mesh, x, y); break;
        case 1: append_hydro(mesh, x, y); break;
        case 2: append_water(mesh, x, y); break;
        case 3: append_workshop(mesh, x, y); break;
        case 4: append_storage(mesh, x, y); break;
        default: append_living(mesh, x, y); break;
    }
}

}  // namespace

ShelterSceneState3D::ShelterSceneState3D(
    int legacy_room_count,
    int legacy_selected_room,
    int legacy_stored,
    int legacy_resident_room,
    std::uint32_t tick) noexcept
    : animation_tick(tick) {
    const int active_rooms = std::clamp(legacy_room_count, 0, 6);
    for (int room = 0; room < active_rooms; ++room) {
        RoomRenderEntry& entry = rooms[room_count++];
        entry.grid_column = (room % 2 == 0) ? 0 : 2;
        entry.grid_floor = room / 2;
        entry.visual_profile = room;
        entry.stored = room == legacy_selected_room
                           ? std::max(0, legacy_stored)
                           : 0;
        entry.selected = room == legacy_selected_room;
    }

    // The old vertical slice assumed one central shaft spanning its three
    // floors. Express it through regular grid entries so the renderer itself
    // never invents an elevator that is absent from scene state.
    for (int floor = 0; floor < 3; ++floor) {
        RoomRenderEntry& elevator = rooms[room_count++];
        elevator.grid_column = 1;
        elevator.grid_floor = floor;
        elevator.elevator = true;
    }

    resident_count = layout::kMinimumResidentEntries;
    for (std::size_t index = 0; index < resident_count; ++index) {
        ResidentRenderEntry& resident = residents[index];
        resident.animation_phase = static_cast<std::uint32_t>(index * 17u);
        resident.archetype = 4;
        resident.world_x =
            layout::room_x(index == 1u ? 2 : 0) + 18.0f +
            static_cast<float>(index) * 19.0f;
        resident.world_y =
            layout::room_y(static_cast<int>(index)) + 17.0f;
        resident.moving = true;
    }

    if (legacy_resident_room >= 0 &&
        legacy_resident_room < active_rooms) {
        const int column =
            (legacy_resident_room % 2 == 0) ? 0 : 2;
        const int floor = legacy_resident_room / 2;
        ResidentRenderEntry& worker = residents[0];
        worker.archetype = legacy_resident_room;
        worker.world_x =
            layout::room_x(column) +
            room_visual_profile(legacy_resident_room).resident_clear_x;
        worker.world_y = layout::room_y(floor) + 17.0f;
        worker.moving = false;
        worker.working = true;
    }
}

Scene3DRenderer::~Scene3DRenderer() { shutdown(); }

bool Scene3DRenderer::initialize() noexcept {
    if (initialized_) return true;
    shutdown();
    shader_dvlb_ = DVLB_ParseFile(reinterpret_cast<u32*>(scene3d_v_shbin),
                                  scene3d_v_shbin_size);
    if (shader_dvlb_ == nullptr) return false;
    shaderProgramInit(&program_);
    program_initialized_ = true;
    shaderProgramSetVsh(&program_, &shader_dvlb_->DVLE[0]);
    projection_uniform_ = shaderInstanceGetUniformLocation(program_.vertexShader, "projection");
    model_view_uniform_ = shaderInstanceGetUniformLocation(program_.vertexShader, "modelView");
    if (projection_uniform_ < 0 || model_view_uniform_ < 0) {
        shutdown();
        return false;
    }
    vertex_buffer_ = static_cast<Vertex3D*>(
        linearAlloc(sizeof(Vertex3D) * SceneMesh3D::kMaxVertices));
    if (vertex_buffer_ == nullptr) {
        shutdown();
        return false;
    }
    if (!C3D_TexInit(&material_texture_,
                     static_cast<u16>(assets::kRoomAssetAtlasWidth),
                     static_cast<u16>(assets::kRoomAssetAtlasHeight),
                     GPU_RGBA5551)) {
        shutdown();
        return false;
    }
    texture_initialized_ = true;
    if (room_assets_bin_size != assets::kRoomAssetRuntimeBytes) {
        shutdown();
        return false;
    }
    std::memcpy(material_texture_.data,
                room_assets_bin,
                assets::kRoomAssetRuntimeBytes);
    C3D_TexFlush(&material_texture_);
    C3D_TexSetFilter(&material_texture_, GPU_LINEAR, GPU_LINEAR);
    C3D_TexSetWrap(&material_texture_, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
    initialized_ = true;
    return true;
}

void Scene3DRenderer::shutdown() noexcept {
    initialized_ = false;
    if (texture_initialized_) {
        C3D_TexDelete(&material_texture_);
        material_texture_ = {};
        texture_initialized_ = false;
    }
    if (vertex_buffer_ != nullptr) {
        linearFree(vertex_buffer_);
        vertex_buffer_ = nullptr;
    }
    if (program_initialized_) {
        shaderProgramFree(&program_);
        program_ = {};
        program_initialized_ = false;
    }
    if (shader_dvlb_ != nullptr) {
        DVLB_Free(shader_dvlb_);
        shader_dvlb_ = nullptr;
    }
    structure_vertex_end_ = 0;
    prop_vertex_end_ = 0;
    projection_uniform_ = -1;
    model_view_uniform_ = -1;
}

void Scene3DRenderer::build_scene(const ShelterCamera& camera,
                                  const ShelterSceneState3D& state,
                                  RenderStats& stats) noexcept {
    mesh_.clear();
    stats = {};

    // The continuous rock mass must remain behind the deepest room rear wall
    // (z=-56). Empty grid cells are therefore simply undisturbed rock instead
    // of renderer-created cavities or blueprints.
    mesh_.append_box({layout::kBackdropX,
                      layout::kBackdropY,
                      -90.0f,
                      layout::kBackdropWidth,
                      layout::kBackdropHeight,
                      8.0f,
                      rgba(12, 13, 14), assets::GeneratedMaterial::Rock});

    const std::size_t room_count =
        std::min(state.room_count, state.rooms.size());
    std::size_t visible_room_count = 0;
    for (std::size_t index = 0; index < room_count; ++index) {
        const RoomRenderEntry& room = state.rooms[index];
        if (!layout::valid_grid_position(room.grid_column,
                                         room.grid_floor)) {
            ++stats.culled_cells;
            continue;
        }
        const float x = layout::room_x(room.grid_column);
        const float y = layout::room_y(room.grid_floor);
        if (!camera.visible(x, y, layout::kRoomWidth, layout::kRoomHeight)) {
            ++stats.culled_cells;
            continue;
        }
        ++stats.visible_cells;
        ++visible_room_count;
        if (room.elevator) {
            append_elevator(mesh_, layout::elevator_x(room.grid_column), y);
        } else {
            append_shell(mesh_, x, y, room.visual_profile);
        }
    }
    structure_vertex_end_ = mesh_.vertex_count();

    const bool overview_lod =
        visible_room_count > layout::kFullDetailVisibleRoomLimit;
    for (std::size_t index = 0; index < room_count; ++index) {
        const RoomRenderEntry& room = state.rooms[index];
        if (room.elevator ||
            !layout::valid_grid_position(room.grid_column,
                                         room.grid_floor)) {
            continue;
        }
        const float x = layout::room_x(room.grid_column);
        const float y = layout::room_y(room.grid_floor);
        if (!camera.visible(x, y, layout::kRoomWidth, layout::kRoomHeight)) {
            continue;
        }
        if (overview_lod) {
            append_dominant_prop(mesh_, x, y, room.visual_profile);
        } else {
            append_props(mesh_, x, y, room.visual_profile);
        }
    }
    prop_vertex_end_ = mesh_.vertex_count();

    bool selection_drawn = false;
    for (std::size_t index = 0;
         index < room_count && !selection_drawn;
         ++index) {
        const RoomRenderEntry& room = state.rooms[index];
        if (!room.selected ||
            !layout::valid_grid_position(room.grid_column,
                                         room.grid_floor)) {
            continue;
        }
        const float cell_x = layout::room_x(room.grid_column);
        const float x = room.elevator
                            ? layout::elevator_x(room.grid_column)
                            : cell_x;
        const float y = layout::room_y(room.grid_floor);
        const float width =
            room.elevator ? layout::kElevatorWidth : layout::kRoomWidth;
        if (camera.visible(cell_x,
                           y,
                           layout::kRoomWidth,
                           layout::kRoomHeight)) {
            if (!room.elevator && room.stored > 0) {
                const float fill = std::clamp(
                    static_cast<float>(room.stored) / 30.0f, 0.0f, 1.0f);
                mesh_.append_box(
                    {x + 10.0f,
                     y + 9.0f,
                     -2.0f,
                     (width - 20.0f) * fill,
                     3.0f,
                     4.0f,
                     rgba(112, 255, 177),
                     assets::GeneratedMaterial::ControlPanel});
            }
            append_selection(mesh_, x, y, width);
            selection_drawn = true;
        }
    }

    const BuildPreviewRenderEntry& preview = state.build_preview;
    if (preview.active &&
        layout::valid_grid_position(preview.grid_column,
                                    preview.grid_floor)) {
        const float cell_x = layout::room_x(preview.grid_column);
        const float y = layout::room_y(preview.grid_floor);
        if (camera.visible(cell_x,
                           y,
                           layout::kRoomWidth,
                           layout::kRoomHeight)) {
            const float x = preview.elevator
                                ? layout::elevator_x(preview.grid_column)
                                : cell_x;
            const float width = preview.elevator
                                    ? layout::kElevatorWidth
                                    : layout::kRoomWidth;
            append_build_preview(mesh_, x, y, width, preview.valid);
        }
    }

    stats.draw_calls = 0;
    if (structure_vertex_end_ > 0) ++stats.draw_calls;
    if (prop_vertex_end_ > structure_vertex_end_) ++stats.draw_calls;
    if (mesh_.vertex_count() > prop_vertex_end_) ++stats.draw_calls;
    stats.estimated_linear_memory =
        mesh_.vertex_count() * sizeof(Vertex3D) + assets::kRoomAssetRuntimeBytes;
}

void Scene3DRenderer::draw(C3D_RenderTarget* target,
                           const ShelterCamera& camera,
                           float stereo_eye,
                           const ShelterSceneState3D& state,
                           RenderStats& stats) noexcept {
    if (!initialized_ || target == nullptr) return;
    build_scene(camera, state, stats);
    if (mesh_.vertex_count() == 0) return;

    std::memcpy(vertex_buffer_, mesh_.data(), mesh_.vertex_count() * sizeof(Vertex3D));
    GSPGPU_FlushDataCache(vertex_buffer_, mesh_.vertex_count() * sizeof(Vertex3D));

    C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x070A0CFF, 0);
    C3D_FrameDrawOn(target);
    C3D_BindProgram(&program_);

    C3D_AttrInfo* attrs = C3D_GetAttrInfo();
    AttrInfo_Init(attrs);
    AttrInfo_AddLoader(attrs, 0, GPU_FLOAT, 3);
    AttrInfo_AddLoader(attrs, 1, GPU_FLOAT, 2);
    AttrInfo_AddLoader(attrs, 2, GPU_FLOAT, 3);
    AttrInfo_AddLoader(attrs, 3, GPU_FLOAT, 4);

    C3D_BufInfo* buffers = C3D_GetBufInfo();
    BufInfo_Init(buffers);
    BufInfo_Add(buffers, vertex_buffer_, sizeof(Vertex3D), 4, 0x3210);

    C3D_TexBind(0, &material_texture_);
    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env,
                  C3D_Both,
                  GPU_TEXTURE0,
                  GPU_PRIMARY_COLOR,
                  GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Both, GPU_MODULATE);

    C3D_Mtx projection;
    C3D_Mtx view;
    build_shelter_view_matrices(projection, view, camera, stereo_eye);

    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, projection_uniform_, &projection);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, model_view_uniform_, &view);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);
    C3D_CullFace(GPU_CULL_NONE);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD,
                   GPU_ONE, GPU_ZERO,
                   GPU_ONE, GPU_ZERO);
    C3D_AlphaTest(true, GPU_GREATER, 0);

    const std::size_t total = mesh_.vertex_count();
    const std::size_t structure_end = std::min(structure_vertex_end_, total);
    const std::size_t props_end = std::max(structure_end,
                                           std::min(prop_vertex_end_, total));
    if (structure_end > 0) {
        C3D_DrawArrays(GPU_TRIANGLES, 0, static_cast<int>(structure_end));
    }
    if (props_end > structure_end) {
        C3D_DrawArrays(GPU_TRIANGLES,
                       static_cast<int>(structure_end),
                       static_cast<int>(props_end - structure_end));
    }
    if (total > props_end) {
        C3D_DrawArrays(GPU_TRIANGLES,
                       static_cast<int>(props_end),
                       static_cast<int>(total - props_end));
    }
    C3D_AlphaTest(false, GPU_ALWAYS, 0);
}

}  // namespace deep_shelter::render
