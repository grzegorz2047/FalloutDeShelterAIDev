#!/usr/bin/env python3
"""Validate asset provenance and reject obviously prohibited material."""

from __future__ import annotations

import csv
import hashlib
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "assets" / "manifest.csv"
ASSET_ROOTS = (ROOT / "assets", ROOT / "data", ROOT / "romfs")
IGNORED_NAMES = {"manifest.csv", "ATTRIBUTION.md", ".gitkeep"}
DENIED_TERMS = {
    "fallout",
    "vault-tec",
    "vaulttec",
    "pip-boy",
    "pipboy",
    "bethesda",
}
DENIED_EXTENSIONS = {".cia", ".3ds", ".3dsx", ".nds", ".rom", ".wad", ".pak"}
DENIED_SHA256: set[str] = set()
REQUIRED_COLUMNS = {
    "path",
    "category",
    "author_or_generator",
    "source",
    "license",
    "date",
    "notes",
}


def fail(message: str) -> None:
    print(f"asset-policy: {message}", file=sys.stderr)


def tracked_assets() -> set[str]:
    result: set[str] = set()
    for root in ASSET_ROOTS:
        if not root.exists():
            continue
        for path in root.rglob("*"):
            if not path.is_file() or path.name in IGNORED_NAMES:
                continue
            result.add(path.relative_to(ROOT).as_posix())
    return result


def load_manifest() -> dict[str, dict[str, str]]:
    if not MANIFEST.is_file():
        raise ValueError("missing assets/manifest.csv")
    with MANIFEST.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        columns = set(reader.fieldnames or [])
        missing = REQUIRED_COLUMNS - columns
        if missing:
            raise ValueError(f"manifest missing columns: {sorted(missing)}")
        entries: dict[str, dict[str, str]] = {}
        for row_number, row in enumerate(reader, start=2):
            normalized = {key: (value or "").strip() for key, value in row.items()}
            path = normalized["path"]
            if not path:
                raise ValueError(f"manifest row {row_number} has empty path")
            if path in entries:
                raise ValueError(f"duplicate manifest path: {path}")
            for key in REQUIRED_COLUMNS:
                if not normalized[key]:
                    raise ValueError(f"manifest row {row_number} has empty {key}")
            entries[path] = normalized
        return entries


def validate_paths(paths: set[str]) -> list[str]:
    errors: list[str] = []
    for relative in sorted(paths):
        lowered = relative.lower()
        for term in DENIED_TERMS:
            if term in lowered:
                errors.append(f"prohibited term {term!r} in {relative}")
        path = ROOT / relative
        if path.suffix.lower() in DENIED_EXTENSIONS:
            errors.append(f"binary game/package file is not allowed: {relative}")
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        if digest in DENIED_SHA256:
            errors.append(f"prohibited file hash: {relative}")
    return errors


def main() -> int:
    try:
        manifest = load_manifest()
    except ValueError as exc:
        fail(str(exc))
        return 1

    assets = tracked_assets()
    errors = validate_paths(assets)
    missing_entries = sorted(assets - set(manifest))
    stale_entries = sorted(set(manifest) - assets - {"assets/manifest.csv"})

    errors.extend(f"missing manifest entry: {path}" for path in missing_entries)
    errors.extend(f"manifest references missing file: {path}" for path in stale_entries)

    if errors:
        for error in errors:
            fail(error)
        return 1

    print(f"asset-policy: validated {len(assets)} asset files")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
