# Playable shelter vertical slice

The runtime demo is backed by `PlayableShelterSession`, a host-testable gameplay
model rather than input-specific logic in `main.cpp`.

## Player loop

1. Select a room with `L`/`R`.
2. Assign the resident from the primary room action.
3. Let the fixed 60 Hz simulation complete a two-second production cycle.
4. Collect the selected room's resource.
5. Spend credits to build the next room.

Only one resident exists in this slice, so assigning another room moves that
resident and pauses the previous room without destroying its progress.

## Persistence

The playable state uses an explicit little-endian, versioned payload. The file
header includes its payload length and CRC-32. Saves are written and decoded
from a temporary file before the current file is moved to `.bak` and the new
file is activated. Loading falls back to the last valid backup.

No C++ structure is written directly, so compiler padding and ABI changes do
not silently alter the file format.

## Tests

The host suite covers:

- build cost, insufficient funds and the six-room limit;
- worker movement and room-local progress/storage;
- power, food, water and credit collection;
- identical simulation progress at 30 and 60 rendered frames per second;
- a complete 250 ms catch-up window without dropped fixed steps;
- codec round-trip, CRC rejection and recovery from a corrupt main save.
