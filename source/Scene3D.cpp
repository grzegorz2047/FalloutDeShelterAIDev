#include "render/Scene3D.hpp"

namespace deep_shelter::render {
namespace {

constexpr std::size_t kVerticesPerBox = 36;

void write_triangle(Vertex3D* out,
                    const Vertex3D& a,
                    const Vertex3D& b,
                    const Vertex3D& c) noexcept {
    out[0] = a;
    out[1] = b;
    out[2] = c;
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
    const std::uint32_t c = box.color;

    const Vertex3D p000{x0, y0, z0, c};
    const Vertex3D p001{x0, y0, z1, c};
    const Vertex3D p010{x0, y1, z0, c};
    const Vertex3D p011{x0, y1, z1, c};
    const Vertex3D p100{x1, y0, z0, c};
    const Vertex3D p101{x1, y0, z1, c};
    const Vertex3D p110{x1, y1, z0, c};
    const Vertex3D p111{x1, y1, z1, c};

    Vertex3D* out = vertices_.data() + vertex_count_;

    write_triangle(out + 0, p001, p101, p111);
    write_triangle(out + 3, p001, p111, p011);
    write_triangle(out + 6, p100, p000, p010);
    write_triangle(out + 9, p100, p010, p110);
    write_triangle(out + 12, p000, p001, p011);
    write_triangle(out + 15, p000, p011, p010);
    write_triangle(out + 18, p101, p100, p110);
    write_triangle(out + 21, p101, p110, p111);
    write_triangle(out + 24, p010, p011, p111);
    write_triangle(out + 27, p010, p111, p110);
    write_triangle(out + 30, p000, p100, p101);
    write_triangle(out + 33, p000, p101, p001);

    vertex_count_ += kVerticesPerBox;
    ++box_count_;
    return true;
}

}  // namespace deep_shelter::render
