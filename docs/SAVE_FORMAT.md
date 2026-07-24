# Save format and recovery

Deep Shelter 3D keeps two independent shelter slots. Each slot uses three files in one application-owned directory:

- `slotN.sav` — active validated generation;
- `slotN.bak` — last validated generation;
- `slotN.tmp` — incomplete candidate, never loaded as progress.

The binary header contains the `DS3D` magic number, schema version, payload length, CRC32 and monotonically increasing generation. The payload stores stable values and is decoded only after the entire file passes length and checksum validation.

## Commit protocol

1. Serialize the new generation to `.tmp`.
2. Flush, reopen and fully validate `.tmp`.
3. Rename the active file to `.bak`.
4. Rename `.tmp` to `.sav`.
5. If activation fails, restore `.bak` as the active file.

A failed write leaves the previous active or backup generation available. A newer generation wins only when its complete header, payload and checksum are valid. A future schema version is reported and is never overwritten automatically.

## Compatibility

Schema version 2 is current. Older supported payloads are migrated in memory and are written back only after the player explicitly saves. Every future migration must add a binary fixture and round-trip test.

CIA and 3DSX builds must point at the same application-owned save directory so updating or changing launch format does not create competing progress files.

## Player-facing repair actions

- `NoSpace`: preserve the previous save and ask the player to free SD-card space before retrying.
- `Corrupt` with valid backup: load the backup and disclose the recovered generation.
- both copies invalid: keep files for diagnostics and offer reset of only the affected slot.
- future version: ask the player to update the game; never reset or save over it.

`SaveStore::export_metadata` produces non-sensitive slot diagnostics without dumping gameplay payloads.
