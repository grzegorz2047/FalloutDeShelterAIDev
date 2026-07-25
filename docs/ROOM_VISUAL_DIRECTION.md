# Room visual direction

Issue: #83
Reference assets: #85

## Readability target

Each completed room must be identifiable in the first second without reading its label. Colour supports recognition, but the dominant silhouette carries the meaning.

- Power: large central generator with side control cabinets.
- Hydroponics: long planter, uneven plant heights and a grow-light bar.
- Water: broad illuminated tank with visibly connected side pipes.
- Workshop: long bench, tool board and tall industrial press.
- Storage: structural rack with crates of deliberately irregular size and height.
- Living: stacked bunks plus a separate compact sitting/storage element.

## Layering

The upper-screen shelter is rendered in three ranges:

1. rock, floors, elevator and structural shells;
2. opaque room equipment and residents;
3. illuminated strips, selection corners and foreground accents.

The previous single-draw-call target is intentionally superseded by visual quality. Geometry may use up to 8192 vertices, provided the scene remains stable in Azahar and on target hardware.

## Progress communication

Normal gameplay still starts with one completed room. Future rooms remain dark excavated cavities with muted blueprint silhouettes, so progression is not falsely communicated as already complete.

## Review checklist

- dominant object readable at native 400x240;
- inactive cavity cannot be mistaken for a functioning room;
- foreground, equipment and back wall remain separable;
- resident placement does not cover the dominant object;
- no mesh overflow;
- CIA and 3DSX build and launch successfully;
- final PR includes an Azahar screenshot and playable binaries.
