# Deterministic economy

`EconomySimulation` is the single model for online and offline resource progress. Time is advanced in bounded analytical steps, so a long absence produces the same state as repeated online updates at the configured step size.

## Resources and capacities

Power, food, water and credits are clamped to `[0, capacity]`. Reducing capacity below the current amount discards the overflow explicitly and records it in the journal. Invalid negative rates and capacities are normalized instead of allowing resources to wrap or become negative.

## Production and collection

Rooms advance only while enabled and staffed. Completed cycles add to a room's pending output. Manual collection transfers only what fits in storage and leaves the rest pending. Collection and credit operations require a non-zero transaction ID; duplicate IDs are rejected, preventing duplicate rewards and costs.

## Shortages

Consumption is applied in stable power, food and water order. At zero power, powered rooms are sorted by descending priority and then stable room ID; the highest-priority room remains active while lower-priority rooms are disabled. Food shortage accumulates hunger and health penalties. Water shortage accumulates thirst and contamination.

## Forecast and diagnostics

The forecast exposes the net hourly trend for all survival resources and the earliest predicted shortage. Every accepted collection, credit transaction, consumption and capacity overflow is appended to a deterministic journal for tests and diagnostics.

The host suite compares repeated hourly updates with a single 30-day offline update and also covers one-day and seven-day shortage behavior.
