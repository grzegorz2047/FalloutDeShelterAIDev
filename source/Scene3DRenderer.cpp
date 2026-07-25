#include "render/Scene3DRenderer.hpp"

#include <algorithm>
#include <cstring>

#include "assets/GeneratedMaterialAtlas.hpp"
#include "render/RoomVisualProfiles.hpp"
#include "render/ShelterSceneLayout.hpp"
#include "scene3d_v_shbin.h"

namespace deep_shelter::render {
namespace {

constexpr u32 rgba(u8 r, u8 g, u8 b, u8 a = 255) noexcept {
    return static_cast<u32>(r) | (static_cast<u32>(g) << 8) |
           (static_cast<u32>(b) << 16) | (static_cast<u32>(a) << 24);
}

u32 room_accent(int room) noexcept {
    switch ((room % 6 + 6) % 6) {
        case 0: return rgba(244, 177, 61);
        case 1: return rgba(102, 210, 116);
        case 2: return rgba(78, 179, 224);
        case 3: return rgba(221, 135, 72);
        case 4: return rgba(207, 173, 94);
        default: return rgba(122, 174, 204);
    }
}

u32 room_wall(int room) noexcept {
    switch ((room % 6 + 6) % 6) {
        case 0: return rgba(91, 78, 48);
        case 1: return rgba(48, 82, 54);
        case 2: return rgba(43, 70, 86);
        case 3: return rgba(83, 58, 43);
        case 4: return rgba(73, 66, 46);
        default: return rgba(68, 55, 51);
    }
}

u32 room_floor(int room) noexcept {
    switch ((room % 6 + 6) % 6) {
        case 0: return rgba(126, 99, 44);
        case 1: return rgba(58, 113, 65);
        case 2: return rgba(52, 103, 128);
        case 3: return rgba(112, 72, 48);
        case 4: return rgba(105, 91, 55);
        default: return rgba(108, 75, 61);
    }
}

void append_selection(SceneMesh3D& mesh, float x, float y) noexcept {
    constexpr u32 c = 0xFF56C7F0;
    constexpr float arm = 22.0f;
    constexpr float t = 3.0f;
    const float r = x + layout::kRoomWidth;
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

void append_cavity(SceneMesh3D& mesh, float x, float y, int room) noexcept {
    const u32 blueprint = room_accent(room);
    mesh.append_box({x + 4.0f, y + 4.0f, -19.0f, 124.0f, 56.0f, 5.0f,
                     rgba(28, 27, 27), assets::GeneratedMaterial::Rock});
    mesh.append_box({x + 8.0f, y + 9.0f, -15.0f, 116.0f, 45.0f, 3.0f,
                     rgba(42, 39, 37), assets::GeneratedMaterial::ExcavatedRock});
    mesh.append_box({x + 8.0f, y + 7.0f, -9.0f, 6.0f, 50.0f, 8.0f,
                     rgba(51, 58, 59), assets::GeneratedMaterial::Steel});
    mesh.append_box({x + 118.0f, y + 7.0f, -9.0f, 6.0f, 50.0f, 8.0f,
                     rgba(51, 58, 59), assets::GeneratedMaterial::Steel});
    mesh.append_box({x + 8.0f, y + 49.0f, -8.0f, 116.0f, 7.0f, 8.0f,
                     rgba(51, 58, 59), assets::GeneratedMaterial::Grating});
    const auto& profile = room_visual_profile(room);
    const float sx = x + (layout::kRoomWidth - profile.dominant_width) * 0.5f;
    const float sy = y + 18.0f;
    mesh.append_box({sx, sy, -5.0f, profile.dominant_width, 6.0f, 3.0f,
                     blueprint, assets::GeneratedMaterial::Grating});
    mesh.append_box({sx + profile.dominant_width * 0.25f, sy + 7.0f, -5.0f,
                     profile.dominant_width * 0.5f, profile.dominant_height - 7.0f, 3.0f,
                     blueprint, assets::GeneratedMaterial::Grating});
}

void append_shell(SceneMesh3D& mesh, float x, float y, int room) noexcept {
    const u32 frame = rgba(48, 58, 60);
    mesh.append_box({x + 5.0f, y + 5.0f, -18.0f, 122.0f, 54.0f, 5.0f,
                     room_wall(room), assets::GeneratedMaterial::VaultPanel});
    mesh.append_box({x + 2.0f, y + 58.0f, -14.0f, 128.0f, 6.0f, 18.0f,
                     frame, assets::GeneratedMaterial::Steel});
    mesh.append_box({x, y + 4.0f, -14.0f, 6.0f, 60.0f, 18.0f,
                     frame, assets::GeneratedMaterial::Steel});
    mesh.append_box({x + 126.0f, y, -14.0f, 6.0f, 57.0f, 18.0f,
                     frame, assets::GeneratedMaterial::Steel});
    mesh.append_box({x + 6.0f, y + 50.0f, -9.0f, 120.0f, 8.0f, 13.0f,
                     room_floor(room), assets::GeneratedMaterial::Grating});
    mesh.append_box({x + 10.0f, y + 2.0f, -11.0f, 112.0f, 5.0f, 12.0f,
                     room_accent(room), assets::GeneratedMaterial::ControlPanel});
}

void append_power(SceneMesh3D& mesh, float x, float y) noexcept {
    const u32 gold = rgba(225, 164, 55);
    mesh.append_box({x + 39.0f, y + 11.0f, -10.0f, 54.0f, 39.0f, 13.0f,
                     gold, assets::GeneratedMaterial::Steel});
    mesh.append_box({x + 47.0f, y + 17.0f, -5.0f, 38.0f, 24.0f, 5.0f,
                     rgba(255, 204, 76), assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({x + 12.0f, y + 25.0f, -8.0f, 24.0f, 25.0f, 10.0f,
                     rgba(118, 91, 45), assets::GeneratedMaterial::VaultPanel});
    mesh.append_box({x + 96.0f, y + 25.0f, -8.0f, 24.0f, 25.0f, 10.0f,
                     rgba(118, 91, 45), assets::GeneratedMaterial::VaultPanel});
    mesh.append_box({x + 20.0f, y + 17.0f, -5.0f, 9.0f, 8.0f, 5.0f,
                     gold, assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({x + 103.0f, y + 17.0f, -5.0f, 9.0f, 8.0f, 5.0f,
                     gold, assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({x + 57.0f, y + 7.0f, -6.0f, 18.0f, 5.0f, 6.0f,
                     rgba(255, 223, 111), assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({x + 16.0f, y + 46.0f, -4.0f, 100.0f, 4.0f, 4.0f,
                     rgba(173, 120, 40), assets::GeneratedMaterial::Grating});
}

void append_hydro(SceneMesh3D& mesh, float x, float y) noexcept {
    mesh.append_box({x + 9.0f, y + 35.0f, -8.0f, 114.0f, 15.0f, 10.0f,
                     rgba(65, 111, 57), assets::GeneratedMaterial::Hydroponic});
    mesh.append_box({x + 17.0f, y + 28.0f, -5.0f, 25.0f, 7.0f, 5.0f,
                     rgba(86, 175, 78), assets::GeneratedMaterial::Hydroponic});
    mesh.append_box({x + 53.0f, y + 21.0f, -5.0f, 27.0f, 14.0f, 5.0f,
                     rgba(130, 222, 105), assets::GeneratedMaterial::Hydroponic});
    mesh.append_box({x + 91.0f, y + 26.0f, -5.0f, 23.0f, 9.0f, 5.0f,
                     rgba(92, 190, 83), assets::GeneratedMaterial::Hydroponic});
    mesh.append_box({x + 22.0f, y + 13.0f, -4.0f, 5.0f, 15.0f, 4.0f,
                     rgba(63, 129, 57), assets::GeneratedMaterial::Hydroponic});
    mesh.append_box({x + 64.0f, y + 8.0f, -4.0f, 5.0f, 13.0f, 4.0f,
                     rgba(74, 145, 62), assets::GeneratedMaterial::Hydroponic});
    mesh.append_box({x + 101.0f, y + 15.0f, -4.0f, 5.0f, 11.0f, 4.0f,
                     rgba(62, 128, 54), assets::GeneratedMaterial::Hydroponic});
    mesh.append_box({x + 14.0f, y + 8.0f, -7.0f, 104.0f, 4.0f, 5.0f,
                     rgba(176, 218, 132), assets::GeneratedMaterial::ControlPanel});
}

void append_water(SceneMesh3D& mesh, float x, float y) noexcept {
    mesh.append_box({x + 38.0f, y + 10.0f, -10.0f, 64.0f, 40.0f, 14.0f,
                     rgba(70, 151, 188), assets::GeneratedMaterial::Water});
    mesh.append_box({x + 45.0f, y + 17.0f, -5.0f, 50.0f, 26.0f, 5.0f,
                     rgba(96, 194, 222), assets::GeneratedMaterial::Water});
    mesh.append_box({x + 12.0f, y + 27.0f, -7.0f, 22.0f, 23.0f, 9.0f,
                     rgba(55, 91, 104), assets::GeneratedMaterial::Steel});
    mesh.append_box({x + 106.0f, y + 22.0f, -7.0f, 14.0f, 28.0f, 9.0f,
                     rgba(55, 91, 104), assets::GeneratedMaterial::Steel});
    mesh.append_box({x + 62.0f, y + 5.0f, -5.0f, 12.0f, 6.0f, 5.0f,
                     rgba(187, 231, 239), assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({x + 19.0f, y + 20.0f, -4.0f, 8.0f, 7.0f, 4.0f,
                     rgba(83, 164, 194), assets::GeneratedMaterial::Water});
    mesh.append_box({x + 102.0f, y + 13.0f, -4.0f, 8.0f, 9.0f, 4.0f,
                     rgba(83, 164, 194), assets::GeneratedMaterial::Water});
    mesh.append_box({x + 17.0f, y + 46.0f, -4.0f, 102.0f, 4.0f, 4.0f,
                     rgba(48, 111, 135), assets::GeneratedMaterial::Water});
}

void append_workshop(SceneMesh3D& mesh, float x, float y) noexcept {
    mesh.append_box({x + 10.0f, y + 38.0f, -8.0f, 112.0f, 12.0f, 10.0f,
                     rgba(122, 88, 61), assets::GeneratedMaterial::Grating});
    mesh.append_box({x + 15.0f, y + 23.0f, -6.0f, 52.0f, 15.0f, 8.0f,
                     rgba(164, 91, 48), assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({x + 78.0f, y + 9.0f, -10.0f, 34.0f, 29.0f, 12.0f,
                     rgba(214, 125, 65), assets::GeneratedMaterial::Steel});
    mesh.append_box({x + 87.0f, y + 17.0f, -5.0f, 16.0f, 13.0f, 5.0f,
                     rgba(242, 170, 87), assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({x + 19.0f, y + 13.0f, -5.0f, 43.0f, 8.0f, 5.0f,
                     rgba(83, 70, 60), assets::GeneratedMaterial::VaultPanel});
    mesh.append_box({x + 24.0f, y + 15.0f, -3.0f, 6.0f, 5.0f, 3.0f,
                     rgba(236, 170, 78), assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({x + 39.0f, y + 15.0f, -3.0f, 6.0f, 5.0f, 3.0f,
                     rgba(106, 175, 190), assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({x + 54.0f, y + 15.0f, -3.0f, 6.0f, 5.0f, 3.0f,
                     rgba(190, 93, 62), assets::GeneratedMaterial::ControlPanel});
}

void append_storage(SceneMesh3D& mesh, float x, float y) noexcept {
    mesh.append_box({x + 10.0f, y + 10.0f, -9.0f, 112.0f, 7.0f, 10.0f,
                     rgba(121, 105, 65), assets::GeneratedMaterial::Steel});
    mesh.append_box({x + 13.0f, y + 18.0f, -8.0f, 7.0f, 32.0f, 9.0f,
                     rgba(90, 83, 59), assets::GeneratedMaterial::Steel});
    mesh.append_box({x + 112.0f, y + 18.0f, -8.0f, 7.0f, 32.0f, 9.0f,
                     rgba(90, 83, 59), assets::GeneratedMaterial::Steel});
    mesh.append_box({x + 25.0f, y + 31.0f, -8.0f, 27.0f, 19.0f, 10.0f,
                     rgba(196, 160, 82), assets::GeneratedMaterial::VaultPanel});
    mesh.append_box({x + 55.0f, y + 20.0f, -9.0f, 31.0f, 30.0f, 11.0f,
                     rgba(151, 122, 67), assets::GeneratedMaterial::VaultPanel});
    mesh.append_box({x + 89.0f, y + 28.0f, -8.0f, 21.0f, 22.0f, 10.0f,
                     rgba(210, 173, 94), assets::GeneratedMaterial::VaultPanel});
    mesh.append_box({x + 27.0f, y + 25.0f, -5.0f, 22.0f, 5.0f, 5.0f,
                     rgba(231, 205, 133), assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({x + 61.0f, y + 14.0f, -5.0f, 19.0f, 5.0f, 5.0f,
                     rgba(231, 205, 133), assets::GeneratedMaterial::ControlPanel});
}

void append_living(SceneMesh3D& mesh, float x, float y) noexcept {
    mesh.append_box({x + 10.0f, y + 35.0f, -8.0f, 72.0f, 15.0f, 10.0f,
                     rgba(158, 111, 86), assets::GeneratedMaterial::VaultPanel});
    mesh.append_box({x + 10.0f, y + 19.0f, -8.0f, 72.0f, 13.0f, 10.0f,
                     rgba(181, 137, 105), assets::GeneratedMaterial::VaultPanel});
    mesh.append_box({x + 10.0f, y + 14.0f, -8.0f, 7.0f, 36.0f, 10.0f,
                     rgba(78, 68, 66), assets::GeneratedMaterial::Steel});
    mesh.append_box({x + 75.0f, y + 14.0f, -8.0f, 7.0f, 36.0f, 10.0f,
                     rgba(78, 68, 66), assets::GeneratedMaterial::Steel});
    mesh.append_box({x + 88.0f, y + 28.0f, -8.0f, 32.0f, 22.0f, 10.0f,
                     rgba(121, 91, 79), assets::GeneratedMaterial::VaultPanel});
    mesh.append_box({x + 96.0f, y + 22.0f, -5.0f, 16.0f, 6.0f, 5.0f,
                     rgba(233, 201, 136), assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({x + 23.0f, y + 22.0f, -4.0f, 48.0f, 5.0f, 4.0f,
                     rgba(226, 190, 141), assets::GeneratedMaterial::ControlPanel});
    mesh.append_box({x + 23.0f, y + 39.0f, -4.0f, 48.0f, 5.0f, 4.0f,
                     rgba(226, 190, 141), assets::GeneratedMaterial::ControlPanel});
}

void append_props(SceneMesh3D& mesh, float x, float y, int room) noexcept {
    switch ((room % 6 + 6) % 6) {
        case 0: append_power(mesh, x, y); break;
        case 1: append_hydro(mesh, x, y); break;
        case 2: append_water(mesh, x, y); break;
        case 3: append_workshop(mesh, x, y); break;
        case 4: append_storage(mesh, x, y); break;
        default: append_living(mesh, x, y); break;
    }
}

}  // namespace

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
                     static_cast<u16>(assets::kGeneratedMaterialAtlasWidth),
                     static_cast<u16>(assets::kGeneratedMaterialAtlasHeight),
                     GPU_RGB565)) {
        shutdown();
        return false;
    }
    texture_initialized_ = true;
    assets::decode_generated_material_atlas_tiled(
        static_cast<std::uint16_t*>(material_texture_.data),
        assets::kGeneratedMaterialPixelCount);
    C3D_TexFlush(&material_texture_);
    C3D_TexSetFilter(&material_texture_, GPU_NEAREST, GPU_NEAREST);
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

void Scene3DRenderer::append_room(float x,
                                  float y,
                                  int room,
                                  bool active,
                                  bool selected,
                                  bool resident,
                                  int stored) noexcept {
    if (!active) {
        append_cavity(mesh_, x, y, room);
        return;
    }
    append_shell(mesh_, x, y, room);
    append_props(mesh_, x, y, room);
    if (stored > 0) {
        const float fill = std::clamp(static_cast<float>(stored) / 30.0f, 0.0f, 1.0f);
        mesh_.append_box({x + 10.0f, y + 8.0f, -3.0f, 112.0f * fill, 4.0f, 5.0f,
                          rgba(112, 255, 177), assets::GeneratedMaterial::ControlPanel});
    }
    if (resident) {
        const float rx = room_visual_profile(room).resident_clear_x;
        mesh_.append_box({x + rx + 4.0f, y + 17.0f, -1.0f, 10.0f, 10.0f, 6.0f,
                          rgba(245, 210, 174), assets::GeneratedMaterial::Steel});
        mesh_.append_box({x + rx, y + 27.0f, -1.0f, 18.0f, 27.0f, 6.0f,
                          rgba(48, 119, 164), assets::GeneratedMaterial::ControlPanel});
    }
    if (selected) append_selection(mesh_, x, y);
}

void Scene3DRenderer::build_scene(const ShelterCamera& camera,
                                  const ShelterSceneState3D& state,
                                  RenderStats& stats) noexcept {
    mesh_.clear();
    mesh_.append_box({4.0f, 10.0f, -34.0f, 392.0f, 220.0f, 8.0f,
                      rgba(12, 13, 14), assets::GeneratedMaterial::Rock});
    mesh_.append_box({0.0f, 5.0f, -29.0f, 32.0f, 225.0f, 18.0f,
                      rgba(47, 39, 34), assets::GeneratedMaterial::Rock});
    mesh_.append_box({368.0f, 5.0f, -29.0f, 32.0f, 225.0f, 18.0f,
                      rgba(47, 39, 34), assets::GeneratedMaterial::Rock});
    mesh_.append_box({18.0f, 8.0f, -29.0f, 364.0f, 17.0f, 18.0f,
                      rgba(60, 46, 38), assets::GeneratedMaterial::ExcavatedRock});
    mesh_.append_box({18.0f, 220.0f, -29.0f, 364.0f, 18.0f, 18.0f,
                      rgba(60, 46, 38), assets::GeneratedMaterial::ExcavatedRock});
    for (int floor = 0; floor < 3; ++floor) {
        const float fy = layout::kRoomY[floor * 2] + layout::kRoomHeight;
        mesh_.append_box({28.0f, fy - 1.0f, -10.0f, 157.0f, 6.0f, 13.0f,
                          rgba(43, 51, 52), assets::GeneratedMaterial::Grating});
        mesh_.append_box({215.0f, fy - 1.0f, -10.0f, 157.0f, 6.0f, 13.0f,
                          rgba(43, 51, 52), assets::GeneratedMaterial::Grating});
    }
    mesh_.append_box({181.0f, 13.0f, -20.0f, 38.0f, 214.0f, 8.0f,
                      rgba(31, 29, 27), assets::GeneratedMaterial::Rock});
    mesh_.append_box({185.0f, 18.0f, -13.0f, 30.0f, 204.0f, 16.0f,
                      rgba(37, 46, 48), assets::GeneratedMaterial::Steel});
    for (int floor = 0; floor < 3; ++floor) {
        const float door_y = layout::kRoomY[floor * 2] + 9.0f;
        mesh_.append_box({189.0f, door_y, -4.0f, 22.0f, 43.0f, 6.0f,
                          rgba(67, 80, 82), assets::GeneratedMaterial::VaultPanel});
        mesh_.append_box({195.0f, door_y + 8.0f, -1.0f, 10.0f, 25.0f, 3.0f,
                          rgba(90, 108, 107), assets::GeneratedMaterial::ControlPanel});
    }
    structure_vertex_end_ = mesh_.vertex_count();

    const int active_rooms = std::clamp(state.rooms, 0, 6);
    for (int room = 0; room < 6; ++room) {
        const float x = layout::kRoomX[room];
        const float y = layout::kRoomY[room];
        if (!camera.visible(x, y, layout::kRoomWidth, layout::kRoomHeight)) {
            ++stats.culled_cells;
            continue;
        }
        ++stats.visible_cells;
        const bool active = room < active_rooms;
        append_room(x,
                    y,
                    room,
                    active,
                    active && room == state.selected_room,
                    active && state.resident_assigned && room == state.selected_room,
                    active && room == state.selected_room ? state.stored : 0);
    }
    prop_vertex_end_ = mesh_.vertex_count();

    stats.draw_calls = mesh_.vertex_count() > 0 ? 3 : 0;
    stats.estimated_linear_memory =
        mesh_.vertex_count() * sizeof(Vertex3D) + assets::kGeneratedMaterialRuntimeBytes;
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

    const float zoom = camera.zoom();
    const float center_x = camera.x() + 200.0f / zoom;
    const float center_y = camera.y() + 120.0f / zoom;

    C3D_Mtx projection;
    Mtx_PerspStereoTilt(&projection,
                        C3D_AngleFromDegrees(22.0f),
                        C3D_AspectRatioTop,
                        1.0f,
                        2000.0f,
                        stereo_eye,
                        3.0f,
                        false);
    C3D_Mtx view;
    Mtx_LookAt(&view,
               FVec3_New(center_x, center_y, 1120.0f / zoom),
               FVec3_New(center_x, center_y, 0.0f),
               FVec3_New(0.0f, -1.0f, 0.0f),
               false);

    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, projection_uniform_, &projection);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, model_view_uniform_, &view);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);
    C3D_CullFace(GPU_CULL_NONE);

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
}

}  // namespace deep_shelter::render
