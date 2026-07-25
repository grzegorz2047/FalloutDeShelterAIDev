#include "render/Scene3DRenderer.hpp"

#include <algorithm>
#include <array>
#include <cstring>

#include "assets/GeneratedMaterialAtlas.hpp"
#include "scene3d_v_shbin.h"

namespace deep_shelter::render {
namespace {

constexpr float kRoomWidth = 72.0f;
constexpr float kRoomHeight = 52.0f;
constexpr std::array<float, 6> kRoomX{{36.0f, 252.0f, 36.0f, 252.0f, 36.0f, 252.0f}};
constexpr std::array<float, 6> kRoomY{{42.0f, 42.0f, 104.0f, 104.0f, 166.0f, 166.0f}};

constexpr u32 rgba(u8 r, u8 g, u8 b, u8 a = 255) noexcept {
    return static_cast<u32>(r) | (static_cast<u32>(g) << 8) |
           (static_cast<u32>(b) << 16) | (static_cast<u32>(a) << 24);
}

u32 room_accent(int room_index) noexcept {
    switch ((room_index % 6 + 6) % 6) {
        case 0: return rgba(236, 172, 60);
        case 1: return rgba(91, 196, 111);
        case 2: return rgba(70, 164, 212);
        case 3: return rgba(211, 126, 63);
        case 4: return rgba(189, 154, 82);
        default: return rgba(101, 153, 181);
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
                                  bool selected,
                                  bool resident,
                                  int stored) noexcept {
    const u32 frame = selected ? rgba(255, 238, 145) : rgba(174, 197, 194);
    const u32 steel = rgba(169, 195, 193);
    const u32 dark = rgba(112, 137, 137);
    const u32 accent = room_accent(room_index);

    mesh_.append_box({x + 3.0f, y + 3.0f, -18.0f, 66.0f, 46.0f, 4.0f, steel,
                      assets::GeneratedMaterial::Steel});
    mesh_.append_box({x, y, -14.0f, kRoomWidth, 4.0f, 18.0f, frame,
                      assets::GeneratedMaterial::Steel});
    mesh_.append_box({x, y + 48.0f, -14.0f, kRoomWidth, 4.0f, 18.0f, frame,
                      assets::GeneratedMaterial::Steel});
    mesh_.append_box({x, y, -14.0f, 4.0f, kRoomHeight, 18.0f, frame,
                      assets::GeneratedMaterial::Steel});
    mesh_.append_box({x + 68.0f, y, -14.0f, 4.0f, kRoomHeight, 18.0f, frame,
                      assets::GeneratedMaterial::Steel});
    mesh_.append_box({x + 4.0f, y + 43.0f, -10.0f, 64.0f, 5.0f, 14.0f, dark,
                      assets::GeneratedMaterial::Grating});

    switch (room_index % 6) {
        case 0:  // Power generation.
            mesh_.append_box({x + 9.0f, y + 17.0f, -9.0f, 16.0f, 26.0f, 11.0f,
                              accent, assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 30.0f, y + 12.0f, -8.0f, 11.0f, 31.0f, 10.0f,
                              rgba(208, 218, 190), assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 47.0f, y + 21.0f, -7.0f, 14.0f, 22.0f, 9.0f,
                              accent, assets::GeneratedMaterial::ControlPanel});
            break;
        case 1:  // Hydroponics.
            mesh_.append_box({x + 8.0f, y + 33.0f, -8.0f, 54.0f, 9.0f, 9.0f,
                              rgba(114, 190, 106), assets::GeneratedMaterial::Grating});
            mesh_.append_box({x + 13.0f, y + 23.0f, -5.0f, 8.0f, 10.0f, 5.0f,
                              accent, assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 31.0f, y + 20.0f, -5.0f, 9.0f, 13.0f, 5.0f,
                              accent, assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 50.0f, y + 25.0f, -5.0f, 7.0f, 8.0f, 5.0f,
                              accent, assets::GeneratedMaterial::ControlPanel});
            break;
        case 2:  // Water treatment.
            mesh_.append_box({x + 9.0f, y + 16.0f, -9.0f, 17.0f, 27.0f, 11.0f,
                              accent, assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 29.0f, y + 24.0f, -8.0f, 29.0f, 19.0f, 10.0f,
                              rgba(102, 183, 211), assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 33.0f, y + 10.0f, -5.0f, 4.0f, 14.0f, 5.0f,
                              rgba(173, 219, 230), assets::GeneratedMaterial::Steel});
            break;
        case 3:  // Workshop.
            mesh_.append_box({x + 8.0f, y + 31.0f, -8.0f, 55.0f, 12.0f, 10.0f,
                              rgba(149, 124, 91), assets::GeneratedMaterial::Grating});
            mesh_.append_box({x + 13.0f, y + 18.0f, -6.0f, 17.0f, 13.0f, 7.0f,
                              accent, assets::GeneratedMaterial::ControlPanel});
            mesh_.append_box({x + 42.0f, y + 15.0f, -7.0f, 13.0f, 16.0f, 8.0f,
                              rgba(207, 176, 117), assets::GeneratedMaterial::Steel});
            break;
        case 4:  // Storage.
            for (int crate = 0; crate < 3; ++crate) {
                mesh_.append_box({x + 9.0f + crate * 18.0f, y + 27.0f, -7.0f,
                                  14.0f, 16.0f, 8.0f, accent,
                                  assets::GeneratedMaterial::Steel});
            }
            mesh_.append_box({x + 14.0f, y + 15.0f, -5.0f, 44.0f, 5.0f, 5.0f,
                              rgba(185, 198, 177), assets::GeneratedMaterial::Grating});
            break;
        default:  // Living quarters.
            mesh_.append_box({x + 8.0f, y + 30.0f, -7.0f, 25.0f, 13.0f, 8.0f,
                              rgba(167, 132, 103), assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 40.0f, y + 31.0f, -7.0f, 22.0f, 12.0f, 8.0f,
                              accent, assets::GeneratedMaterial::Steel});
            mesh_.append_box({x + 30.0f, y + 17.0f, -5.0f, 10.0f, 8.0f, 6.0f,
                              rgba(229, 208, 157), assets::GeneratedMaterial::ControlPanel});
            break;
    }

    const float fill = std::clamp(static_cast<float>(stored) / 30.0f, 0.0f, 1.0f);
    if (fill > 0.0f) {
        mesh_.append_box({x + 8.0f, y + 7.0f, -4.0f, 52.0f * fill, 3.0f, 5.0f,
                          rgba(110, 255, 171), assets::GeneratedMaterial::Grating});
    }

    if (resident) {
        mesh_.append_box({x + 45.0f, y + 22.0f, -3.0f, 7.0f, 9.0f, 6.0f,
                          rgba(255, 220, 183), assets::GeneratedMaterial::Steel});
        mesh_.append_box({x + 43.0f, y + 31.0f, -3.0f, 11.0f, 12.0f, 6.0f,
                          rgba(95, 196, 210), assets::GeneratedMaterial::Steel});
    }

    if (selected) {
        constexpr u32 highlight = 0xFFF2A4FF;
        mesh_.append_box({x - 3.0f, y - 3.0f, -2.0f, 78.0f, 3.0f, 4.0f,
                          highlight, assets::GeneratedMaterial::Steel});
        mesh_.append_box({x - 3.0f, y + 52.0f, -2.0f, 78.0f, 3.0f, 4.0f,
                          highlight, assets::GeneratedMaterial::Steel});
        mesh_.append_box({x - 3.0f, y, -2.0f, 3.0f, 52.0f, 4.0f,
                          highlight, assets::GeneratedMaterial::Steel});
        mesh_.append_box({x + 72.0f, y, -2.0f, 3.0f, 52.0f, 4.0f,
                          highlight, assets::GeneratedMaterial::Steel});
    }
}

void Scene3DRenderer::build_scene(const ShelterCamera& camera,
                                  const ShelterSceneState3D& state,
                                  RenderStats& stats) noexcept {
    mesh_.clear();

    // Carved rock backdrop around the three-floor cutaway.
    mesh_.append_box({8.0f, 24.0f, -28.0f, 384.0f, 210.0f, 10.0f,
                      rgba(147, 139, 132), assets::GeneratedMaterial::Rock});

    // Excavated central cavity and structural floor bands.
    mesh_.append_box({24.0f, 32.0f, -18.0f, 352.0f, 194.0f, 5.0f,
                      rgba(106, 129, 129), assets::GeneratedMaterial::Steel});
    for (int floor = 0; floor < 3; ++floor) {
        const float y = 90.0f + floor * 62.0f;
        mesh_.append_box({24.0f, y, -10.0f, 352.0f, 6.0f, 12.0f,
                          rgba(146, 164, 159), assets::GeneratedMaterial::Grating});
    }

    // Central elevator shaft, doors and illuminated cabin marker.
    mesh_.append_box({182.0f, 36.0f, -14.0f, 36.0f, 184.0f, 18.0f,
                      rgba(138, 151, 151), assets::GeneratedMaterial::Steel});
    for (int floor = 0; floor < 3; ++floor) {
        const float door_y = 50.0f + floor * 62.0f;
        mesh_.append_box({187.0f, door_y, -5.0f, 26.0f, 31.0f, 7.0f,
                          floor == 1 ? rgba(228, 148, 68) : rgba(102, 126, 128),
                          assets::GeneratedMaterial::ControlPanel});
        mesh_.append_box({194.0f, door_y + 6.0f, -2.0f, 12.0f, 18.0f, 4.0f,
                          rgba(192, 206, 199), assets::GeneratedMaterial::Steel});
    }

    const int visible_rooms = std::clamp(std::max(state.rooms, 3), 0, 6);
    for (int room_index = 0; room_index < visible_rooms; ++room_index) {
        const float x = kRoomX[room_index];
        const float y = kRoomY[room_index];
        if (!camera.visible(x, y, kRoomWidth, kRoomHeight)) {
            ++stats.culled_cells;
            continue;
        }
        ++stats.visible_cells;
        append_room(x,
                    y,
                    room_index,
                    room_index == state.selected_room,
                    state.resident_assigned && room_index == state.selected_room,
                    room_index == state.selected_room ? state.stored : 0);
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

    C3D_RenderTargetClear(target, C3D_CLEAR_ALL, 0x08101AFF, 0);
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
    const float eye_z = 900.0f / zoom;

    C3D_Mtx projection;
    Mtx_PerspStereoTilt(&projection,
                        C3D_AngleFromDegrees(28.0f),
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
