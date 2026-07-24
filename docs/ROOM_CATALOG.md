# Room catalog and unlock rules

Room definitions live in `romfs/data/catalog.json`. The catalog is validated before every build and contains stable IDs, localization keys, icon paths, SPECIAL affinity, dimensions, costs, levels, resource effects and unlock requirements.

The 1.0 matrix includes production, storage, residential, training, medical, recruitment, crafting, cosmetic and special rooms. No unlock rule depends on a room name. `RoomCatalog` evaluates population, shelter progress and achievement requirements in the same order for every definition.

Locked entries return both a concrete reason and a next action for the UI. Reaching a threshold unlocks the definition deterministically. A later population or progress decrease can lock future construction but never removes already-built rooms.

Save compatibility uses stable room IDs. A removed or unknown ID resolves to `room.unknown`, preserving the occupied footprint and allowing the game to load safely instead of crashing. Definitions may be added without a save migration; changed dimensions or semantics require an explicit migration; retired IDs must remain in compatibility fixtures.

Presentation assets may fall back to a geometric placeholder when the referenced icon cannot be loaded. Missing localization keys, malformed icon paths, zero costs, invalid dimensions, unknown resources and incomplete category coverage fail validation.
