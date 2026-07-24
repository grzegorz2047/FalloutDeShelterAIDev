#include "render/ShelterCamera.hpp"

#include <cassert>

using namespace deep_shelter::render;

int main() {
    ShelterCamera camera({1200.0f, 720.0f}, {400.0f, 240.0f});
    assert(camera.x() == 0.0f);
    assert(camera.y() == 0.0f);
    assert(camera.zoom() == 1.0f);

    camera.pan(5000.0f, 5000.0f);
    assert(camera.x() == 800.0f);
    assert(camera.y() == 480.0f);

    camera.pan(-5000.0f, -5000.0f);
    assert(camera.x() == 0.0f);
    assert(camera.y() == 0.0f);

    camera.zoom_by(10.0f);
    assert(camera.zoom() == 2.5f);
    camera.pan(5000.0f, 5000.0f);
    assert(camera.x() == 1040.0f);
    assert(camera.y() == 624.0f);

    assert(camera.visible(1030.0f, 620.0f, 30.0f, 30.0f));
    assert(!camera.visible(0.0f, 0.0f, 20.0f, 20.0f));

    camera.zoom_by(-10.0f);
    assert(camera.zoom() == 0.5f);
    assert(camera.x() <= 400.0f);
    assert(camera.y() <= 240.0f);

    camera.set_world({100.0f, 100.0f});
    assert(camera.x() == 0.0f);
    assert(camera.y() == 0.0f);
    assert(camera.visible(0.0f, 0.0f, 100.0f, 100.0f));
    return 0;
}
