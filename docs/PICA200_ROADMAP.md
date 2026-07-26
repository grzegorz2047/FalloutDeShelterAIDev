# PICA200 visual roadmap

This branch prioritizes visual quality over the previous minimal draw-call target.

Immediate pass:
- 24-bit depth plus 8-bit stencil on top-screen targets;
- separate structure, detailed-prop and additive-light ranges;
- linear material filtering with protected atlas borders;
- native Azahar validation at 400x240 per eye.

Deferred shader pass:
- editable shader source restoration;
- hardware lighting and cel-shading experiments;
- normal-map and shadow feasibility tests;
- measured GPU, CPU and command-buffer budgets on target hardware.
