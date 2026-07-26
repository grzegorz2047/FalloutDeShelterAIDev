# Generated UI atlas

The lower-screen interface uses an original, procedurally generated 128×64 RGBA8 atlas.

## Contents

- eight 16×16 icons: build, work, collect, save, power, food, water and credits;
- four 32×24 state swatches retained for compact controls and tests;
- Polish text labels remain visible next to action icons;
- the primary context action is 194×68 pixels and the secondary actions are
  98×32 pixels;
- drawing and hit testing share the same `ShelterHudLayout` constants.

## Runtime layout

Pixels are generated deterministically at startup and written in the PICA200 8×8 Morton-tiled order expected by `C3D_Tex`.

Citro3D's `GPU_RGBA8` texture bytes are stored in PICA order (`A, B, G, R` on
the little-endian CPU). The generator explicitly converts its conventional
row-major `R, G, B, A` test representation before tiling. This prevents the
red/blue swap that previously made green interface symbols appear magenta.

- GPU texture allocation: 32 KiB;
- startup scratch storage: 32 KiB in static storage, never on the 3DS main-thread stack;
- no file I/O;
- no texture allocation or atlas generation during a frame;
- one shared texture for all lower-screen icons and state swatches.

The atlas test compares every row-major pixel with its converted tiled
counterpart, checks that every icon remains visible and confirms that the four
button states are visually distinct. UI framework tests also activate the
centre of every rendered button to guard against drawing/hitbox drift.

## Input and accessibility contract

The existing `UiTree` remains authoritative for focus, hit testing and
activation. Touch capture is exposed read-only as `pressed_id()` so rendering
cannot change input state. Disabled actions retain their reason and suggested
next step and keep a visible focus outline. Text labels are intentionally
preserved because icons alone are not sufficient for first-time users.

The large primary and secondary HUD buttons use geometric two-pixel borders
instead of stretching a 32×24 bitmap. This keeps corner and border thickness
consistent at both button sizes.

## Asset policy

All symbols and frames are original geometric artwork created specifically for Deep Shelter 3D. The atlas contains no extracted commercial-game graphics, logos or third-party icon packs.
