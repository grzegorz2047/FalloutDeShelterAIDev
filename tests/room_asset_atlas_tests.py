#!/usr/bin/env python3
"""Structural checks for the issue #85 room atlas and PICA200 binary."""

from __future__ import annotations

import struct
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
ATLAS = ROOT / "assets" / "room_assets_atlas.png"
BINARY = ROOT / "data" / "room_assets.bin"
WIDTH = 512
HEIGHT = 256


def pica_tile_offset(x: int, y: int) -> int:
    tile_x, tile_y = x // 8, y // 8
    local_x, local_y = x & 7, y & 7
    morton = (
        (local_x & 1)
        | ((local_y & 1) << 1)
        | ((local_x & 2) << 1)
        | ((local_y & 2) << 2)
        | ((local_x & 4) << 2)
        | ((local_y & 4) << 3)
    )
    return (tile_y * (WIDTH // 8) + tile_x) * 64 + morton


def rgba5551(pixel: tuple[int, int, int, int]) -> int:
    r, g, b, a = pixel
    return (
        ((r >> 3) << 11)
        | ((g >> 3) << 6)
        | ((b >> 3) << 1)
        | (1 if a >= 128 else 0)
    )


def main() -> int:
    image = Image.open(ATLAS).convert("RGBA")
    assert image.size == (WIDTH, HEIGHT)
    pixels = image.load()
    binary = BINARY.read_bytes()
    assert len(binary) == WIDTH * HEIGHT * 2
    tiled = struct.unpack(f"<{WIDTH * HEIGHT}H", binary)

    for x, y in ((0, 0), (63, 63), (320, 12), (127, 127), (511, 255)):
        assert tiled[pica_tile_offset(x, y)] == rgba5551(
            pixels[x, y]
        )

    alpha = image.getchannel("A")
    for material in range(8):
        box = (material * 64, 0, (material + 1) * 64, 64)
        assert alpha.crop(box).getextrema() == (255, 255)
    for prop in range(12):
        x = (prop % 4) * 128
        y = 64 + (prop // 4) * 64
        extrema = alpha.crop((x, y, x + 128, y + 64)).getextrema()
        assert extrema[0] == 0 and extrema[1] == 255

    print("room-asset-atlas-tests: source regions and tiled RGBA5551 passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
