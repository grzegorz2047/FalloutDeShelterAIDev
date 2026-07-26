# Room atlas provenance

The production room atlas uses the texture and prop pack uploaded by the
repository owner in GitHub issue #85. The owner explicitly requested that these
assets be integrated into the game. The exact repository paths, source issue
and permission reference are recorded in `assets/manifest.csv`.

## Selected source material

The 512×256 source atlas contains eight 64×64 materials:

| Runtime material | Source crop from issue #85 |
|---|---|
| Rock | `01_dark_rock_wall.png` |
| ExcavatedRock | `02_excavated_rock.png` |
| Steel | `03_brushed_steel_panel.png` |
| VaultPanel | `05_vault_interior_panel.png` |
| Grating | `07_metal_floor_grating.png` |
| Water | `09_glass_water_tank_panel.png` |
| Hydroponic | `10_hydroponic_planter.png` |
| ControlPanel | `14_glowing_control_panel.png` |

The remaining three rows contain twelve transparent props selected from
`props_crops_014` through `props_crops_036`: control console, generator, water
machinery, shelf, lockers, planter, terminal, bunks, table, crate, sofa and
ceiling light.

## Reproducible conversion

`scripts/generate_room_asset_atlas.py` can rebuild the PNG from the unpacked
issue attachment with `--source-root`. A normal invocation converts the
committed PNG into `data/room_assets.bin`:

- dimensions: 512×256;
- format: RGBA5551;
- layout: PICA200 8×8 Morton tiles;
- runtime GPU footprint: 262,144 bytes;
- filtering: bilinear with half-texel UV insets;
- wrapping: clamp-to-edge.

`tests/room_asset_atlas_tests.py` validates every region, alpha coverage,
selected pixel encodings, PICA tiling and binary size.
