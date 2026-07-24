#!/usr/bin/env python3
from __future__ import annotations

import copy
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from validate_content import load_catalog, validate_catalog  # noqa: E402


def expect_error(data: dict, fragment: str) -> None:
    errors = validate_catalog(data)
    assert any(fragment in error for error in errors), (fragment, errors)


def main() -> None:
    valid = load_catalog()
    assert validate_catalog(valid) == []

    duplicate = copy.deepcopy(valid)
    duplicate["rooms"].append(copy.deepcopy(duplicate["rooms"][0]))
    duplicate["rooms"][-1]["id"] = duplicate["rooms"][0]["id"].upper()
    expect_error(duplicate, "duplicate id")

    bad_reference = copy.deepcopy(valid)
    bad_reference["rooms"][0]["produces"] = "resource.missing"
    expect_error(bad_reference, "unknown resource")

    bad_translation = copy.deepcopy(valid)
    del bad_translation["translations"]["pl"]["resource.power.name"]
    expect_error(bad_translation, "translations.pl missing key")

    bad_number = copy.deepcopy(valid)
    bad_number["rooms"][0]["base_cost"] = -1
    expect_error(bad_number, "base_cost")

    cycle = copy.deepcopy(valid)
    cycle["recipes"] = [{"id": "recipe.loop", "inputs": [{"id": "item.loop", "count": 1}], "output": "item.loop"}]
    expect_error(cycle, "consumes its own output")

    encoded = json.dumps(valid, ensure_ascii=False, sort_keys=True)
    decoded = json.loads(encoded)
    assert validate_catalog(decoded) == []
    print("content-validator-tests: all tests passed")


if __name__ == "__main__":
    main()
