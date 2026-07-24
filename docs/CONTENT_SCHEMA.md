# Versioned content schema

Game content lives in `romfs/data/catalog.json` and is loaded as read-only data. Simulation code must refer to stable string IDs rather than array indexes or localized names.

## Version contract

- `schema_version` describes the file structure and requires an explicit parser migration when changed.
- `content_version` describes the shipped content revision and may change without save migration when stable IDs remain compatible.
- Unknown schema versions must be rejected with a readable error instead of being partially loaded.
- Removing or renaming an ID requires a save migration or a documented fallback.

## Required sections

The catalog always contains arrays for rooms, resources, weapons, outfits, companions, robots, recipes, incidents, enemies, events, quests, dialogues, rewards and locations. Empty arrays are valid while a system is not implemented.

Translations are stored under `translations.en` and `translations.pl`. Both languages must expose identical key sets. Gameplay data references translation keys, never user-visible text.

## Identifier rules

IDs use lowercase namespaces such as `room.power_generator` and must match:

```text
^[a-z][a-z0-9_]*(\.[a-z0-9_]+)+$
```

IDs are unique case-insensitively across the whole catalog. This prevents collisions on filesystems and tooling with different case behavior.

## Validation

Run:

```sh
python3 scripts/validate_content.py
python3 tests/content_validator_tests.py
```

The validator reports the section, record index and field. CI rejects malformed JSON, duplicate IDs, missing translations, invalid numeric ranges, unknown resource references and direct recipe cycles.

## Compatibility policy

Additive fields must have safe defaults. A future schema version may introduce stricter validation, but an older executable must never overwrite content or saves it cannot understand. Active quests and long-running processes will eventually store stable IDs plus the minimum snapshot required to survive content updates.
