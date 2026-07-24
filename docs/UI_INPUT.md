# Unified UI input contract

Every actionable lower-screen control is represented once in `UiTree`. Touch and physical buttons route through the same control ID and produce at most one `UiAction` per frame.

## Input priority

1. Cancel clears an active touch capture and emits one cancel action.
2. A new touch captures the topmost visible hitbox and suppresses button activation for that frame.
3. Hold/drag remains owned by the captured control until release or cancel.
4. Release activates only when it occurs on the same control; release outside cancels safely.
5. Without touch capture, D-pad moves focus and A activates the focused control.

This ordering prevents simultaneous A and touch from executing a transaction twice.

## Focus

Focus navigation uses deterministic geometry and stable control IDs. Removing, hiding or changing a control repairs focus to the first legal item. Disabled controls can retain focus so the player can inspect the reason they are unavailable.

## Disabled actions

A disabled activation returns `ShowDisabledReason` with both a concrete reason and a next step. Domain code does not receive a partial command.

## Layout and accessibility

Controls must fit in the configured safe area. The current 3DS layout reserves at least four pixels from screen edges. Text rendering and localization can change labels without changing control IDs, hitboxes or focus order. Every touch control must remain reachable with D-pad, A and B.
