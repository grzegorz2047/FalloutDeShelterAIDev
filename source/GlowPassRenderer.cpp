#include "render/GlowPassRenderer.hpp"

#include <algorithm>
#include <cstring>

#include "render/ShelterSceneLayout.hpp"
#include "scene3d_v_shbin.h"

namespace deep_shelter::render {
namespace {

constexpr float kByteToUnit = 1.0f / 255.0f;

u32 accent_for_room(int room, u8 alpha) noexcept {
    switch ((room % 6 + 6) % 6) {
        case 0: return 244u | (177u << 8) | (61u << 16) | (static_cast<u32>(alpha) << 24);
        case 1: return 102u | (210u << 8) | (116u << 16) | (static_cast<u32>(alpha) << 24);
        case 2: return 78u | (179u << 8) | (224u << 16) | (static_cast<u32>(alpha) << 24);
        case 3: return 221u | (135u << 8) | (72u << 16) | (static_cast<u32>(alpha) << 24);
        case 4: return 207u | (173u << 8) | (94u << 16) | (static_cast<u32>(alpha) << 24);
        default: return 122u | (174u << 8) | (204u << 16) | (static_cast<u32>(alpha) << 24);
    }
}

Vertex3D glow_vertex(float x, float y, float z, u32 color) noexcept {
    return {x,
            y,
            z,
            0.5f,
            0.5f,
            0.0f,
            0.0f,
            1.0f,
            static_cast<float>(color & 0xffu) * kByteToUnit,
            static_cast<float>((color >> 8) & 0xffu) * kByteToUnit,
            static_cast<float>((color >> 16) & 0xffu) * kByteToUnit,
            static_cast<float>((color >> 24) & 0xffu) * kByteToUnit};
}

}  // namespace

GlowPassRenderer::~GlowPassRenderer() { shutdown(); }

bool GlowPassRenderer::initialize() noexcept {
    if (initialized_) return true;
    shutdown();

    shader_dvlb_ = DVLB_ParseFile(reinterpret_cast<u32*>(scene3d_v_shbin),
                                  scene3d_v_shbin_size);
    if (shader_dvlb_ == nullptr) return false;

    shaderProgramInit(&program_);
    program_initialized_ = true;
    shaderProgramSetVsh(&program_, &shader_dvlb_->DVLE[0]);
    projection_uniform_ = shaderInstanceGetUniformLocation(program_.vertexShader,
                                                            "projection");
    model_view_uniform_ = shaderInstanceGetUniformLocation(program_.vertexShader,
                                                            "modelView");
    if (projection_uniform_ < 0 || model_view_uniform_ < 0) {
        shutdown();
        return false;
    }

    vertex_buffer_ = static_cast<Vertex3D*>(
        linearAlloc(sizeof(Vertex3D) * kMaxGlowVertices));
    if (vertex_buffer_ == nullptr) {
        shutdown();
        return false;
    }

    initialized_ = true;
    return true;
}

void GlowPassRenderer::shutdown() noexcept {
    initialized_ = false;
    vertex_count_ = 0;
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
    projection_uniform_ = -1;
    model_view_uniform_ = -1;
}

bool GlowPassRenderer::append_quad(float x,
                                   float y,
                                   float z,
                                   float width,
                                   float height,
                                   u32 color) noexcept {
    if (width <= 0.0f || height <= 0.0f ||
        vertex_count_ + 6 > vertices_.size()) {
        return false;
    }

    const Vertex3D top_left = glow_vertex(x, y, z, color);
    const Vertex3D top_right = glow_vertex(x + width, y, z, color);
    const Vertex3D bottom_right = glow_vertex(x + width, y + height, z, color);
    const Vertex3D bottom_left = glow_vertex(x, y + height, z, color);
    Vertex3D* out = vertices_.data() + vertex_count_;
    out[0] = top_left;
    out[1] = top_right;
    out[2] = bottom_right;
    out[3] = top_left;
    out[4] = bottom_right;
    out[5] = bottom_left;
    vertex_count_ += 6;
    return true;
}

void GlowPassRenderer::build(const ShelterCamera& camera,
                             const ShelterSceneState3D& state) noexcept {
    vertex_count_ = 0;
    const int active_rooms = std::clamp(state.rooms, 0, 6);
    for (int room = 0; room < active_rooms; ++room) {
        const float x = layout::kRoomX[room];
        const float y = layout::kRoomY[room];
        if (!camera.visible(x, y, layout::kRoomWidth, layout::kRoomHeight)) continue;

        append_quad(x + 15.0f, y + 6.0f, 0.5f, 102.0f, 3.5f,
                    accent_for_room(room, 104));
        append_quad(x + 26.0f, y + 44.0f, 0.4f, 80.0f, 2.5f,
                    accent_for_room(room, 48));
    }

    if (state.selected_room >= 0 && state.selected_room < active_rooms) {
        const int room = state.selected_room;
        const float x = layout::kRoomX[room];
        const float y = layout::kRoomY[room];
        if (camera.visible(x, y, layout::kRoomWidth, layout::kRoomHeight)) {
            const u32 selection = 86u | (199u << 8) | (240u << 16) |
                                  (static_cast<u32>(126) << 24);
            append_quad(x - 2.0f, y - 2.0f, 1.0f, layout::kRoomWidth + 4.0f, 3.0f,
                        selection);
            append_quad(x - 2.0f, y + layout::kRoomHeight - 1.0f, 1.0f,
                        layout::kRoomWidth + 4.0f, 3.0f, selection);
            append_quad(x - 2.0f, y + 1.0f, 1.0f, 3.0f,
                        layout::kRoomHeight - 2.0f, selection);
            append_quad(x + layout::kRoomWidth - 1.0f, y + 1.0f, 1.0f, 3.0f,
                        layout::kRoomHeight - 2.0f, selection);
        }
    }
}

void GlowPassRenderer::draw(C3D_RenderTarget* target,
                            const ShelterCamera& camera,
                            float stereo_eye,
                            const ShelterSceneState3D& state) noexcept {
    if (!initialized_ || target == nullptr) return;
    build(camera, state);
    if (vertex_count_ == 0) return;

    std::memcpy(vertex_buffer_, vertices_.data(), vertex_count_ * sizeof(Vertex3D));
    GSPGPU_FlushDataCache(vertex_buffer_, vertex_count_ * sizeof(Vertex3D));

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

    C3D_TexEnv* env = C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env, C3D_Both, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR,
                  GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(env, C3D_Both, GPU_REPLACE);

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

    C3D_CullFace(GPU_CULL_NONE);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD,
                   GPU_SRC_ALPHA, GPU_ONE,
                   GPU_SRC_ALPHA, GPU_ONE);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_COLOR);
    C3D_DrawArrays(GPU_TRIANGLES, 0, static_cast<int>(vertex_count_));

    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD,
                   GPU_ONE, GPU_ZERO,
                   GPU_ONE, GPU_ZERO);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);
}

}  // namespace deep_shelter::render
