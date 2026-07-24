# Dweller model

Each dweller has a stable non-zero ID, identity and presentation metadata, base SPECIAL, outfit bonuses, level and XP, health and radiation, happiness, activity status, assignments, equipment, family links and an append-only event history.

Base SPECIAL is stored separately from temporary outfit bonuses. Base values are clamped to 1-10, while outfit bonuses are clamped to 0-10. Effective values are calculated without mutating the base profile. Health uses `effective_max_hp = max(1, max_hp - radiation)`, while current HP is always clamped to that value. Zero HP deterministically sets the dweller to `Dead`.

XP thresholds are data-driven through `XpTable`. One XP transaction may cross several thresholds. Every awarded level is recorded and transaction IDs are idempotent, so a repeated reward cannot grant HP or a level twice.

Unknown weapon, outfit and companion IDs are replaced with stable fallback IDs during load. A missing outfit clears only the derived bonus and preserves base SPECIAL. Duplicate or zero IDs can be reassigned deterministically by `add_with_unique_id`, which selects the next free monotonic ID.

Schema version 1 uses an escaped text representation. The round-trip preserves identity, SPECIAL, equipment, assignments, family links, event history and awarded-level markers. Unsupported schema versions and malformed payloads are rejected before normalization.
