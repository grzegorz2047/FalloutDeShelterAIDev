#!/usr/bin/env python3
"""Build the compact PICA200 room atlas from the project-owner asset pack.

Passing --source-root rebuilds the inspectable PNG atlas from issue #85 crops.
Without it, the committed PNG is used to regenerate the runtime binary.
"""

from __future__ import annotations

import argparse
import struct
from pathlib import Path

from PIL import Image, ImageOps


ROOT = Path(__file__).resolve().parents[1]
ATLAS_PATH = ROOT / "assets" / "room_assets_atlas.png"
BINARY_PATH = ROOT / "data" / "room_assets.bin"
ATLAS_WIDTH = 512
ATLAS_HEIGHT = 256
MATERIAL_SIZE = 64
PROP_WIDTH = 128
PROP_HEIGHT = 64

MATERIAL_FILES = (
    "01_dark_rock_wall.png",
    "02_excavated_rock.png",
    "03_brushed_steel_panel.png",
    "05_vault_interior_panel.png",
    "07_metal_floor_grating.png",
    "09_glass_water_tank_panel.png",
    "10_hydroponic_planter.png",
    "14_glowing_control_panel.png",
)

PROP_FILES = (
    "props_crops_014.png",  # long control console
    "props_crops_015.png",  # power generator
    "props_crops_016.png",  # water machinery
    "props_crops_017.png",  # storage shelf
    "props_crops_018.png",  # lockers
    "props_crops_019.png",  # hydroponic planter
    "props_crops_020.png",  # terminal
    "props_crops_021.png",  # bunk beds
    "props_crops_023.png",  # work/dining table
    "props_crops_025.png",  # storage crate
    "props_crops_026.png",  # sofa
    "props_crops_036.png",  # ceiling light
)


def require_image(path: Path) -> Image.Image:
    if not path.is_file():
        raise FileNotFoundError(f"missing issue #85 crop: {path}")
    return Image.open(path).convert("RGBA")


def crop_visible(image: Image.Image) -> Image.Image:
    alpha = image.getchannel("A")
    bounds = alpha.getbbox()
    return image.crop(bounds) if bounds is not None else image


def contain_visible(image: Image.Image, width: int, height: int) -> Image.Image:
    image = crop_visible(image)
    contained = ImageOps.contain(
        image,
        (width - 6, height - 6),
        method=Image.Resampling.LANCZOS,
    )
    slot = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    slot.alpha_composite(
        contained,
        ((width - contained.width) // 2, height - contained.height - 2),
    )
    return slot


def build_source_atlas(source_root: Path) -> Image.Image:
    texture_root = source_root / "texture_atlas_tiles"
    prop_root = source_root / "props_crops"
    atlas = Image.new("RGBA", (ATLAS_WIDTH, ATLAS_HEIGHT), (0, 0, 0, 0))

    for index, name in enumerate(MATERIAL_FILES):
        material = ImageOps.fit(
            require_image(texture_root / name),
            (MATERIAL_SIZE, MATERIAL_SIZE),
            method=Image.Resampling.LANCZOS,
        )
        atlas.alpha_composite(material, (index * MATERIAL_SIZE, 0))

    for index, name in enumerate(PROP_FILES):
        prop = contain_visible(
            require_image(prop_root / name),
            PROP_WIDTH,
            PROP_HEIGHT,
        )
        x = (index % 4) * PROP_WIDTH
        y = MATERIAL_SIZE + (index // 4) * PROP_HEIGHT
        atlas.alpha_composite(prop, (x, y))

    return atlas


def pica_tile_offset(x: int, y: int) -> int:
    tile_x = x // 8
    tile_y = y // 8
    local_x = x & 7
    local_y = y & 7
    tile_index = tile_y * (ATLAS_WIDTH // 8) + tile_x
    morton = (
        (local_x & 1)
        | ((local_y & 1) << 1)
        | ((local_x & 2) << 1)
        | ((local_y & 2) << 2)
        | ((local_x & 4) << 2)
        | ((local_y & 4) << 3)
    )
    return tile_index * 64 + morton


def rgba5551(pixel: tuple[int, int, int, int]) -> int:
    r, g, b, a = pixel
    return (
        ((r >> 3) << 11)
        | ((g >> 3) << 6)
        | ((b >> 3) << 1)
        | (1 if a >= 128 else 0)
    )


def encode_tiled(image: Image.Image) -> bytes:
    if image.size != (ATLAS_WIDTH, ATLAS_HEIGHT):
        raise ValueError(
            f"atlas must be {ATLAS_WIDTH}x{ATLAS_HEIGHT}, got {image.size}"
        )
    image = image.convert("RGBA")
    pixels = image.load()
    tiled = [0] * (ATLAS_WIDTH * ATLAS_HEIGHT)
    for y in range(ATLAS_HEIGHT):
        for x in range(ATLAS_WIDTH):
            tiled[pica_tile_offset(x, y)] = rgba5551(
                pixels[x, y]
            )
    return struct.pack(f"<{len(tiled)}H", *tiled)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--source-root",
        type=Path,
        help="directory containing texture_atlas_tiles/ and props_crops/",
    )
    args = parser.parse_args()

    if args.source_root is not None:
        atlas = build_source_atlas(args.source_root)
        ATLAS_PATH.parent.mkdir(parents=True, exist_ok=True)
        atlas.save(ATLAS_PATH, optimize=True)
    else:
        if not ATLAS_PATH.is_file():
            raise FileNotFoundError(
                f"{ATLAS_PATH} is missing; pass --source-root for the first build"
            )
        atlas = Image.open(ATLAS_PATH).convert("RGBA")

    BINARY_PATH.parent.mkdir(parents=True, exist_ok=True)
    BINARY_PATH.write_bytes(encode_tiled(atlas))
    print(
        f"room-atlas: {ATLAS_PATH.relative_to(ROOT)} -> "
        f"{BINARY_PATH.relative_to(ROOT)} "
        f"({BINARY_PATH.stat().st_size} bytes)"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
