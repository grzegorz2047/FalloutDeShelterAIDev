# Shelter rendering

The upper screen renders a deterministic cross-section of the shelter. The lower screen is reserved for controls, selection context and diagnostics.

## Camera

`ShelterCamera` stores world-space position and zoom independently from simulation state. It clamps to legal world bounds after every pan, zoom and map-size change. The circle pad pans; L/R change zoom. Camera tests cover all edges, small worlds, extreme zoom and culling.

## Layers

The stable draw order is:

1. background;
2. rock and empty cells;
3. excavated cells;
4. rooms;
5. resident and object placeholders;
6. lower-screen UI and diagnostics.

Missing visual assets use geometric placeholders and never prevent the simulation from loading.

## Stereo 3D

Both top-screen eyes are rendered from the same immutable scene snapshot. The physical slider only changes bounded horizontal parallax. It never advances simulation or changes selection, pathfinding, saves or rewards. At slider value zero the two views converge.

## Performance contract

Only cells intersecting the camera viewport are submitted. `RenderStats` publishes visible and culled cell counts, draw calls and an estimated transient memory footprint. The baseline scene avoids per-frame allocation and keeps geometry batched as simple Citro2D primitives. Later asset-backed renderers must retain culling and provide an Old 3DS benchmark before replacing placeholders.
