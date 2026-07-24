# Work assignment and happiness

Assignments are processed as deterministic transit orders sorted by completion time and command sequence. A dweller can have at most one pending move, and the current room remains unchanged until arrival. At completion, the service validates the destination again and atomically removes the dweller from the old room before inserting them into the new one.

Rooms expose capacity, availability and a preferred SPECIAL index. Individual efficiency is calculated from effective SPECIAL and level, so an outfit change is reflected immediately without modifying base SPECIAL. Group efficiency is the sum of current occupants. `preview` returns the current value, target value, difference and a concrete failure reason for unavailable, full or missing rooms.

`suggest_room` chooses the available room with the highest resulting efficiency and uses room ID order as the deterministic tie-breaker. Removing a room or cancelling movement clears stale assignments and records the reason in dweller history.

Happiness changes require a non-empty reason and append an auditable log entry containing timestamp, dweller ID, applied delta and reason. Values remain clamped to 0-100.

The UI can use the same preview contract for drag and drop or button-based assignment. It should show the efficiency difference and translate `AssignmentError` to a concrete action before the player confirms the move.
