# Feature parity and delivery truth

This document records what is actually available to a player. It does not treat
an isolated model, a passing unit test, a debug-only state, or a rendered
placeholder as a completed gameplay feature.

## Status rules

- `missing` — no usable implementation exists.
- `partial` — some code or presentation exists, but the acceptance path is
  incomplete.
- `implemented` — the complete player path is connected to the normal runtime.
- `verified` — the implemented path also has automated integration evidence and,
  where required, Azahar or physical 3DS evidence.
- `N/A` — infrastructure rather than player-facing gameplay.

A feature is never `implemented` merely because its model compiles. A
benchmark-only initial state is not a player flow. Performance-sensitive
features cannot become `verified` using Azahar evidence alone.

## Immediate P0

**[#92 — P0 runtime integration: larger shelter, free-form building and
resident movement](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/92)**
is the next delivery target.

It must replace the current six-slot presentation with a coordinate-driven grid
of at least 8 columns by 4 floors. The player must choose both the room type and
its legal position. Residents must have positions independent of work
assignments, follow bounded deterministic paths, use continuous elevator shafts
between floors, and visibly enter the `InTransit` state before work begins.

The audit **reopened #15, #16, #17, #18, #20 and #21**. Their domain code and
unit tests remain useful, but their original runtime-facing acceptance criteria
were not met. #92 integrates #15, #16, #20 and #21 first; #17 and #18 follow as
the next slice.

## Mechanic matrix

| Fallout Shelter mechanic | Model status | Normal runtime status | Issue / audit action | Delivery order |
|---|---|---|---|---|
| Shelter grid, rock excavation, legal cells, elevators and bounded pathfinding | `partial` — `ShelterGrid` has unit-tested 8×4 operations, but no grid serialization or migration | `missing` — runtime stores only `rooms=1..6`; rendering uses six fixed coordinates | **[#15](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/15) REOPENED**; integrate through **[#92](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/92) P0** | 0 |
| Player chooses what and where to build | `partial` — previews and collision rules exist only at model level | `missing` — build costs 100 credits, increments a counter, and derives room type from slot index | **[#92](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/92) P0** | 0 |
| Data-driven room catalog and unlock reasons | `partial` — validated RomFS JSON and a separate hard-coded C++ catalog both exist | `missing` — neither catalog drives the runtime build menu, costs, types or unlocks | **[#16](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/16) REOPENED**; one catalog must become the source of truth in #92 | 0 |
| Room merging, upgrades and safe demolition | `partial` — grouping and idempotent transactions have unit tests | `missing` — no merge, upgrade or demolition UI/state is connected | **[#17](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/17) REOPENED** | 1 |
| Power, food, water, production, collection, consumption, shortages and capacity | `partial` — `EconomySimulation` covers production, consumption, power priority, forecast and long offline steps | `partial` — a separate mini-model produces 5 units every 120 fixed steps; no consumption, shortage, capacity rooms, forecast or offline settlement | **[#18](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/18) REOPENED** | 1 |
| Persistent dweller identity, SPECIAL, HP, radiation, happiness and XP | `partial` — `DwellerService` and migration/serialization tests exist | `missing` — the visible billboard is not backed by a runtime `Dweller`; SPECIAL, HP and XP do not affect play | **[#20](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/20) REOPENED**; integrate through #92 | 0 |
| Work capacity, SPECIAL efficiency, assignment and happiness | `partial` — `WorkAssignmentService` has unit-tested capacity, previews, transit deadlines and happiness logs | `missing` — one abstract resident is assigned instantly by changing `assigned_room`; no capacity, efficiency or happiness is used | **[#21](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/21) REOPENED**; integrate through #92 | 0 |
| Visible resident navigation, elevators and deterministic idle walking | `partial` — a pathfinder and a transit queue exist separately | `missing` — the current resident teleports between a room and elevator presentation | **[#92](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/92) P0** | 0 |
| Relationships, pregnancy, children and ageing | `missing` | `missing` | [#22](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/22) open | 4 |
| Radio recruitment and candidate queue | `missing` | `missing` | [#23](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/23) open | 4 |
| Dweller levelling and SPECIAL training | `partial` — base XP level calculations exist in `DwellerService` | `missing` — no XP sources, training rooms, deadlines or collection flow | [#24](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/24) open | 4 |
| Inventory ownership, weapons, outfits and medicine | `missing`; catalog arrays are empty | `missing` | [#25](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/25) open | 2 |
| Scrap, recipes and weapon/outfit workshops | `missing`; recipe data is empty | `missing` | [#26](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/26) open | 5 |
| Dweller appearance and room themes | `partial` — generated visual archetypes and furnished room profiles exist | `missing` — there is no customization state or player flow | [#27](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/27) open | 5 |
| Objectives, earned currencies, achievements and reward crates | `missing`; reward data is empty | `missing` | [#28](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/28) open | 5 |
| Internal incidents and room-to-room spread | `missing`; incident data is empty | `missing` | [#29](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/29) open | 3 |
| Shelter attacks, enemies and room combat | `missing`; enemy data is empty | `missing` | [#30](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/30) open after #25 and #29 | 3 |
| Healing, radiation, death, revival and survival permadeath | `partial` — the dweller model clamps HP/radiation and has a dead state | `missing` — no medicine transaction, death consequences, revival or mode rules | [#31](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/31) open after #30 | 3 |
| Companions and floor automation robot | `missing`; companion/robot data is empty | `missing` | [#32](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/32) open after #25 and #30 | 5 |
| Wasteland exploration, events, loot and return | `missing`; event data is empty | `missing` | [#33](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/33) open after #25 and #31 | 6 |
| Quest teams, world map, travel and mission state | `missing`; quest data is empty | `missing` | [#34](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/34) open after #33 | 6 |
| Multi-room quest locations and discovery | `missing`; location data is empty | `missing` | [#35](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/35) open after #34 | 6 |
| Quest combat, critical hits, dialogue and choices | `missing`; dialogue data is empty | `missing` | [#36](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/36) open after #30, #31 and #35 | 6 |
| Quest chains, daily content and procedural generation | `missing` | `missing` | [#37](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/37) open after #28 and #34–#36 | 6 |
| Tutorial and actionable next-step messages | `partial` — the demo exposes short room-action messages | `missing` — no state-driven tutorial, resume/reset, highlighting or complete first-session flow | [#38](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/38) open; add slice-specific guidance early, finish after core mechanics | 7 |
| Polish/English localization and accessible controls | `partial` — catalog contains matching PL/EN keys and basic controls support touch/buttons | `missing` — normal UI strings are hard-coded Polish; no runtime language selection, overflow gate or remapping | [#39](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/39) open | 7 |
| Standard/survival balance and long-run economy simulation | `missing` | `missing` | [#40](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/40) open after playable systems #22–#37 | 7 |
| Trusted time and offline deadlines | `partial` — `TrustedClock` has isolated tests | `missing` — the playable session does not use it and performs no offline progress | [#41](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/41) is a gate for every timer-bearing slice | Gate |
| Versioned atomic save and backup | `partial` — the current demo save has explicit encoding, CRC, temporary file and backup recovery | `partial` — only the small six-room demo state is persisted; domain models, grid and transit are absent | #10 foundation; migration and fault-injection requirements are gated by #41 and #92 | Gate |
| Diagnostics, deterministic RNG and benchmark reporting | `partial` — renderer telemetry and an Azahar performance artifact exist | `partial` — no complete seeded gameplay scenario, bounded runtime log or comparable Old/New 3DS report | [#12](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/12) open | Gate |
| Old/New 3DS performance, memory and soak coverage | `partial` — Azahar provides mono/stereo submission and GPU metrics for the six-room benchmark | `missing` — no full-grid, population, pathfinding, save, soak or physical Old 3DS acceptance result | [#42](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/42) open; enforce per slice | Gate |
| Build, policy checks, 3DSX/CIA and Azahar launch | `N/A` | `N/A` | [#19](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/19) remains closed; infrastructure is verified, not gameplay parity | Continuous |
| Feature completeness audit and evidence | `partial` — this matrix exists | `partial` — most player mechanics are still missing | [#43](https://github.com/grzegorz2047/FalloutDeShelterAIDev/issues/43) stays open and is updated after every slice | Gate |

## Delivery sequence

1. **Order 0 — #92 P0:** integrate #15, #16, #20 and #21 into one
   coordinate-driven build-and-move runtime slice.
2. **Order 1:** connect #17 room lifecycle and replace the mini economy with
   #18.
3. **Order 2:** implement #25 inventory as the ownership foundation for combat,
   medicine, rewards and expeditions.
4. **Order 3:** deliver #29, then #30, then #31 so damage and death have one
   deterministic combat pipeline.
5. **Order 4:** deliver #23 recruitment, #24 progression/training and #22
   family growth on the shared dweller/time model.
6. **Order 5:** deliver #26 and #28, then #27 and #32, all on the shared
   inventory and transaction model.
7. **Order 6:** deliver #33, #34, #35, #36 and #37 in dependency order.
8. **Order 7:** complete #38, #39 and #40. Tutorial, localization checks and
   balance instrumentation should still be added incrementally rather than
   postponed wholesale.

#12, #41, #42 and #43 are cross-cutting gates, not final cleanup tasks.

## QA gates

### Gate A — save schema and migration

#92 cannot be accepted until all of the following pass:

- introduce a new explicit save version for the coordinate grid, stable room
  IDs, dweller identities/positions, assignments and active transit;
- migrate every valid legacy `rooms=1..6` value to a deterministic legal layout
  while preserving credits, resources, stored production and progress;
- reject or safely repair invalid dimensions, duplicate IDs, illegal room
  footprints, broken elevator paths and impossible transit targets;
- prove that old-save migration followed by save/load is stable and does not
  run the migration twice;
- save and reload during resident movement, including a route that changes
  floor through an elevator;
- cover crash/fault points before temporary write, after temporary verification,
  after backup rotation and before final activation, always retaining at least
  one valid copy;
- retain explicit encoding and CRC validation; never persist raw C++ object
  layouts.

Required evidence: host migration fixtures for all six legacy room counts,
corruption/fault-injection tests, and an Azahar relaunch from a migrated save.

### Gate B — Nintendo 3DS performance and resource limits

Before #92 implementation begins, #42 must record numeric acceptance budgets
for frame time/FPS, linear memory, ordinary heap, draw calls, save pause and
pathfinding work. Until those numbers and physical-device results exist, the
feature can be at most `implemented`, never `verified`.

The representative #92 benchmark must include:

- the maximum grid size supported by the slice, a deliberately irregular
  layout, continuous elevators and the declared maximum active residents;
- simultaneous resident movement, idle walking, room animation, HUD, camera
  pan/zoom and full-range stereoscopic rendering;
- mono and stereo measurements on Azahar, Old 3DS and New 3DS without changing
  simulation outcomes;
- bounded pathfinding work and command queues, with a visible failure mode
  rather than an unbounded frame stall;
- no scene-vertex overflow, no dropped residents, no unbounded heap/linear
  memory growth and no resource leak during a soak run;
- save/load during motion and camera activity without corrupting state.

The existing six-room Azahar telemetry is a useful baseline only. It is not
evidence for a larger grid or physical Old 3DS performance.

### Gate C — Azahar player flow

The #92 Azahar test must start from a normal fresh save, without the
benchmark-only furnished initial state, and automatically exercise the same
controls available to a player:

1. open the build catalog;
2. choose two different production-room types;
3. place them in two legal, non-sequential player-selected cells;
4. build a continuous elevator to another floor;
5. attempt an occupied-cell build and prove that neither state nor credits
   change;
6. select a resident and a destination on another floor;
7. capture intermediate frames proving movement through the shelter and
   elevator rather than teleportation;
8. save while `InTransit`, relaunch, load and reach the same legal destination;
9. leave a resident unassigned and prove deterministic idle walking without
   changing the work assignment.

Required artifacts: action/state log, save/load result, performance log, and
readable screenshots or a frame sequence showing the irregular grid, build
preview, invalid placement, intermediate transit and final arrival.

The current Azahar check verifies launch, metrics and a six-room CI-injected
presentation. It does not satisfy this user-flow gate.

## Known integration hazards

- `romfs/data/catalog.json` and `built_in_room_definitions()` are two room
  catalogs. #92 must choose one validated source of truth before connecting the
  build UI.
- `PlayableShelterSession` duplicates parts of `ShelterGrid`,
  `EconomySimulation`, `DwellerService` and `WorkAssignmentService`. Extending
  both paths would create divergent rules and save formats; #92 should compose
  the domain models behind one runtime session.
- `ShelterSceneLayout` and the scene/glow/dweller renderers clamp presentation
  to six fixed room coordinates. A larger grid requires coordinate-driven
  visibility and explicit geometry budgets, not a larger fixed array.
- The billboard renderer has a declared 12-dweller buffer but the current
  gameplay state supplies only one abstract resident. Population and renderer
  limits must be defined together.
- The current playable save and the general `SaveStore` are separate
  persistence paths. #92 must designate one canonical shelter save schema
  before migration work is accepted.
