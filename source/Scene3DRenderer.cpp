#include "render/Scene3DRenderer.hpp"

#include <algorithm>
#include <cstring>

#include "scene3d_v_shbin.h"

namespace deep_shelter::render {
namespace {

constexpr int kColumns = 12;
constexpr int kRows = 7;
constexpr float kCellWidth = 72.0f;
constexpr float kCellHeight = 52.0f;
constexpr bool kForceDiagnosticFallback = true;

constexpr u32 rgba(u8 r, u8 g, u8 b, u8 a = 255) noexcept {
    return static_cast<u32>(r) | (static_cast<u32>(g) << 8) |
           (static_cast<u32>(b) << 16) | (static_cast<u32>(a) << 24);
}

u32 room_accent(int room_index) noexcept {
    switch ((room_index % 6 + 6) % 6) {
        case 0: return rgba(224, 153, 55);
        case 1: return rgba(89, 181, 104);
        case 2: return rgba(56, 145, 183);
        case 3: return rgba(181, 128, 66);
        case 4: return rgba(189, 143, 73);
        default: return rgba(89, 136, 139);
    }
}

}  // namespace

Scene3DRenderer::~Scene3DRenderer() {
    shutdown();
}

bool Scene3DRenderer::initialize() noexcept {
    if (initialized_) return true;
    if (kForceDiagnosticFallback) return false;

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

    initialized_ = true;
    return true;
}

void Scene3DRenderer::shutdown() noexcept {
    initialized_ = false;

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
    const u32 frame = selected ? rgba(235, 188, 72) : rgba(91, 103, 105);
    const u32 steel = rgba(53, 66, 68);
    const u32 dark = rgba(31, 39, 43);
    const u32 accent = room_accent(room_index);

    mesh_.append_box({x + 3.0f, y + 3.0f, -18.0f, 66.0f, 46.0f, 4.0f, steel});
    mesh_.append_box({x, y, -14.0f, 72.0f, 4.0f, 18.0f, frame});
    mesh_.append_box({x, y + 48.0f, -14.0f, 72.0f, 4.0f, 18.0f, frame});
    mesh_.append_box({x, y, -14.0f, 4.0f, 52.0f, 18.0f, frame});
    mesh_.append_box({x + 68.0f, y, -14.0f, 4.0f, 52.0f, 18.0f, frame});
    mesh_.append_box({x + 4.0f, y + 43.0f, -10.0f, 64.0f, 5.0f, 14.0f, dark});

    mesh_.append_box({x + 10.0f, y + 25.0f, -9.0f, 14.0f, 18.0f, 10.0f, accent});
    mesh_.append_box({x + 29.0f, y + 18.0f, -8.0f, 15.0f, 25.0f, 9.0f, accent});
    mesh_.append_box({x + 50.0f, y + 29.0f, -7.0f, 11.0f, 14.0f, 8.0f, accent});

    const float fill = std::clamp(static_cast<float>(stored) / 30.0f, 0.0f, 1.0f);
    if (fill > 0.0f) {
        mesh_.append_box({x + 8.0f, y + 7.0f, -5.0f, 52.0f * fill, 3.0f, 5.0f,
                          rgba(89, 211, 138)});
    }

    if (resident) {
        mesh_.append_box({x + 45.0f, y + 24.0f, -3.0f, 7.0f, 9.0f, 6.0f,
                          rgba(224, 191, 151)});
        mesh_.append_box({x + 43.0f, y + 33.0f, -3.0f, 11.0f, 10.0f, 6.0f,
                          rgba(54, 117, 126)});
    }
}

void Scene3DRenderer::build_scene(const ShelterCamera& camera,
                                  const ShelterSceneState3D& state,
                                  RenderStats& stats) noexcept {
    mesh_.clear();

    for (int row = 0; row < kRows; ++row) {
        for (int column = 0; column < kColumns; ++column) {
            const float x = static_cast<float>(column) * kCellWidth;
            const float y = static_cast<float>(row) * kCellHeight;
            if (!camera.visible(x, y, kCellWidth, kCellHeight)) {
                ++stats.culled_cells;
                continue;
            }
            ++stats.visible_cells;

            const bool excavated = row >= 2 && column >= 1 && column <= 10;
            const int room_index = column - 2;
            const bool room = row == 4 && room_index >= 0 && room_index < state.rooms;

            if (room) {
                append_room(x,
                            y,
                            room_index,
                            room_index == state.selected_room,
                            state.resident_assigned && room_index == state.selected_room,
                            room_index == state.selected_room ? state.stored : 0);
            } else if (excavated) {
                mesh_.append_box({x + 2.0f, y + 2.0f, -18.0f, 68.0f, 48.0f, 6.0f,
                                  rgba(28, 47, 51)});
                mesh_.append_box({x + 2.0f, y + 46.0f, -10.0f, 68.0f, 4.0f, 10.0f,
                                  rgba(72, 82, 80)});
            } else {
                const u8 shade = static_cast<u8>(35 + ((column * 13 + row * 7) & 15));
                mesh_.append_box({x, y, -24.0f, 70.0f, 50.0f, 22.0f,
                                  rgba(shade, static_cast<u8>(shade - 3),
                                       static_cast<u8>(shade - 1))});
            }
        }
    }

    stats.draw_calls = mesh_.vertex_count() > 0 ? 1 : 0;
    stats.estimated_linear_memory = mesh_.vertex_count() * sizeof(Vertex3D);
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
    AttrInfo_AddLoader(attr_info, 1, GPU_UNSIGNED_BYTE, 4);

    C3D_BufInfo* buf_info = C3D_GetBufInfo();
    BufInfo_Init(buf_info);
    BufInfo_Add(buf_info, vertex_buffer_, sizeof(Vertex3D), 2, 0x10);

    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

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