#include "render/Scene3D.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>

using deep_shelter::assets::GeneratedMaterial;
using deep_shelter::render::Box3D;
using deep_shelter::render::SceneMesh3D;
using deep_shelter::render::Vertex3D;

namespace {
struct Normal { float x; float y; float z; };
bool near(float left, float right) { return std::fabs(left - right) < 0.0001f; }
void expect_normal(const Vertex3D& vertex, const Normal& expected) {
    assert(near(vertex.nx, expected.x));
    assert(near(vertex.ny, expected.y));
    assert(near(vertex.nz, expected.z));
    const float length = std::sqrt(vertex.nx * vertex.nx +
                                   vertex.ny * vertex.ny +
                                   vertex.nz * vertex.nz);
    assert(near(length, 1.0f));
}
}  // namespace

int main() {
    static_assert(sizeof(Vertex3D) == 48);

    SceneMesh3D mesh;
    assert(mesh.append_box(Box3D{1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f}));
    assert(mesh.vertex_count() == 36);
    assert(mesh.box_count() == 1);
    assert(!mesh.overflowed());

    constexpr std::array<Normal, 6> expected{{
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f},
        {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f},
    }};
    const Vertex3D* vertices = mesh.data();
    for (std::size_t face = 0; face < expected.size(); ++face) {
        for (std::size_t vertex = 0; vertex < 6; ++vertex) {
            expect_normal(vertices[face * 6 + vertex], expected[face]);
        }
    }

    mesh.clear();
    assert(mesh.append_box(Box3D{0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
                                 0xffffffffu, GeneratedMaterial::ControlPanel}));
    vertices = mesh.data();
    constexpr float tile_width = 1.0f / 8.0f;
    constexpr float inset = 0.002f;
    assert(near(vertices[0].u, 7.0f * tile_width + inset));
    assert(near(vertices[1].u, 8.0f * tile_width - inset));

    mesh.clear();
    assert(mesh.vertex_count() == 0);
    assert(mesh.box_count() == 0);
    assert(!mesh.append_box(Box3D{0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f}));
    assert(mesh.vertex_count() == 0);
    return 0;
}
