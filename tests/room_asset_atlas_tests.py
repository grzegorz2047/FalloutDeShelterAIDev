#!/usr/bin/env python3
"""Structural checks for the issue #85 room atlas and PICA200 binary."""

from __future__ import annotations

import struct
import zlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ATLAS = ROOT / "assets" / "room_assets_atlas.png"
BINARY = ROOT / "data" / "room_assets.bin"
WIDTH = 512
HEIGHT = 256


def paeth(left: int, above: int, upper_left: int) -> int:
    estimate = left + above - upper_left
    left_distance = abs(estimate - left)
    above_distance = abs(estimate - above)
    upper_left_distance = abs(estimate - upper_left)
    if left_distance <= above_distance and left_distance <= upper_left_distance:
        return left
    if above_distance <= upper_left_distance:
        return above
    return upper_left


def decode_rgba_png(path: Path) -> tuple[int, int, bytes]:
    encoded = path.read_bytes()
    assert encoded[:8] == b"\x89PNG\r\n\x1a\n"
    offset = 8
    compressed = bytearray()
    width = height = 0
    while offset < len(encoded):
        length = struct.unpack_from(">I", encoded, offset)[0]
        kind = encoded[offset + 4 : offset + 8]
        payload = encoded[offset + 8 : offset + 8 + length]
        offset += 12 + length
        if kind == b"IHDR":
            width, height, depth, color_type, compression, filtering, interlace = (
                struct.unpack(">IIBBBBB", payload)
            )
            assert depth == 8 and color_type == 6
            assert compression == 0 and filtering == 0 and interlace == 0
        elif kind == b"IDAT":
            compressed.extend(payload)
        elif kind == b"IEND":
            break

    stride = width * 4
    raw = zlib.decompress(bytes(compressed))
    assert len(raw) == height * (stride + 1)
    decoded = bytearray(width * height * 4)
    source = 0
    for y in range(height):
        filter_type = raw[source]
        source += 1
        for x in range(stride):
            value = raw[source]
            source += 1
            left = decoded[y * stride + x - 4] if x >= 4 else 0
            above = decoded[(y - 1) * stride + x] if y > 0 else 0
            upper_left = (
                decoded[(y - 1) * stride + x - 4]
                if y > 0 and x >= 4
                else 0
            )
            if filter_type == 1:
                value = (value + left) & 0xFF
            elif filter_type == 2:
                value = (value + above) & 0xFF
            elif filter_type == 3:
                value = (value + ((left + above) // 2)) & 0xFF
            elif filter_type == 4:
                value = (value + paeth(left, above, upper_left)) & 0xFF
            else:
                assert filter_type == 0
            decoded[y * stride + x] = value
    return width, height, bytes(decoded)


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
    width, height, pixels = decode_rgba_png(ATLAS)
    assert (width, height) == (WIDTH, HEIGHT)
    binary = BINARY.read_bytes()
    assert len(binary) == WIDTH * HEIGHT * 2
    tiled = struct.unpack(f"<{WIDTH * HEIGHT}H", binary)

    for x, y in ((0, 0), (63, 63), (320, 12), (127, 127), (511, 255)):
        pixel_offset = (y * WIDTH + x) * 4
        assert tiled[pica_tile_offset(x, y)] == rgba5551(
            tuple(pixels[pixel_offset : pixel_offset + 4])
        )

    def alpha_extrema(left: int, top: int, right: int, bottom: int) -> tuple[int, int]:
        values = [
            pixels[(y * WIDTH + x) * 4 + 3]
            for y in range(top, bottom)
            for x in range(left, right)
        ]
        return min(values), max(values)

    for material in range(8):
        assert alpha_extrema(
            material * 64, 0, (material + 1) * 64, 64
        ) == (255, 255)
    for prop in range(12):
        x = (prop % 4) * 128
        y = 64 + (prop // 4) * 64
        extrema = alpha_extrema(x, y, x + 128, y + 64)
        assert extrema[0] == 0 and extrema[1] == 255

    print("room-asset-atlas-tests: source regions and tiled RGBA5551 passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
