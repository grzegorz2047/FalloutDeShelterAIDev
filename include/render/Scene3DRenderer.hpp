#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>

#include <3ds.h>
#include <citro3d.h>

#include "render/Scene3D.hpp"
#include "render/ShelterCamera.hpp"
#include "render/ShelterSceneLayout.hpp"

namespace deep_shelter::render {

// Citro2D's convenience target uses a 16-bit depth buffer for every screen.
// The shelter scene benefits from the PICA200's native 24-bit depth and
// 8-bit stencil support, while the flat lower-screen UI does not need it.
[[nodiscard]] inline C3D_RenderTarget* create_screen_target(
    gfxScreen_t screen,
    gfx3dSide_t side) noexcept {
    const int height = screen == GFX_TOP
                           ? (gfxIsWide() ? GSP_SCREEN_HEIGHT_TOP_2X
                                          : GSP_SCREEN_HEIGHT_TOP)
                           : GSP_SCREEN_HEIGHT_BOTTOM;
    const GPU_DEPTHBUF depth_format =
        screen == GFX_TOP ? GPU_RB_DEPTH24_STENCIL8 : GPU_RB_DEPTH16;

    C3D_RenderTarget* target = C3D_RenderTargetCreate(
        GSP_SCREEN_WIDTH,
        height,
        GPU_RB_RGBA8,
        depth_format);
    if (target == nullptr) return nullptr;

    C3D_RenderTargetSetOutput(
        target,
        screen,
        side,
        GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) |
            GX_TRANSFER_RAW_COPY(0) |
            GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) |
            GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) |
            GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
    return target;
}

struct RoomRenderEntry {
    int grid_column = -1;
    int grid_floor = -1;
    // Profiles 0..5 map to power, food/hydroponics, water, workshop,
    // storage and living silhouettes. Elevator entries ignore this field.
    int visual_profile = 0;
    int stored = 0;
    bool selected = false;
    bool elevator = false;
};

struct ResidentRenderEntry {
    // Simulation/pathfinding supplies continuous world-space coordinates.
    // They are deliberately independent from room slots so a resident can
    // visibly interpolate through corridors and the elevator.
    float world_x = 0.0f;
    float world_y = 0.0f;
    int archetype = 4;
    bool moving = false;
    bool working = false;
    std::uint32_t animation_phase = 0;
};

struct BuildPreviewRenderEntry {
    int grid_column = -1;
    int grid_floor = -1;
    int visual_profile = 0;
    bool elevator = false;
    bool valid = false;
    bool active = false;
};

struct ShelterSceneState3D {
    std::array<RoomRenderEntry, layout::kMaximumRoomEntries> rooms{};
    std::size_t room_count = 0;
    std::array<ResidentRenderEntry, layout::kMaximumResidentEntries>
        residents{};
    std::size_t resident_count = 0;
    BuildPreviewRenderEntry build_preview{};
    std::uint32_t animation_tick = 0;

    ShelterSceneState3D() noexcept = default;

    // Keeps the existing vertical-slice caller source-compatible while the
    // gameplay layer migrates to explicit render entries.
    ShelterSceneState3D(int legacy_room_count,
                        int legacy_selected_room,
                        int legacy_stored,
                        int legacy_resident_room,
                        std::uint32_t tick) noexcept;
};

static_assert(layout::kMaximumRoomEntries >= 48,
              "The render state must cover the complete 8x6 shelter grid");
static_assert(layout::kMaximumResidentEntries >=
                  layout::kMinimumResidentEntries,
              "The scene must support at least three visible residents");

class Scene3DRenderer {
public:
    Scene3DRenderer() = default;
    ~Scene3DRenderer();

    Scene3DRenderer(const Scene3DRenderer&) = delete;
    Scene3DRenderer& operator=(const Scene3DRenderer&) = delete;

    [[nodiscard]] bool initialize() noexcept;
    void shutdown() noexcept;

    void draw(C3D_RenderTarget* target,
              const ShelterCamera& camera,
              float stereo_eye,
              const ShelterSceneState3D& state,
              RenderStats& stats) noexcept;

private:
    void build_scene(const ShelterCamera& camera,
                     const ShelterSceneState3D& state,
                     RenderStats& stats) noexcept;

    SceneMesh3D mesh_{};
    shaderProgram_s program_{};
    DVLB_s* shader_dvlb_ = nullptr;
    Vertex3D* vertex_buffer_ = nullptr;
    C3D_Tex material_texture_{};
    std::size_t structure_vertex_end_ = 0;
    std::size_t prop_vertex_end_ = 0;
    int projection_uniform_ = -1;
    int model_view_uniform_ = -1;
    bool program_initialized_ = false;
    bool texture_initialized_ = false;
    bool initialized_ = false;
};

namespace telemetry {

constexpr const char* kPerformanceLogPath = "sdmc:/DeepShelter3D_perf.log";
constexpr const char* kBenchmarkFlagPath = "sdmc:/DeepShelter3D_benchmark.flag";

struct FrameBucket {
    double cpu_submission_ms = 0.0;
    double gpu_drawing_pct = 0.0;
    double gpu_processing_pct = 0.0;
    float command_buffer_peak_pct = 0.0f;
    unsigned int samples = 0;
};

struct FrameState {
    FrameBucket mono{};
    FrameBucket stereo{};
    u64 cpu_start_ms = 0;
    double previous_cpu_ms = 0.0;
    float previous_command_buffer_pct = 0.0f;
    bool current_stereo = false;
    bool previous_stereo = false;
    bool previous_valid = false;
    bool benchmark_sequence = false;
    bool benchmark_stereo = false;
    bool log_initialized = false;
};

[[nodiscard]] inline FrameState& state() noexcept {
    static FrameState value{};
    return value;
}

inline void initialize_log() noexcept {
    FrameState& value = state();
    if (value.log_initialized) return;

    std::remove(kPerformanceLogPath);
    FILE* flag = std::fopen(kBenchmarkFlagPath, "rb");
    if (flag != nullptr) {
        value.benchmark_sequence = true;
        std::fclose(flag);
        // The CI benchmark marker is intentionally one-shot. A normal launch
        // after the measurement returns to the physical 3D slider immediately.
        std::remove(kBenchmarkFlagPath);
    }
    value.log_initialized = true;
}

[[nodiscard]] inline bool benchmark_sequence_enabled() noexcept {
    initialize_log();
    return state().benchmark_sequence;
}

inline void emit_bucket(const char* mode, FrameBucket& bucket) noexcept {
    if (bucket.samples < 120) return;

    const double divisor = static_cast<double>(bucket.samples);
    char message[192];
    const int length = std::snprintf(
        message,
        sizeof(message),
        "DEEP_SHELTER_PERF mode=%s samples=%u cpu_submit_ms=%.3f "
        "gpu_draw_pct=%.2f gpu_process_pct=%.2f cmd_peak_pct=%.2f\n",
        mode,
        bucket.samples,
        bucket.cpu_submission_ms / divisor,
        bucket.gpu_drawing_pct / divisor,
        bucket.gpu_processing_pct / divisor,
        static_cast<double>(bucket.command_buffer_peak_pct));
    if (length > 0) {
        const std::size_t safe_length = static_cast<std::size_t>(
            length < static_cast<int>(sizeof(message)) ? length : sizeof(message) - 1);
        FILE* file = std::fopen(kPerformanceLogPath, "ab");
        if (file != nullptr) {
            std::fwrite(message, 1, safe_length, file);
            std::fclose(file);
        }
        // The SD log is the canonical telemetry sink. Calling
        // svcOutputDebugString here made Azahar jump through a null address
        // immediately after the mono bucket on some binary layouts.
    }

    FrameState& value = state();
    if (mode[0] == 'm' && value.benchmark_sequence) {
        // After the first complete mono bucket, measure the exact same scene
        // with the slider forced to its full stereoscopic separation.
        value.benchmark_stereo = true;
    }
    bucket = {};
}

inline void record_previous_frame() noexcept {
    FrameState& value = state();
    if (!value.previous_valid) return;

    FrameBucket& bucket = value.previous_stereo ? value.stereo : value.mono;
    bucket.cpu_submission_ms += value.previous_cpu_ms;
    // Citro3D's official examples convert these counters to percentage with *3.
    bucket.gpu_drawing_pct += static_cast<double>(C3D_GetDrawingTime() * 3.0f);
    bucket.gpu_processing_pct += static_cast<double>(C3D_GetProcessingTime() * 3.0f);
    if (value.previous_command_buffer_pct > bucket.command_buffer_peak_pct) {
        bucket.command_buffer_peak_pct = value.previous_command_buffer_pct;
    }
    ++bucket.samples;
    emit_bucket(value.previous_stereo ? "stereo" : "mono", bucket);
    value.previous_valid = false;
}

inline bool frame_begin(u8 flags) noexcept {
    initialize_log();
    record_previous_frame();
    const bool begun = C3D_FrameBegin(flags);
    if (begun) {
        FrameState& value = state();
        value.cpu_start_ms = osGetTime();
        value.current_stereo = false;
    }
    return begun;
}

inline void note_stereo_eye(float eye_separation) noexcept {
    if (eye_separation < -0.0001f || eye_separation > 0.0001f) {
        state().current_stereo = true;
    }
}

inline void perspective_stereo_tilt(C3D_Mtx* projection,
                                    float fov_y,
                                    float aspect_ratio,
                                    float near_plane,
                                    float far_plane,
                                    float eye_separation,
                                    float screen_distance,
                                    bool is_left_handed) noexcept {
    note_stereo_eye(eye_separation);
    Mtx_PerspStereoTilt(projection,
                        fov_y,
                        aspect_ratio,
                        near_plane,
                        far_plane,
                        eye_separation,
                        screen_distance,
                        is_left_handed);
}

inline void frame_end(u8 flags) noexcept {
    FrameState& value = state();
    const u64 cpu_end_ms = osGetTime();
    value.previous_cpu_ms = static_cast<double>(cpu_end_ms - value.cpu_start_ms);
    value.previous_command_buffer_pct = C3D_GetCmdBufUsage() * 100.0f;
    value.previous_stereo = value.current_stereo;
    value.previous_valid = true;
    C3D_FrameEnd(flags);
}

[[nodiscard]] inline float slider_state() noexcept {
    return state().benchmark_stereo ? 1.0f : osGet3DSliderState();
}

}  // namespace telemetry

}  // namespace deep_shelter::render

// main.cpp includes this header after citro2d.h. Redirect only subsequent calls
// to project-owned wrappers without changing Citro2D or Citro3D themselves.
#define C2D_CreateScreenTarget(screen, side) \
    ::deep_shelter::render::create_screen_target((screen), (side))
#define C3D_FrameBegin(flags) \
    ::deep_shelter::render::telemetry::frame_begin((flags))
#define Mtx_PerspStereoTilt(projection, fov_y, aspect_ratio, near_plane, far_plane, eye_separation, screen_distance, is_left_handed) \
    ::deep_shelter::render::telemetry::perspective_stereo_tilt( \
        (projection), (fov_y), (aspect_ratio), (near_plane), (far_plane), \
        (eye_separation), (screen_distance), (is_left_handed))
#define C3D_FrameEnd(flags) \
    ::deep_shelter::render::telemetry::frame_end((flags))
#define osGet3DSliderState() \
    ::deep_shelter::render::telemetry::slider_state()
