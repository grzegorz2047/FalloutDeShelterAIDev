# Generated UI atlas

The lower-screen interface uses an original, procedurally generated 128×64 RGBA8 atlas.

## Contents

- eight 16×16 icons: build, work, collect, save, power, food, water and credits;
- four 32×24 button frames: normal, focused, pressed and disabled;
- existing text labels remain visible next to action icons;
- button interaction bounds remain 66×34 pixels.

## Runtime layout

Pixels are generated deterministically at startup and written in the PICA200 8×8 Morton-tiled order expected by `C3D_Tex`.

- GPU texture allocation: 32 KiB;
- startup scratch storage: 32 KiB in static storage, never on the 3DS main-thread stack;
- no file I/O;
- no texture allocation or atlas generation during a frame;
- one shared texture for all lower-screen icons and button frames.

The atlas test compares every row-major pixel with its tiled counterpart, checks that every icon remains visible and confirms that the four button states are visually distinct.

## Input and accessibility contract

The existing `UiTree` remains authoritative for focus, hit testing and activation. Touch capture is exposed read-only as `pressed_id()` so rendering cannot change input state. Disabled actions retain their reason and suggested next step. Text labels are intentionally preserved because icons alone are not sufficient for first-time users.

## Asset policy

All symbols and frames are original geometric artwork created specifically for Deep Shelter 3D. The atlas contains no extracted commercial-game graphics, logos or third-party icon packs.
