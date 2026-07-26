# Procedural room visuals (superseded)

This document records the first playable visual pass. The production Citro3D
renderer now uses the issue #85 room atlas documented in
`ASSET_PROVENANCE.md`; procedural materials remain only as a host-test fallback.

## Provenance

- Author: Deep Shelter 3D contributors.
- Source: original project code.
- License: same license as the repository source code.
- Third-party textures, commercial-game screenshots, traced art and extracted assets: none.
- Attribution requirement: none.

## Visual direction

The renderer presents an underground shelter as a side-on cutaway, a general genre convention. Deep Shelter 3D uses its own muted industrial palette, room silhouettes, furniture, residents and UI. It must not reproduce protected logos, layouts, icons or distinctive visual assets from commercial games.

## Implemented themes

1. power machinery;
2. hydroponic planters;
3. water treatment tanks;
4. workshop benches;
5. storage crates;
6. living furniture.

The historical pass used deterministic procedural patterns. The current pass
uses real texture tiles and transparent props at explicit world-space depths.

## Performance budget

The playable demo supports at most six rooms. At the widest visible scene, procedural cells and room furniture remain below `C2D_DEFAULT_MAX_OBJECTS`. Every primitive increments `RenderStats::draw_calls`; camera culling prevents invisible cells from submitting primitives.

The replacement is complete. The PNG source atlas and tiled runtime binary are
listed in `assets/manifest.csv` and checked by `scripts/check_asset_policy.py`.
