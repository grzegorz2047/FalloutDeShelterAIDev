# Room lifecycle transactions

Adjacent segments merge only when their type, level and row match. Groups are normalized from left to right and use the smallest segment ID as the stable group ID, so construction order and save migration order do not change the result. A group never exceeds the configured width.

Upgrade and demolition use preview/confirm transactions. Preview is side-effect free and reports credit impact, residents to evacuate, stored units to relocate and in-progress products to preserve. Confirmation revalidates the live state and requires a non-zero transaction ID. Reusing the same transaction ID is rejected, preventing double charges or refunds from duplicate input.

Active incidents block both upgrade and demolition. Demolition also requires enough relocation capacity for residents, stored resources and work in progress. If capacity is insufficient, the room, credits and payload remain unchanged.

A successful upgrade changes every segment in the group atomically, then normalizes grouping again. A successful demolition removes one segment, applies the refund exactly once and deterministically splits or merges the remaining segments. Save code must persist either the state before a transaction or the state after it, together with committed transaction IDs when operations can survive a restart.
