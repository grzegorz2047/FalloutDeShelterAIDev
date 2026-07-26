# PICA200 render-target memory budget

This document records the explicit framebuffer budget used by the shelter renderer.

## Screen targets

The top screen is 400x240 pixels per eye. Citro3D stores the target as 240x400, which is the same 96,000-pixel area.

| Target | Color | Depth/stencil | Bytes |
|---|---:|---:|---:|
| Top left | RGBA8: 384,000 | D24S8: 384,000 | 768,000 |
| Top right | RGBA8: 384,000 | D24S8: 384,000 | 768,000 |
| Bottom | RGBA8: 307,200 | D16: 153,600 | 460,800 |
| **Total** |  |  | **1,996,800 bytes (about 1.90 MiB)** |

The two stereoscopic top targets therefore consume about 1.46 MiB. The lower UI target consumes about 0.44 MiB.

## Other known allocations

- Room texture/prop atlas: 256 KiB in RGBA5551.
- Scene vertex buffer: 8,192 vertices, allocated once in linear memory.
- UI atlas and Citro2D internal buffers are separate and must be included in future runtime telemetry.

## Policy

Visual quality takes priority over the former one-draw-call target, but the game must keep an explicit memory and command-buffer budget. New render targets, shadow maps or normal maps require this table to be updated and must be validated in both mono and stereoscopic modes.
