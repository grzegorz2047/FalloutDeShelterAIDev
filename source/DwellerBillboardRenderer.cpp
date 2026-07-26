#include "render/DwellerBillboardRenderer.hpp"

#include <algorithm>
#include <cstring>

#include "assets/GeneratedDwellerAtlas.hpp"
#include "dweller_v_shbin.h"
#include "render/RoomVisualProfiles.hpp"
#include "render/ShelterSceneLayout.hpp"

namespace deep_shelter::render {
namespace {

constexpr float kDwellerWorldWidth = 21.0f;
constexpr float kDwellerWorldHeight = 34.0f;

Vertex3D dweller_vertex(float x,
                        float y,
                        float z,
                        float u,
                        float v) noexcept {
    return {x, y, z, u, v, 0.0f, 0.0f, 1.0f,
            1.0f, 1.0f, 1.0f, 1.0f};
}

}  // namespace

DwellerBillboardRenderer::~DwellerBillboardRenderer() {
    shutdown();
}

bool DwellerBillboardRenderer::initialize() noexcept {
    if (initialized_) return true;
    shutdown();

    shader_dvlb_ = DVLB_ParseFile(reinterpret_cast<u32*>(dweller_v_shbin),
                                  dweller_v_shbin_size);
    if (shader_dvlb_ == nullptr) return false;

    shaderProgramInit(&program_);
    program_initialized_ = true;
    shaderProgramSetVsh(&program_, &shader_dvlb_->DVLE[0]);
    projection_uniform_ =
        shaderInstanceGetUniformLocation(program_.vertexShader, "projection");
    model_view_uniform_ =
        shaderInstanceGetUniformLocation(program_.vertexShader, "modelView");
    if (projection_uniform_ < 0 || model_view_uniform_ < 0) {
        shutdown();
        return false;
    }

    vertex_buffer_ = static_cast<Vertex3D*>(
        linearAlloc(sizeof(Vertex3D) * kMaxVertices));
    if (vertex_buffer_ == nullptr) {
        shutdown();
        return false;
    }

    if (!C3D_TexInit(&texture_,
                     static_cast<u16>(assets::kGeneratedDwellerAtlasWidth),
                     static_cast<u16>(assets::kGeneratedDwellerAtlasHeight),
                     GPU_RGBA5551)) {
        shutdown();
        return false;
    }
    texture_initialized_ = true;
    assets::decode_generated_dweller_atlas_tiled(
        static_cast<std::uint16_t*>(texture_.data),
        assets::kGeneratedDwellerPixelCount);
    C3D_TexFlush(&texture_);
    C3D_TexSetFilter(&texture_, GPU_NEAREST, GPU_NEAREST);
    C3D_TexSetWrap(&texture_, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);

    initialized_ = true;
    return true;
}

void DwellerBillboardRenderer::shutdown() noexcept {
    initialized_ = false;
    vertex_count_ = 0;
    if (texture_initialized_) {
        C3D_TexDelete(&texture_);
        texture_ = {};
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
    projection_uniform_ = -1;
    model_view_uniform_ = -1;
}

bool DwellerBillboardRenderer::append_dweller(
    float x,
    float y,
    float z,
    int room_index,
    bool working,
    std::uint32_t simulation_tick,
    std::uint32_t phase) noexcept {
    if (vertex_count_ + kVerticesPerDweller > vertices_.size()) return false;

    const auto animation = working ? assets::DwellerAnimation::Work
                                   : assets::DwellerAnimation::Idle;
    const auto archetype = working
                               ? assets::dweller_archetype_for_room(room_index)
                               : assets::DwellerArchetype::Civilian;
    const std::size_t frame = assets::dweller_animation_frame(
        simulation_tick, animation, phase);
    const auto region = assets::dweller_atlas_region(
        archetype, animation, frame);

    constexpr float atlas_width =
        static_cast<float>(assets::kGeneratedDwellerAtlasWidth);
    constexpr float atlas_height =
        static_cast<float>(assets::kGeneratedDwellerAtlasHeight);
    const float u0 = (static_cast<float>(region.x) + 0.25f) / atlas_width;
    const float u1 =
        (static_cast<float>(region.x + region.width) - 0.25f) / atlas_width;
    const float v_top =
        1.0f - (static_cast<float>(region.y) + 0.25f) / atlas_height;
    const float v_bottom =
        1.0f -
        (static_cast<float>(region.y + region.height) - 0.25f) / atlas_height;

    const Vertex3D top_left = dweller_vertex(x, y, z, u0, v_top);
    const Vertex3D top_right =
        dweller_vertex(x + kDwellerWorldWidth, y, z, u1, v_top);
    const Vertex3D bottom_right = dweller_vertex(
        x + kDwellerWorldWidth, y + kDwellerWorldHeight, z, u1, v_bottom);
    const Vertex3D bottom_left = dweller_vertex(
        x, y + kDwellerWorldHeight, z, u0, v_bottom);

    Vertex3D* out = vertices_.data() + vertex_count_;
    out[0] = top_left;
    out[1] = top_right;
    out[2] = bottom_right;
    out[3] = top_left;
    out[4] = bottom_right;
    out[5] = bottom_left;
    vertex_count_ += kVerticesPerDweller;
    return true;
}

void DwellerBillboardRenderer::build(
    const ShelterCamera& camera,
    const ShelterSceneState3D& state) noexcept {
    vertex_count_ = 0;
    const int active_rooms = std::clamp(state.rooms, 0, 6);
    if (state.resident_room >= 0 &&
        state.resident_room < active_rooms) {
        const int room = state.resident_room;
        const float room_x = layout::kRoomX[room];
        const float room_y = layout::kRoomY[room];
        if (!camera.visible(room_x,
                            room_y,
                            layout::kRoomWidth,
                            layout::kRoomHeight)) {
            return;
        }
        const float resident_x =
            room_x + room_visual_profile(room).resident_clear_x;
        append_dweller(resident_x,
                        room_y + 17.0f,
                        1.5f,
                        room,
                        true,
                        state.animation_tick,
                        0u);
        return;
    }

    const int floor = std::clamp(state.selected_room / 2, 0, 2);
    const float elevator_y = layout::kRoomY[floor * 2] + 18.0f;
    if (camera.visible(layout::kElevatorX,
                       elevator_y,
                       layout::kElevatorWidth,
                       kDwellerWorldHeight)) {
        append_dweller(layout::kElevatorX + 4.5f,
                        elevator_y,
                        1.5f,
                        5,
                        false,
                        state.animation_tick,
                        1u);
    }
}

void DwellerBillboardRenderer::draw(
    C3D_RenderTarget* target,
    const ShelterCamera& camera,
    float stereo_eye,
    const ShelterSceneState3D& state,
    RenderStats& stats) noexcept {
    if (!initialized_ || target == nullptr) return;
    build(camera, state);
    if (vertex_count_ == 0) return;

    std::memcpy(vertex_buffer_,
                vertices_.data(),
                vertex_count_ * sizeof(Vertex3D));
    GSPGPU_FlushDataCache(vertex_buffer_,
                          vertex_count_ * sizeof(Vertex3D));

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

    C3D_TexBind(0, &texture_);
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

    C3D_CullFace(GPU_CULL_NONE);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD,
                   GPU_ONE, GPU_ZERO,
                   GPU_ONE, GPU_ZERO);
    C3D_AlphaTest(true, GPU_GREATER, 0);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);
    C3D_DrawArrays(GPU_TRIANGLES, 0, static_cast<int>(vertex_count_));

    C3D_AlphaTest(false, GPU_ALWAYS, 0);
    C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD,
                   GPU_ONE, GPU_ZERO,
                   GPU_ONE, GPU_ZERO);
    C3D_DepthTest(true, GPU_GEQUAL, GPU_WRITE_ALL);
    ++stats.draw_calls;
    stats.estimated_linear_memory +=
        assets::kGeneratedDwellerRuntimeBytes +
        kMaxVertices * sizeof(Vertex3D);
}

}  // namespace deep_shelter::render
