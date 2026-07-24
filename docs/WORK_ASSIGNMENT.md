# Work assignment

Work rooms expose capacity, availability and a preferred SPECIAL stat. Assignment previews show the active productivity, projected productivity and delta before confirmation.

Movement is deterministic and ordered by completion time and command sequence. A dweller has the explicit `InTransit` status, cannot receive another move command, reserves a target workstation and contributes no production until arrival. Arrival atomically removes the old assignment and applies the new one. If the room disappears or becomes unavailable, the previous valid state is restored and the cancellation reason is written to history.

Productivity uses effective SPECIAL without modifying base SPECIAL: `preferred SPECIAL * 100 + level * 5`. Outfit changes therefore update previews and active production immediately while preserving the base profile.

Full or unavailable rooms return explicit errors. `suggest_room` chooses the highest-productivity available workstation. Happiness changes require a non-empty reason and append an auditable timestamped entry.

UI integration should offer both drag-and-drop and an accessible button-based assignment flow using the same preview and move APIs.
