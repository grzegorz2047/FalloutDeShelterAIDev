#include "render/Scene3DRenderer.hpp"

#include <algorithm>
#include <cstring>

#include "assets/GeneratedMaterialAtlas.hpp"
#include "render/ShelterSceneLayout.hpp"
#include "scene3d_v_shbin.h"

namespace deep_shelter::render {
namespace {

constexpr u32 rgba(u8 r, u8 g, u8 b, u8 a = 255) noexcept {
    return static_cast<u32>(r) | (static_cast<u32>(g) << 8) |
           (static_cast<u32>(b) << 16) | (static_cast<u32>(a) << 24);
}

u32 room_accent(int room_index) noexcept {
    switch ((room_index % 6 + 6) % 6) {
        case 0: return rgba(244, 177, 61);
        case 1: return rgba(102, 210, 116);
        case 2: return rgba(78, 179, 224);
        case 3: return rgba(221, 135, 72);
        case 4: return rgba(207, 173, 94);
        default: return rgba(122, 174, 204);
    }
}

u32 room_back_wall(int room_index) noexcept {
    switch ((room_index % 6 + 6) % 6) {
        case 0: return rgba(91, 78, 48);
        case 1: return rgba(48, 82, 54);
        case 2: return rgba(43, 70, 86);
        case 3: return rgba(83, 58, 43);
        case 4: return rgba(73, 66, 46);
        default: return rgba(68, 55, 51);
    }
}

u32 room_floor(int room_index) noexcept {
    switch ((room_index % 6 + 6) % 6) {
        case 0: return rgba(126, 99, 44);
        case 1: return rgba(58, 113, 65);
        case 2: return rgba(52, 103, 128);
        case 3: return rgba(112, 72, 48);
        case 4: return rgba(105, 91, 55);
        default: return rgba(108, 75, 61);
    }
}

void append_selection_corners(SceneMesh3D& mesh, float x, float y) noexcept {
    constexpr u32 highlight = 0xFF56C7F0;
    constexpr float arm = 21.0f;
    constexpr float thickness = 3.0f;
    constexpr float depth = 3.0f;
    const float right = x + layout::kRoomWidth;
    const float bottom = y + layout::kRoomHeight;

    mesh.append_box({x - 2.0f, y - 2.0f, -1.0f, arm, thickness, depth,
                     highlight, assets::GeneratedMaterial::Steel});
    mesh.append_box({x - 2.0f, y - 2.0f, -1.0f, thickness, arm, depth,
                     highlight, assets::GeneratedMaterial::Steel});
    mesh.append_box({right - arm + 2.0f, y - 2.0f, -1.0f, arm, thickness, depth,
                     highlight, assets::GeneratedMaterial::Steel});
    mesh.append_box({right - 1.0f, y - 2.0f, -1.0f, thickness, arm, depth,
                     highlight, assets::GeneratedMaterial::Steel});
    mesh.append_box({x - 2.0f, bottom - 1.0f, -1.0f, arm, thickness, depth,
                     highlight, assets::GeneratedMaterial::Steel});
    mesh.append_box({x - 2.0f, bottom - arm + 2.0f, -1.0f, thickness, arm, depth,
                     highlight, assets::GeneratedMaterial::Steel});
    mesh.append_box({right - arm + 2.0f, bottom - 1.0f, -1.0f, arm, thickness, depth,
                     highlight, assets::GeneratedMaterial::Steel});
    mesh.append_box({right - 1.0f, bottom - arm + 2.0f, -1.0f, thickness, arm, depth,
                     highlight, assets::GeneratedMaterial::Steel});
}

void append_unbuilt_cavity(SceneMesh3D& mesh,
                           float x,
                           float y,
                           int room_index) noexcept {
    const u32 rock = rgba(34, 31, 30);
    const u32 cut_rock = rgba(45, 41, 39);
    const u32 brace = rgba(54, 61, 61);
    const u32 blueprint = [&]() noexcept {
        switch ((room_index % 6 + 6) % 6) {
            case 0: return rgba(126, 91, 34);
            case 1: return rgba(58, 105, 65);
            case 2: return rgba(49, 101, 124);
            case 3: return rgba(113, 70, 42);
            case 4: return rgba(108, 88, 48);
            default: return rgba(99, 68, 59);
        }
    }();

    // Five structural boxes keep the module visibly excavated rather than built.
    mesh.append_box({x + 4.0f, y + 4.0f, -19.0f,
                     layout::kRoomWidth - 8.0f, layout::kRoomHeight - 8.0f, 5.0f,
                     rock, assets::GeneratedMaterial::Rock});
    mesh.append_box({x + 8.0f, y + 9.0f, -15.0f,
                     layout::kRoomWidth - 16.0f, layout::kRoomHeight - 19.0f, 3.0f,
                     cut_rock, assets::GeneratedMaterial::Rock});
    mesh.append_box({x + 8.0f, y + 7.0f, -9.0f, 6.0f,
                     layout::kRoomHeight - 14.0f, 8.0f,
                     brace, assets::GeneratedMaterial::Steel});
    mesh.append_box({x + layout::kRoomWidth - 14.0f, y + 7.0f, -9.0f, 6.0f,
                     layout::kRoomHeight - 14.0f, 8.0f,
                     brace, assets::GeneratedMaterial::Steel});
    mesh.append_box({x + 8.0f, y + layout::kRoomHeight - 15.0f, -8.0f,
                     layout::kRoomWidth - 16.0f, 7.0f, 8.0f,
                     brace, assets::GeneratedMaterial::Grating});

    // Up to three thin, muted blueprint shapes identify the future room type
    // without making the cavity look operational. Total remains <= 8 boxes.
    switch ((room_index % 6 + 6) % 6) {
        case 0:  // Power: stepped lightning-bolt silhouette.
            mesh.append_box({x + 42.0f, y + 17.0f, -5.0f, 35.0f, 7.0f, 3.0f,
                             blueprint, assets::GeneratedMaterial::Grating});
            mesh.append_box({x + 58.0f, y + 24.0f, -5.0f, 31.0f, 7.0f, 3.0f,
                             blueprint, assets::GeneratedMaterial::Grating});
            mesh.append_box({x + 48.0f, y + 31.0f, -5.0f, 35.0f, 7.0f, 3.0f,
                             blueprint, assets::GeneratedMaterial::Grating});
            break;
        case 1:  // Hydroponics: long bed with two plant stems.
            mesh.append_box({x + 28.0f, y + 34.0f, -5.0f, 76.0f, 8.0f, 3.0f,
                             blueprint, assets::GeneratedMaterial::Grating});
            mesh.append_box({x + 47.0f, y + 20.0f, -5.0f, 7.0f, 14.0f, 3.0f,
                             blueprint, assets::GeneratedMaterial::Grating});
            mesh.append_box({x + 79.0f, y + 17.0f, -5.0f, 7.0f, 17.0f, 3.0f,
                             blueprint, assets::GeneratedMaterial::Grating});
            break;
        case 2:  // Water: broad tank and a narrow feed pipe.
            mesh.append_box({x + 39.0f, y + 17.0f, -5.0f, 52.0f, 25.0f, 3.0f,
                             blueprint, assets::GeneratedMaterial::Grating});
            mesh.append_box({x + 61.0f, y + 11.0f, -5.0f, 8.0f, 6.0f, 3.0f,
                             blueprint, assets::GeneratedMaterial::Grating});
            mesh.append_box({x + 91.0f, y + 28.0f, -5.0f, 17.0f, 7.0f, 3.0f,
                             blueprint, assets::GeneratedMaterial::Grating});
            break;
        case 3:  // Workshop: bench, tool board and press.
            mesh.append_box({x + 24.0f, y + 34.0f, -5.0f, 82.0f, 8.0f, 3.0f,
                             blueprint, assets::GeneratedMaterial::Grating});
            mesh.append_box({x + 35.0f, y + 18.0f, -5.0f, 35.0f, 10.0f, 3.0f,
                             blueprint, assets::GeneratedMaterial::Grating});
            mesh.append_box({x + 84.0f, y + 17.0f, -5.0f, 12.0f, 17.0f, 3.0f,
                             blueprint, assets::GeneratedMaterial::Grating});
            break;
        case 4:  // Storage: three uneven crate silhouettes.
            mesh.append_box({x + 31.0f, y + 28.0f, -5.0f, 22.0f, 14.0f, 3.0f,
                             blueprint, assets::GeneratedMaterial::Grating});
            mesh.append_box({x + 57.0f, y + 20.0f, -5.0f, 25.0f, 22.0f, 3.0f,
                             blueprint, assets::GeneratedMaterial::Grating});
            mesh.append_box({x + 86.0f, y + 31.0f, -5.0f, 18.0f, 11.0f, 3.0f,
                             blueprint, assets::GeneratedMaterial::Grating});
            break;
        default:  // Living: bed, headboard and side cabinet.
            mesh.append_box({x + 32.0f, y + 31.0f, -5.0f, 68.0f, 11.0f, 3.0f,
                             blueprint, assets::GeneratedMaterial::Grating});
            mesh.append_box({x + 32.0f, y + 18.0f, -5.0f, 10.0f, 13.0f, 3.0f,
                             blueprint, assets::GeneratedMaterial::Grating});
            mesh.append_box({x + 103.0f, y + 29.0f, -5.0f, 12.0f, 13.0f, 3.0f,
                             blueprint, assets::GeneratedMaterial::Grating});
            break;
    }
}

}  // namespace

Scene3DRenderer::~Scene3DRenderer() {
    shutdown();
}

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
        program_initialized_ = false;
        program_ = {};
    }

    if (shader_dvlb_ != nullptr) {
        DVLB_Free(shader_dvlb_);
        shader_dvlb_ = nullptr;
    }

    projection_uniform_ = -1;
    model_view_uniform_ = -1;
}

void Scene3DRenderer::append_room(float x,
                                  float y,
                                  int room_index,
                                  bool active,
                                  bool selected,
                                  bool resident,
                                  int stored) noexcept {
    if (!active) {
        append_unbuilt_cavity(mesh_, x, y, room_index);
        return;
    }

    const int profile = (room_index % 6 + 6) % 6;
    const u32 frame = selected ? rgba(126, 92, 40) : rgba(55, 65, 66);
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
                      selected ? rgba(198, 151, 62) : accent,
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

    switch (room_index % 6) {
        case 0:
            mesh_.append_box({x + 10.0f, y + 25.0f, -8.0f, 28.0f, 26.0f, 11.0f,
                              rgba(145, 105, 42), assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 43.0f, y + 12.0f, -9.0f, 48.0f, 39.0f, 12.0f,
                              accent, assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 96.0f, y + 27.0f, -8.0f, 25.0f, 24.0f, 11.0f,
                              rgba(145, 105, 42), assets::GeneratedMaterial::ControlPanel});
            break;
        case 1:
            mesh_.append_box({x + 10.0f, y + 35.0f, -8.0f, 112.0f, 16.0f, 10.0f,
                              rgba(73, 132, 78), assets::GeneratedMaterial::Grating});
            mesh_.append_box({x + 19.0f, y + 18.0f, -5.0f, 24.0f, 17.0f, 6.0f,
                              accent, assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 51.0f, y + 13.0f, -5.0f, 31.0f, 22.0f, 6.0f,
                              rgba(145, 224, 137), assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 91.0f, y + 20.0f, -5.0f, 22.0f, 15.0f, 6.0f,
                              accent, assets::GeneratedMaterial::ControlPanel});
            break;
        case 2:
            mesh_.append_box({x + 11.0f, y + 24.0f, -8.0f, 27.0f, 27.0f, 11.0f,
                              rgba(55, 126, 153), assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 43.0f, y + 11.0f, -10.0f, 68.0f, 40.0f, 13.0f,
                              accent, assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 67.0f, y + 5.0f, -5.0f, 11.0f, 9.0f, 5.0f,
                              rgba(191, 229, 237), assets::GeneratedMaterial::ControlPanel});
            break;
        case 3:
            mesh_.append_box({x + 13.0f, y + 39.0f, -8.0f, 108.0f, 12.0f, 10.0f,
                              rgba(132, 99, 69), assets::GeneratedMaterial::Grating});
            mesh_.append_box({x + 18.0f, y + 23.0f, -6.0f, 43.0f, 16.0f, 8.0f,
                              rgba(162, 91, 47), assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 77.0f, y + 10.0f, -9.0f, 34.0f, 29.0f, 11.0f,
                              accent, assets::GeneratedMaterial::Steel});
            break;
        case 4:
            mesh_.append_box({x + 12.0f, y + 33.0f, -8.0f, 26.0f, 18.0f, 9.0f,
                              accent, assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 41.0f, y + 20.0f, -9.0f, 31.0f, 31.0f, 10.0f,
                              rgba(177, 143, 72), assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 75.0f, y + 28.0f, -8.0f, 21.0f, 23.0f, 9.0f,
                              accent, assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 99.0f, y + 15.0f, -9.0f, 23.0f, 36.0f, 10.0f,
                              rgba(177, 143, 72), assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 18.0f, y + 11.0f, -5.0f, 96.0f, 5.0f, 5.0f,
                              rgba(220, 205, 153), assets::GeneratedMaterial::Grating});
            break;
        default:
            mesh_.append_box({x + 11.0f, y + 34.0f, -8.0f, 70.0f, 17.0f, 9.0f,
                              rgba(164, 119, 92), assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 84.0f, y + 22.0f, -8.0f, 35.0f, 29.0f, 9.0f,
                              accent, assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 18.0f, y + 15.0f, -5.0f, 21.0f, 18.0f, 6.0f,
                              rgba(237, 217, 166), assets::GeneratedMaterial::ControlPanel});
            break;
    }

    const float fill = std::clamp(static_cast<float>(stored) / 30.0f, 0.0f, 1.0f);
    if (fill > 0.0f) {
        mesh_.append_box({x + 10.0f, y + 8.0f, -3.0f,
                          (layout::kRoomWidth - 20.0f) * fill, 4.0f, 5.0f,
                          rgba(112, 255, 177), assets::GeneratedMaterial::Grating});
    }

    if (resident) {
        const float resident_x = profile == 0 ? 105.0f :
                                 (profile == 1 ? 88.0f :
                                  (profile == 2 ? 18.0f :
                                   (profile == 3 ? 64.0f :
                                    (profile == 4 ? 12.0f : 92.0f))));
        mesh_.append_box({x + resident_x + 4.0f, y + 17.0f, -1.0f, 10.0f, 10.0f, 6.0f,
                          rgba(245, 210, 174), assets::GeneratedMaterial::Steel});
        mesh_.append_box({x + resident_x, y + 27.0f, -1.0f, 18.0f, 27.0f, 6.0f,
                          rgba(48, 119, 164), assets::GeneratedMaterial::ControlPanel});
    }

    if (selected) append_selection_corners(mesh_, x, y);
}

void Scene3DRenderer::build_scene(const ShelterCamera& camera,
                                          const ShelterSceneState3D& state,
                                          RenderStats& stats) noexcept {
    mesh_.clear();
    const u32 deep_void = rgba(13, 14, 14);
    const u32 rock_dark = rgba(34, 29, 26);
    const u32 rock_mid = rgba(54, 45, 38);
    const u32 rock_edge = rgba(78, 61, 48);
    const u32 steel_dark = rgba(35, 43, 44);
    const u32 steel_mid = rgba(61, 72, 72);
    mesh_.append_box({layout::kBackdropX, layout::kBackdropY, -34.0f,
                      layout::kBackdropWidth, layout::kBackdropHeight, 8.0f,
                      deep_void, assets::GeneratedMaterial::Rock});
    mesh_.append_box({2.0f, 8.0f, -29.0f, 26.0f, 72.0f, 18.0f,
                      rock_mid, assets::GeneratedMaterial::Rock});
    mesh_.append_box({0.0f, 72.0f, -25.0f, 35.0f, 83.0f, 14.0f,
                      rock_dark, assets::GeneratedMaterial::Rock});
    mesh_.append_box({5.0f, 149.0f, -31.0f, 25.0f, 82.0f, 20.0f,
                      rock_edge, assets::GeneratedMaterial::Rock});
    mesh_.append_box({368.0f, 5.0f, -27.0f, 32.0f, 62.0f, 16.0f,
                      rock_edge, assets::GeneratedMaterial::Rock});
    mesh_.append_box({360.0f, 61.0f, -31.0f, 40.0f, 98.0f, 20.0f,
                      rock_dark, assets::GeneratedMaterial::Rock});
    mesh_.append_box({372.0f, 154.0f, -26.0f, 28.0f, 78.0f, 15.0f,
                      rock_mid, assets::GeneratedMaterial::Rock});
    mesh_.append_box({18.0f, 9.0f, -28.0f, 126.0f, 18.0f, 17.0f,
                      rock_dark, assets::GeneratedMaterial::Rock});
    mesh_.append_box({127.0f, -4.0f, -31.0f, 151.0f, 17.0f, 20.0f,
                      rock_edge, assets::GeneratedMaterial::Rock});
    mesh_.append_box({272.0f, 8.0f, -27.0f, 109.0f, 18.0f, 16.0f,
                      rock_mid, assets::GeneratedMaterial::Rock});
    mesh_.append_box({11.0f, 220.0f, -30.0f, 161.0f, 18.0f, 19.0f,
                      rock_edge, assets::GeneratedMaterial::Rock});
    mesh_.append_box({154.0f, 226.0f, -26.0f, 139.0f, 12.0f, 15.0f,
                      rock_dark, assets::GeneratedMaterial::Rock});
    mesh_.append_box({287.0f, 214.0f, -31.0f, 98.0f, 24.0f, 20.0f,
                      rock_mid, assets::GeneratedMaterial::Rock});
    for (int floor_index = 0; floor_index < 3; ++floor_index) {
        const float y = layout::kRoomY[floor_index * 2] + layout::kRoomHeight;
        mesh_.append_box({28.0f, y - 1.0f, -10.0f, 157.0f, 6.0f, 13.0f,
                          floor_index == 2 ? rock_edge : steel_dark,
                          assets::GeneratedMaterial::Grating});
        mesh_.append_box({215.0f, y - 1.0f, -10.0f, 157.0f, 6.0f, 13.0f,
                          floor_index == 0 ? rock_mid : steel_dark,
                          assets::GeneratedMaterial::Grating});
    }
    mesh_.append_box({layout::kElevatorX - 4.0f, layout::kElevatorY - 5.0f, -20.0f,
                      layout::kElevatorWidth + 8.0f, layout::kElevatorHeight + 10.0f,
                      8.0f, rock_dark, assets::GeneratedMaterial::Rock});
    mesh_.append_box({layout::kElevatorX, layout::kElevatorY, -13.0f,
                      layout::kElevatorWidth, layout::kElevatorHeight, 16.0f,
                      steel_dark, assets::GeneratedMaterial::Steel});
    for (int floor_index = 0; floor_index < 3; ++floor_index) {
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
    if (!state.resident_assigned) {
        constexpr float idle_y = layout::kRoomY[2] + 14.0f;
        mesh_.append_box({layout::kElevatorX + 11.0f, idle_y, -1.0f,
                          8.0f, 9.0f, 6.0f,
                          rgba(245, 210, 174), assets::GeneratedMaterial::Steel});
        mesh_.append_box({layout::kElevatorX + 8.0f, idle_y + 9.0f, -1.0f,
                          14.0f, 24.0f, 6.0f,
                          rgba(48, 119, 164), assets::GeneratedMaterial::ControlPanel});
    }
    const int active_rooms = std::clamp(state.rooms, 0, 6);
    for (int room_index = 0; room_index < 6; ++room_index) {
        const float x = layout::kRoomX[room_index];
        const float y = layout::kRoomY[room_index];
        if (!camera.visible(x, y, layout::kRoomWidth, layout::kRoomHeight)) {
            ++stats.culled_cells;
            continue;
        }
        ++stats.visible_cells;
        const bool active = room_index < active_rooms;
        append_room(x,
                    y,
                    room_index,
                    active,
                    active && room_index == state.selected_room,
                    active && state.resident_assigned && room_index == state.selected_room,
                    active && room_index == state.selected_room ? state.stored : 0);
    }
    stats.draw_calls = mesh_.vertex_count() > 0 ? 1 : 0;
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

    C3D_AttrInfo* attr_info = C3D_GetAttrInfo();
    AttrInfo_Init(attr_info);
    AttrInfo_AddLoader(attr_info, 0, GPU_FLOAT, 3);
    AttrInfo_AddLoader(attr_info, 1, GPU_FLOAT, 2);
    AttrInfo_AddLoader(attr_info, 2, GPU_FLOAT, 3);
    AttrInfo_AddLoader(attr_info, 3, GPU_FLOAT, 4);

    C3D_BufInfo* buf_info = C3D_GetBufInfo();
    BufInfo_Init(buf_info);
    BufInfo_Add(buf_info, vertex_buffer_, sizeof(Vertex3D), 4, 0x3210);

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
    const float eye_z = 1120.0f / zoom;

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
               FVec3_New(center_x, center_y, eye_z),
               FVec3_New(center_x, center_y, 0.0f),
               FVec3_New(0.0f, -1.0f, 0.0f),
               false);

    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, projection_uniform_, &projection);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, model_view_uniform_, &view);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);
    C3D_CullFace(GPU_CULL_NONE);
    C3D_DrawArrays(GPU_TRIANGLES, 0, static_cast<int>(mesh_.vertex_count()));
}

}  // namespace deep_shelter::render
