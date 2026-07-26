#include "render/Scene3DRenderer.hpp"

#include <algorithm>
#include <cstring>

#include "assets/GeneratedMaterialAtlas.hpp"
#include "assets/RoomAssetAtlas.hpp"
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
    const u32 variation =
        ((room & 1) == 0) ? rgba(155, 143, 129) : rgba(134, 126, 116);
    // An unbuilt slot is a deep excavation with no blueprint "prop". The long
    // floor slab makes the opening read as a volume even before construction.
    mesh.append_box({x + 3.0f, y + 4.0f, -58.0f, 126.0f, 55.0f, 5.0f,
                     rgba(112, 105, 96), assets::GeneratedMaterial::Rock});
    mesh.append_box({x + 9.0f, y + 10.0f, -52.0f, 114.0f, 40.0f, 4.0f,
                     variation, assets::GeneratedMaterial::ExcavatedRock});
    mesh.append_box({x + 3.0f, y + 51.0f, -52.0f, 126.0f, 9.0f, 54.0f,
                     rgba(90, 84, 76), assets::GeneratedMaterial::ExcavatedRock});
    mesh.append_box({x + 2.0f, y + 4.0f, -52.0f, 8.0f, 56.0f, 54.0f,
                     rgba(99, 92, 84), assets::GeneratedMaterial::Rock});
    mesh.append_box({x + 122.0f, y + 4.0f, -52.0f, 8.0f, 56.0f, 54.0f,
                     rgba(99, 92, 84), assets::GeneratedMaterial::Rock});
}

void append_shell(SceneMesh3D& mesh, float x, float y, int room) noexcept {
    const auto wall_material =
        room == 2 ? assets::GeneratedMaterial::Water
                  : (room == 1 ? assets::GeneratedMaterial::Hydroponic
                               : assets::GeneratedMaterial::VaultPanel);
    const u32 wall_tint =
        room == 2 ? rgba(190, 224, 235)
                  : (room == 1 ? rgba(204, 225, 193)
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
    mesh.append_box({x + 9.0f, y + 8.0f, -50.0f, 4.0f, 42.0f, 3.0f,
                     rgba(128, 138, 137), assets::GeneratedMaterial::Steel});
    mesh.append_box({x + 119.0f, y + 8.0f, -50.0f, 4.0f, 42.0f, 3.0f,
                     rgba(128, 138, 137), assets::GeneratedMaterial::Steel});
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
        if (active) {
            append_shell(mesh_, x, y, room);
        } else {
            append_cavity(mesh_, x, y, room);
        }
    }
    structure_vertex_end_ = mesh_.vertex_count();

    for (int room = 0; room < active_rooms; ++room) {
        const float x = layout::kRoomX[room];
        const float y = layout::kRoomY[room];
        if (!camera.visible(x, y, layout::kRoomWidth, layout::kRoomHeight)) {
            continue;
        }
        append_props(mesh_, x, y, room);
    }
    prop_vertex_end_ = mesh_.vertex_count();

    if (state.selected_room >= 0 && state.selected_room < active_rooms) {
        const int room = state.selected_room;
        const float x = layout::kRoomX[room];
        const float y = layout::kRoomY[room];
        if (camera.visible(x, y, layout::kRoomWidth, layout::kRoomHeight)) {
            if (state.stored > 0) {
                const float fill = std::clamp(
                    static_cast<float>(state.stored) / 30.0f, 0.0f, 1.0f);
                mesh_.append_box(
                    {x + 10.0f, y + 9.0f, -2.0f, 112.0f * fill, 3.0f, 4.0f,
                     rgba(112, 255, 177),
                     assets::GeneratedMaterial::ControlPanel});
            }
            append_selection(mesh_, x, y);
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
