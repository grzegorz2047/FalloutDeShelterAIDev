# True 2.5D renderer

The top screen uses Citro3D geometry rather than Citro2D room primitives. The shelter is a three-dimensional scene viewed through a permanently side-facing camera, producing a 2.5D cutaway presentation.

## Camera contract

- The view remains fixed but uses a shallow oblique yaw/pitch toward the
  shelter cutaway, exposing real floor, ceiling and side-wall depth in mono.
- Player input may pan in X/Y and adjust zoom, but cannot rotate, orbit or tilt the camera.
- Left and right eyes render the same scene with separate stereo projection matrices derived from the physical 3D slider.
- Room fronts remain open so the rear wall, floor, ceiling, side columns and equipment are visible.

## Geometry and memory budget

`SceneMesh3D` owns a fixed-capacity array of 8192 vertices and performs no heap allocation while building a frame.

- One box: 12 triangles / 36 vertices.
- Absolute fixed-capacity maximum: 227 complete boxes (8172 vertices).
- Vertex format: XYZ position, UV coordinates, XYZ unit normal and normalized RGBA tint: twelve 32-bit floats / 48 bytes.
- Maximum CPU mesh storage: 393,216 bytes.
- Maximum linear-memory VBO: 393,216 bytes.
- Project-supplied room texture/prop atlas: 512×256 RGBA5551 / 262,144 bytes.
- Generated dweller texture: 256×256 RGBA5551 / 131,072 bytes.
- Dweller CPU/VBO batch: at most 72 vertices per eye submission.
- Total fixed scene and dweller renderer data is approximately 1.15 MiB,
  excluding Citro3D target buffers.
- Overflow is detected by `SceneMesh3D::overflowed()` instead of writing past the buffer.

The current scene is camera-culled before geometry generation. Structural,
prop and foreground geometry use up to three draw calls per eye. Dwellers use
one additional alpha-tested draw call. The lower-screen interface remains in
Citro2D and is outside this scene budget.

## Surface normals and lighting

Each generated box face receives one exact axis-aligned unit normal: front/back use ±Z, side walls use ±X, and floor/ceiling use ±Y. The host test validates all 36 vertices of a generated box and verifies that every normal has unit length.

The vertex shader applies a fixed directional light and quantizes it into three
bands: 0.78, 0.89 and 1.0. This keeps supplied textures readable while still
separating floors, ceilings, side walls and front-facing equipment. There are
no dynamic lights, shadow maps or normal maps.

## Depth layers

The implementation uses several real Z ranges:

1. rock mass behind the shelter;
2. rear wall and structural shell;
3. room equipment;
4. production indicator and alpha-tested resident billboard nearest the open
   front.

Every generated box has a non-zero depth. Visibility and stereoscopy therefore come from model/view/projection transforms and the depth buffer, not from manually offsetting 2D rectangles.

## Room asset atlas

The 512×256 atlas contains eight supplied 64×64 surface materials and twelve
transparent equipment/furniture cells. The build embeds a PICA200-tiled
RGBA5551 binary, so no texture file is read and no texture memory is allocated
during a frame.

Shell faces select a material tile. Recognizable props are alpha-tested
billboards with explicit world sizes and Z layers. Bilinear filtering plus
transparent padding and half-texel UV insets limit atlas bleeding.

## Animated dwellers

Residents are not box placeholders. A dedicated unlit pass draws original
24×32 pixel-art frames as camera-facing quads. Five job archetypes share one
RGBA5551 texture; idle, work and walk each have four frames. Both stereo eyes
receive the same simulation tick, so animation never diverges between eyes.
Alpha-tested visible pixels write depth on an explicit character layer between
room props and the front floor lip/glow pass.
See [`GENERATED_DWELLER_ATLAS.md`](GENERATED_DWELLER_ATLAS.md).

## Asset policy

Geometry and generated residents remain original project work. Room textures
and props are the project-owner-supplied issue #85 pack, recorded in the asset
manifest under `LicenseRef-Project-Owner-Permission`. The runtime contains no
commercial-game extraction, logo or interface fragment.
