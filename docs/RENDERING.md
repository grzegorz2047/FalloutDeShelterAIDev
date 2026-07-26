# Shelter rendering

The upper screen renders a deterministic cross-section of the shelter. The lower screen is reserved for controls, selection context and diagnostics.

## Camera

`ShelterCamera` stores world-space position and zoom independently from
simulation state. It clamps to legal world bounds after every pan, zoom and
map-size change. When the zoomed viewport is larger than the 400×240 world, it
centres the world instead of anchoring it in a corner. The Circle Pad pans;
`X`/`Y` change zoom. Camera tests cover all edges, small worlds, extreme zoom
and culling.

## Layers

The stable draw order is:

1. background;
2. rock and empty cells;
3. excavated cells;
4. rooms;
5. alpha-tested animated resident billboards;
6. glow pass;
7. lower-screen UI and diagnostics.

Missing visual assets use geometric placeholders and never prevent the simulation from loading.

## Stereo 3D

Both top-screen eyes are rendered from the same immutable scene snapshot. The physical slider only changes bounded horizontal parallax. It never advances simulation or changes selection, pathfinding, saves or rewards. At slider value zero the two views converge.

## Performance contract

Only cells intersecting the camera viewport are submitted. `RenderStats`
publishes visible and culled cell counts, draw calls and an estimated transient
memory footprint. The scene and resident batches use fixed-capacity storage and
perform no per-frame texture allocation. Old 3DS/Azahar performance is recorded
by the CI benchmark.
