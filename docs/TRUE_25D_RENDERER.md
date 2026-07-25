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
- Vertex format: three 32-bit floats plus packed RGBA, 16 bytes per vertex.
- Maximum CPU mesh storage: 65,536 bytes.
- Maximum linear-memory VBO: 65,536 bytes.
- Total fixed vertex storage: 128 KiB.
- Overflow is detected by `SceneMesh3D::overflowed()` instead of writing past the buffer.

The current reference scene is camera-culled before geometry generation and submits all visible geometry in one draw call per eye. The lower-screen interface remains in Citro2D and is outside this scene budget.

## Depth layers

The first implementation uses several real Z ranges:

1. rock mass behind the shelter;
2. rear wall and structural shell;
3. room equipment;
4. production indicator and resident geometry nearest the open front.

Every generated box has a non-zero depth. Visibility and stereoscopy therefore come from model/view/projection transforms and the depth buffer, not from manually offsetting 2D rectangles.

## Asset policy

The renderer creates original procedural geometry and colours. It contains no models, textures, interface art or extracted material from Fallout Shelter or another commercial game.
