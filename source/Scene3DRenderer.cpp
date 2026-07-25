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
constexpr std::array<float, 6> kRoomX{{36.0f, 252.0f, 36.0f, 252.0f, 36.0f