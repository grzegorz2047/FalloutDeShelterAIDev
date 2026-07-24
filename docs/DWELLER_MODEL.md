# Dweller model

Each dweller has a stable non-zero ID, identity and presentation metadata, base SPECIAL, outfit bonuses, level and XP, health and radiation, happiness, activity status, assignments, equipment, family links and an append-only event history.

Base SPECIAL is stored separately from temporary outfit bonuses. Both are clamped to explicit ranges and the effective value is calculated without mutating the base profile. Health uses `effective_max_hp = max(1, max_hp - radiation)`, while current HP is always clamped to that value. Zero HP deterministically sets the dweller to `Dead`.

XP thresholds are data-driven through `XpTable`. One XP transaction may cross several thresholds. Every awarded level is recorded and transaction IDs are idempotent, so a repeated reward cannot grant HP or a level twice.

Unknown weapon, outfit and companion IDs are replaced with stable fallback IDs during load. A missing outfit also clears its derived bonus while preserving base SPECIAL. Duplicate or zero dweller IDs fail insertion; migrations must allocate new IDs before the model becomes visible to gameplay.

The in-memory structure is schema version 1. Persistence adapters must serialize every field losslessly, including family links, history and awarded level markers. Future versions must migrate through explicit version steps before normalization.
