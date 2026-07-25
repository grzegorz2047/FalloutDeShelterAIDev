#include "render/Scene3D.hpp"

namespace deep_shelter::render {
namespace {

constexpr std::size_t kVerticesPerBox = 36;
constexpr float kByteToUnit = 1.0f / 255.0f;
constexpr float kMaterialWidth =
    1.0f / static_cast<float>(assets::kGeneratedMaterialTileCount);
// Keep bilinear samples inside a material tile. At 16 pixels per tile this
// inset is a little over one texture-pixel in atlas UV space.
constexpr float kUvInset = 0.0045f;

struct Position3D { float x; float y; float z; };
struct Normal3D { float x; float y; float z; };

Vertex3D make_vertex(const Position3D& position,
                     float u,
                     float v,
                     const Normal3D& normal,
                     std::uint32_t color) noexcept {
    return {position.x, position.y, position.z, u, v,
            normal.x, normal.y, normal.z,
            static_cast<float>(color & 0xffu) * kByteToUnit,
            static_cast<float>((color >> 8) & 0xffu) * kByteToUnit,
            static_cast<float>((color >> 16) & 0xffu) * kByteToUnit,
            static_cast<float>((color >> 24) & 0xffu) * kByteToUnit};
}

void write_face(Vertex3D* out,
                const Position3D& top_left,
                const Position3D& top_right,
                const Position3D& bottom_right,
                const Position3D& bottom_left,
                const Normal3D& normal,
                float u0,
                float u1,
                std::uint32_t color) noexcept {
    constexpr float v0 = kUvInset;
    constexpr float v1 = 1.0f - kUvInset;
    const Vertex3D a = make_vertex(top_left, u0, v0, normal, color);
    const Vertex3D b = make_vertex(top_right, u1, v0, normal, color);
    const Vertex3D c = make_vertex(bottom_right, u1, v1, normal, color);
    const Vertex3D d = make_vertex(bottom_left, u0, v1, normal, color);
    out[0] = a;
    out[1] = b;
    out[2] = c;
    out[3] = a;
    out[4] = c;
    out[5] = d;
}

}  // namespace

void SceneMesh3D::clear() noexcept {
    vertex_count_ = 0;
    box_count_ = 0;
    overflowed_ = false;
}

bool SceneMesh3D::append_box(const Box3D& box) noexcept {
    if (box.width <= 0.0f || box.height <= 0.0f || box.depth <= 0.0f) {
        return false;
    }
    if (vertex_count_ + kVerticesPerBox > vertices_.size()) {
        overflowed_ = true;
        return false;
    }

    const float x0 = box.x;
    const float x1 = box.x + box.width;
    const float y0 = box.y;
    const float y1 = box.y + box.height;
    const float z0 = box.z;
    const float z1 = box.z + box.depth;
    const std::uint32_t color = box.color;
    const float material = static_cast<float>(box.material);
    const float u0 = material * kMaterialWidth + kUvInset;
    const float u1 = (material + 1.0f) * kMaterialWidth - kUvInset;

    const Position3D p000{x0, y0, z0};
    const Position3D p001{x0, y0, z1};
    const Position3D p010{x0, y1, z0};
    const Position3D p011{x0, y1, z1};
    const Position3D p100{x1, y0, z0};
    const Position3D p101{x1, y0, z1};
    const Position3D p110{x1, y1, z0};
    const Position3D p111{x1, y1, z1};

    constexpr Normal3D front{0.0f, 0.0f, 1.0f};
    constexpr Normal3D back{0.0f, 0.0f, -1.0f};
    constexpr Normal3D left{-1.0f, 0.0f, 0.0f};
    constexpr Normal3D right{1.0f, 0.0f, 0.0f};
    constexpr Normal3D bottom{0.0f, 1.0f, 0.0f};
    constexpr Normal3D top{0.0f, -1.0f, 0.0f};

    Vertex3D* out = vertices_.data() + vertex_count_;
    write_face(out + 0, p001, p101, p111, p011, front, u0, u1, color);
    write_face(out + 6, p100, p000, p010, p110, back, u0, u1, color);
    write_face(out + 12, p000, p001, p011, p010, left, u0, u1, color);
    write_face(out + 18, p101, p100, p110, p111, right, u0, u1, color);
    write_face(out + 24, p010, p011, p111, p110, bottom, u0, u1, color);
    write_face(out + 30, p000, p100, p101, p001, top, u0, u1, color);

    vertex_count_ += kVerticesPerBox;
    ++box_count_;
    return true;
}

}  // namespace deep_shelter::render
