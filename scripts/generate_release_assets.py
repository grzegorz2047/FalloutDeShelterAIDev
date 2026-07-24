#!/usr/bin/env python3
"""Generate deterministic, original PNG and WAV assets for CIA packaging."""

from __future__ import annotations

import binascii
import pathlib
import struct
import wave
import zlib

ROOT = pathlib.Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "build" / "release-assets"


def png_chunk(kind: bytes, payload: bytes) -> bytes:
    checksum = binascii.crc32(kind + payload) & 0xFFFFFFFF
    return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", checksum)


def write_png(path: pathlib.Path, width: int, height: int, pixel) -> None:
    rows = bytearray()
    for y in range(height):
        rows.append(0)
        for x in range(width):
            rows.extend(pixel(x, y, width, height))
    data = b"\x89PNG\r\n\x1a\n"
    data += png_chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
    data += png_chunk(b"IDAT", zlib.compress(bytes(rows), 9))
    data += png_chunk(b"IEND", b"")
    path.write_bytes(data)


def shelter_pixel(x: int, y: int, width: int, height: int) -> bytes:
    nx = x / max(width - 1, 1)
    ny = y / max(height - 1, 1)
    r = int(10 + 18 * ny)
    g = int(24 + 34 * ny)
    b = int(34 + 38 * nx)

    border = min(x, y, width - 1 - x, height - 1 - y)
    room_left = width * 0.18
    room_right = width * 0.82
    room_top = height * 0.28
    room_bottom = height * 0.76
    in_room = room_left <= x <= room_right and room_top <= y <= room_bottom
    divider = abs(y - height * 0.52) <= max(1, height // 64)
    doorway = abs(x - width * 0.5) <= width * 0.08 and height * 0.58 <= y <= room_bottom

    if in_room:
        r, g, b = 36, 92, 96
    if divider and in_room:
        r, g, b = 224, 168, 64
    if doorway:
        r, g, b = 12, 28, 34
    if border < max(1, width // 48):
        r, g, b = 228, 236, 218
    return bytes((r, g, b, 255))


def write_silence(path: pathlib.Path) -> None:
    sample_rate = 8000
    frame_count = sample_rate // 4
    with wave.open(str(path), "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(sample_rate)
        output.writeframes(b"\x00\x00" * frame_count)


def main() -> None:
    OUTPUT.mkdir(parents=True, exist_ok=True)
    write_png(OUTPUT / "icon.png", 48, 48, shelter_pixel)
    write_png(OUTPUT / "banner.png", 256, 128, shelter_pixel)
    write_silence(OUTPUT / "banner.wav")


if __name__ == "__main__":
    main()
