# Material atlas provenance

The compact runtime material atlas is derived from the reference pack attached to GitHub issue #85.
Only eight 16×16 patterns are generated in the executable; room colors remain vertex tints.
The loose reference PNG files are not loaded or shipped at runtime.

## Selected source motifs

| Runtime material | Source tile from #85 |
|---|---|
| Rock | `01_dark_rock_wall.png` |
| ExcavatedRock | `02_excavated_rock.png` |
| Steel | `04_riveted_steel_panel.png` |
| VaultPanel | `05_vault_interior_panel.png` |
| Grating | `07_metal_floor_grating.png` |
| Water | `09_glass_water_tank_panel.png` |
| Hydroponic | `10_hydroponic_planter.png` |
| ControlPanel | `14_glowing_control_panel.png` |

The final runtime patterns retain the large-scale visual motifs that remain readable at 400×240: irregular rock aggregate, cut-rock bands, steel seams and rivets, recessed vault framing, floor grating, water ripples, planter clusters and a luminous control panel.

## Runtime budget

- Dimensions: 128×16 pixels (power of two)
- Equivalent indexed footprint: 4bpp, 1,024 bytes plus a 32-byte RGB565 palette
- Decoded GPU texture: RGB565, 4,096 bytes
- Filtering: nearest; wrapping: clamp-to-edge
- Draw calls: unchanged; the atlas changes only UV selection
- Runtime allocations: unchanged; decode writes directly into the existing C3D texture

The generator is deterministic and has no dependency on image libraries in the production build.
