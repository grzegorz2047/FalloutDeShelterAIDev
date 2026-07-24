#!/usr/bin/env python3
"""Validate versioned Deep Shelter 3D content stored in RomFS."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "romfs" / "data" / "catalog.json"
ID_RE = re.compile(r"^[a-z][a-z0-9_]*(\.[a-z0-9_]+)+$")
SPECIAL = set("SPECIAL")
ROOM_CATEGORIES = {"production", "storage", "residential", "training", "medical", "recruitment", "crafting", "cosmetic", "special"}
ARRAY_SECTIONS = ("rooms", "resources", "weapons", "outfits", "companions", "robots", "recipes", "incidents", "enemies", "events", "quests", "dialogues", "rewards", "locations")


class ValidationError(Exception):
    pass


def require(condition: bool, message: str, errors: list[str]) -> None:
    if not condition:
        errors.append(message)


def is_integer(value: Any) -> bool:
    return isinstance(value, int) and not isinstance(value, bool)


def load_catalog(path: Path = CATALOG) -> dict[str, Any]:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as exc:
        raise ValidationError(f"missing catalog: {path}") from exc
    except json.JSONDecodeError as exc:
        raise ValidationError(f"{path}:{exc.lineno}:{exc.colno}: {exc.msg}") from exc
    if not isinstance(value, dict):
        raise ValidationError("catalog root must be an object")
    return value


def validate_catalog(data: dict[str, Any]) -> list[str]:
    errors: list[str] = []
    require(data.get("schema_version") == 1 and not isinstance(data.get("schema_version"), bool), "schema_version must equal integer 1", errors)
    require(isinstance(data.get("content_version"), str) and bool(data.get("content_version")), "content_version must be a non-empty string", errors)
    for section in ARRAY_SECTIONS:
        require(isinstance(data.get(section), list), f"{section} must be an array", errors)

    translations = data.get("translations")
    require(isinstance(translations, dict), "translations must be an object", errors)
    if not isinstance(translations, dict):
        translations = {}
    for language in ("en", "pl"):
        require(isinstance(translations.get(language), dict), f"translations.{language} must be an object", errors)

    ids: dict[str, str] = {}
    translation_keys: set[str] = set()
    for language, entries in translations.items():
        if isinstance(entries, dict):
            for key, value in entries.items():
                require(isinstance(key, str) and bool(key), f"translations.{language} contains an invalid key", errors)
                require(isinstance(value, str) and bool(value.strip()), f"translations.{language}.{key} must be non-empty text", errors)
                translation_keys.add(key)

    en_keys = set(translations.get("en", {})) if isinstance(translations.get("en"), dict) else set()
    pl_keys = set(translations.get("pl", {})) if isinstance(translations.get("pl"), dict) else set()
    for missing in sorted(en_keys - pl_keys):
        errors.append(f"translations.pl missing key: {missing}")
    for missing in sorted(pl_keys - en_keys):
        errors.append(f"translations.en missing key: {missing}")

    for section in ARRAY_SECTIONS:
        entries = data.get(section, [])
        if not isinstance(entries, list):
            continue
        for index, entry in enumerate(entries):
            prefix = f"{section}[{index}]"
            require(isinstance(entry, dict), f"{prefix} must be an object", errors)
            if not isinstance(entry, dict):
                continue
            identifier = entry.get("id")
            require(isinstance(identifier, str) and bool(ID_RE.fullmatch(identifier or "")), f"{prefix}.id must match {ID_RE.pattern}", errors)
            if isinstance(identifier, str):
                previous = ids.get(identifier.lower())
                if previous is not None:
                    errors.append(f"duplicate id (case-insensitive): {identifier}; first seen in {previous}")
                else:
                    ids[identifier.lower()] = prefix
            for key in ("name_key", "description_key"):
                if key in entry and entry[key] is not None:
                    require(entry[key] in translation_keys, f"{prefix}.{key} references missing translation: {entry[key]}", errors)

    resource_ids = {entry.get("id") for entry in data.get("resources", []) if isinstance(entry, dict)}
    for index, resource in enumerate(data.get("resources", [])):
        if not isinstance(resource, dict):
            continue
        minimum, maximum = resource.get("min"), resource.get("max")
        require(is_integer(minimum) and is_integer(maximum), f"resources[{index}] min/max must be integers", errors)
        if is_integer(minimum) and is_integer(maximum):
            require(0 <= minimum <= maximum, f"resources[{index}] must satisfy 0 <= min <= max", errors)

    rooms = data.get("rooms", [])
    require(isinstance(rooms, list) and len(rooms) >= 20, "rooms must contain at least 20 definitions", errors)
    categories_seen: set[str] = set()
    for index, room in enumerate(rooms if isinstance(rooms, list) else []):
        if not isinstance(room, dict):
            continue
        prefix = f"rooms[{index}]"
        category = room.get("category")
        require(category in ROOM_CATEGORIES, f"{prefix}.category must be a supported category", errors)
        if category in ROOM_CATEGORIES:
            categories_seen.add(category)
        require(room.get("special") in SPECIAL, f"{prefix}.special must be one of SPECIAL", errors)
        require(isinstance(room.get("icon"), str) and room.get("icon", "").startswith("romfs:/"), f"{prefix}.icon must be a RomFS path", errors)
        for key in ("base_cost", "width", "max_level", "unlocks_at_population", "unlocks_at_progress", "storage_bonus"):
            value = room.get(key)
            require(is_integer(value) and value >= 0, f"{prefix}.{key} must be a non-negative integer", errors)
        require(is_integer(room.get("base_cost")) and room.get("base_cost", 0) > 0, f"{prefix}.base_cost must be positive", errors)
        require(is_integer(room.get("width")) and room.get("width") in (1, 2, 3), f"{prefix}.width must be 1, 2 or 3", errors)
        require(is_integer(room.get("max_level")) and room.get("max_level", 0) >= 1, f"{prefix}.max_level must be at least 1", errors)
        achievement = room.get("requires_achievement")
        require(achievement is None or (isinstance(achievement, str) and bool(ID_RE.fullmatch(achievement))), f"{prefix}.requires_achievement must be null or an id", errors)
        produced = room.get("produces")
        require(produced is None or produced in resource_ids, f"{prefix}.produces references unknown resource: {produced}", errors)
    require(categories_seen == ROOM_CATEGORIES, f"rooms must cover full category matrix; got {sorted(categories_seen)}", errors)

    for index, recipe in enumerate(data.get("recipes", [])):
        if not isinstance(recipe, dict):
            continue
        inputs, output = recipe.get("inputs", []), recipe.get("output")
        require(isinstance(inputs, list), f"recipes[{index}].inputs must be an array", errors)
        require(isinstance(output, str), f"recipes[{index}].output must be an id", errors)
        if isinstance(output, str) and isinstance(inputs, list):
            input_ids = {item.get("id") for item in inputs if isinstance(item, dict)}
            require(output not in input_ids, f"recipes[{index}] directly consumes its own output", errors)
    return errors


def main() -> int:
    try:
        data = load_catalog()
    except ValidationError as exc:
        print(f"content-validator: {exc}", file=sys.stderr)
        return 1
    errors = validate_catalog(data)
    if errors:
        for error in errors:
            print(f"content-validator: {error}", file=sys.stderr)
        return 1
    total = sum(len(data[section]) for section in ARRAY_SECTIONS)
    print(f"content-validator: schema v{data['schema_version']}; validated {total} records")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
