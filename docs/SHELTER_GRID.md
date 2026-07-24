# Shelter grid and construction transactions

The shelter uses a bounded rectangular grid. Cells are rock, excavated empty space, rooms, elevators or permanently blocked terrain. Every mutation is previewed before confirmation.

## Transactions

- Preview validates bounds, occupancy, adjacency, width and available credits without changing state.
- Confirmation repeats validation, mutates cells and charges exactly once.
- Repeated confirmation fails because the target is no longer in its original state.
- Cancel is represented by discarding the preview; no rollback is needed because preview is side-effect free.

## Connectivity

Horizontal movement is possible through excavated, room and elevator cells. Vertical movement requires an elevator at both cells. Demolishing a room or elevator is blocked when it would disconnect traversable shelter space unless the caller explicitly accepts the unsafe operation.

## Pathfinding

Breadth-first search uses a fixed neighbour order for deterministic results. A caller-provided work limit prevents malformed or maximal layouts from monopolizing a frame. Failure and work-limit exhaustion are reported separately.

## Save compatibility

Persist dimensions and cell values, never vector indexes from another schema. On load, validate dimensions, cell count and enum ranges before exposing the layout. A future map-size migration must either place every retained cell deterministically or reject the slot without modifying its files.
