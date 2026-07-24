# Trusted time and offline progress

The console RTC is the primary cross-session clock. On Nintendo 3DS the platform adapter reads it with `osGetTime()`. Short intervals inside a running session use a monotonic tick source so sleep, HOME transitions and manual wall-clock edits cannot create negative frame time.

`TrustedClock` stores three values in the save snapshot: last observed system time, last trusted time and trust level. At startup it compares the current RTC with the saved baseline:

- a normal forward difference becomes offline progress;
- a backward difference grants zero progress and reports a clock warning;
- a large unverified forward jump is capped;
- a successful optional network verification may confirm the full real interval.

Network time is advisory. It runs outside the startup path, uses short timeouts and never changes the console clock. Missing DNS, Wi-Fi or malformed replies cannot block loading or normal play.

Long-running systems store absolute deadlines. `complete_deadline_units()` calculates completed production, training, crafting, pregnancy, travel or exploration intervals analytically, without replaying one tick per elapsed second.

Time-zone and daylight-saving changes do not require special handling because persisted values are Unix milliseconds. Player-facing messages explain limitations without removing existing progress or applying punitive effects.
