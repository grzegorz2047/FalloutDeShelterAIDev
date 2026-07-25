# True 2.5D renderer

The top screen uses Citro3D geometry rather than Citro2D room primitives. The shelter is a three-dimensional scene viewed through a permanently side-facing camera, producing a 2.5D cutaway presentation.

## Camera contract

- The view direction is fixed along the Z axis toward the shelter cutaway.
- Player input may pan in X/Y and adjust zoom, but cannot rotate, orbit or tilt the camera.
- Left and right eyes render the same scene with separate stereo projection matrices derived from the physical 3D slider.
- Room fronts remain open so the rear wall, floor, ceiling, side columns and equipment are visible.

## Geometry and memory budget

`SceneMesh3D` owns a fixed-capacity array of 4096 vertices and performs no heap allocation while building a frame.

- One box: 12 triangles / 36 vertices.
- Absolute fixed-capacity maximum: 113 complete boxes (4068 vertices).
- Vertex format: XYZ position, UV coordinates, XYZ unit normal and normalized RGBA tint: twelve 32-bit floats / 48 bytes.
- Maximum CPU mesh storage: 196,608 bytes.
- Maximum linear-memory VBO: 196,608 bytes.
- Generated material texture: 64×16 RGB565 / 2,048 bytes.
- Compressed source atlas: 512 bytes of 4 bpp indices plus a 32-byte RGB565 palette.
- Total fixed renderer data is approximately 386 KiB, excluding Citro3D target buffers.
- Overflow is detected by `SceneMesh3D::overflowed()` instead of writing past the buffer.

The current reference scene is camera-culled before geometry generation and submits all visible geometry in one draw call per eye. The material atlas is bound once per eye and does not add draw calls. The lower-screen interface remains in Citro2D and is outside this scene budget.

## Surface normals and lighting

Each generated box face receives one exact axis-aligned unit normal: front/back use ±Z, side walls use ±X, and floor/ceiling use ±Y. The host test validates all 36 vertices of a generated box and verifies that every normal has unit length.

The vertex shader applies a fixed directional light aligned with the permanently side-facing camera. Lighting is intentionally inexpensive: `ambient 0.42 + max(dot(normal, light), 0) × diffuse 0.58`. Ambient light keeps back-facing surfaces readable, while the directional term separates floors, ceilings, side walls and front-facing equipment. There are no dynamic lights, shadow maps, extra textures or additional draw calls.

## Depth layers

The implementation uses several real Z ranges:

1. rock mass behind the shelter;
2. rear wall and structural shell;
3. room equipment;
4. production indicator and resident geometry nearest the open front.

Every generated box has a non-zero depth. Visibility and stereoscopy therefore come from model/view/projection transforms and the depth buffer, not from manually offsetting 2D rectangles.

## Generated material atlas

The 64×16 atlas contains four 16×16 materials: rock, steel, floor grating and control-panel detail. Source pixels are stored as a shared 16-colour RGB565 palette and packed 4 bpp indices. At startup they are decoded directly into the PICA200 8×8 Morton-tiled texture allocation. No texture files are read and no texture memory is allocated during a frame.

Each box face receives UV coordinates within one material tile. Nearest filtering and a small UV inset avoid bleeding between adjacent tiles while preserving crisp details at Nintendo 3DS resolution.

## Asset policy

The geometry and material artwork are original assets created specifically for Deep Shelter 3D. The project does not contain extracted models, textures, interface art, logos or other material from Fallout Shelter or another commercial game.
