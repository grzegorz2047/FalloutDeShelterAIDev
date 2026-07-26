# Generated dweller atlas

Deep Shelter 3D uses an original, deterministic 256×256 RGBA5551 resident
atlas generated from C++ drawing rules at startup.

## Layout

- five job archetypes: technician, gardener, water operator, mechanic and
  civilian;
- three animation states: idle, work and walk;
- four frames per animation;
- 60 frames in total, packed in a 10×6 grid;
- each frame is 24×32 pixels;
- unused atlas pixels are transparent.

The public lookup functions clamp invalid archetypes to the civilian and wrap
frame indices. Room types select a matching job silhouette and palette.

## Renderer contract

The atlas is decoded directly into one 128 KiB PICA200 texture in Morton-tiled
order. A dedicated unlit Citro3D pass batches camera-facing quads, uses nearest
filtering and rejects transparent pixels with the GPU alpha test. Visible
pixels write depth so later glow cannot bleed through the character. Rooms
render first, dwellers second and the glow pass last.

Animation advances on the same fixed 60 Hz simulation clock as production.
The left and right stereo views receive an identical animation tick and frame
phase.

## Verification

The host test:

- checks all 65,536 row-major pixels against their tiled positions;
- validates all 60 frame regions;
- verifies that every archetype has visible and distinct artwork;
- checks the idle, work and walk frame cadences;
- locks the runtime texture budget to 128 KiB.

## Asset policy

The characters are original procedural pixel art created for Deep Shelter 3D.
The reference images attached to GitHub issues are used only as a mood and
layout board. They are not copied, bundled or decoded by the game.
