# CS330-portfolio

This repository holds my final project for CS-330: a real-time 3D scene built on the course's OpenGL framework. The scene recreates a corner of my kitchen counter: an Iwatani butane torch on its black oval base, a Primal Kitchen balsamic vinegar bottle, a Country Time Lemonade canister, and a Diet Coke can, all on a butcher-block plane. Four objects, sixteen primitives, two point lights, and a first-person camera.

I picked these objects for two reasons. Each one decomposes into the primitives the framework already ships (cylinders, boxes, a plane), so I built recognizable items without modeling custom geometry. The objects also differ in material: brushed metal, printed labels, dark matte plastic, and glass each take light differently, which gave the lighting work something to act on. Flat white objects would have hidden whether the lighting did anything.

## Layout

- `MainCode.cpp`: entry point, GLFW/GLEW init, the render loop, manager lifetimes
- `SceneManager.{h,cpp}`: scene preparation, texture/material loading, per-object draws
- `ViewManager.{h,cpp}`: camera, input handling, projection
- `Design_Decisions.docx`: the full design write-up

---

## Reflection

### How do I approach designing software?

I design from the constraints inward. The framework offered cylinders, boxes, and a plane, so I chose subjects that those primitives could actually represent rather than fighting the toolset to model something it was never built for. The lemonade canister and the soda can reduce to near-pure cylinders; the bottle is a box body with a cylinder neck and cap; the torch stacks eight primitives. That mapping happened before I wrote a draw call. Designing against what the medium can cheaply express, instead of against an ideal in my head, kept the scope honest.

The new skill here is composing recognizable objects out of deliberately limited parts and reasoning about light as a design input rather than a finishing pass. I spaced the objects farther apart than the reference photo does (bottle front-left, can front-right, torch and canister in the back row) so no object cast its neighbor into shadow and each surface stayed readable. Composition served the lighting, not just the framing.

The process was: study the reference, decompose each object into primitives, place objects for separation, then build materials and lights last so I could judge them against finished geometry. That ordering transfers directly. Letting the constraints of a system pick the shape of the solution, and deferring the expensive, hard-to-tune work until the cheap structural work is stable, is the same discipline I apply outside graphics.

### How do I approach developing programs?

I built in the stages the framework lays out and kept the per-object work behind tagged helpers. `SetTransformations` takes a scale, three rotation angles, and a position, builds the model matrix, and ships it to the shader. Every one of the sixteen draws routes through it, so the transform math lives in one place. `SetShaderTexture` and `SetShaderMaterial` resolve by string tag, which is what makes surfaces reusable: the canister band, the valve, the head cap, the nozzle, and the can rim all request `"metal"` instead of each carrying its own copy. Adding an object became mostly a matter of new numbers against an existing tag. That tag-based indirection is the development strategy I lean on most, and it is the reason the render code stayed readable as the primitive count climbed.

Iteration drove the lighting entirely. The Phong shader sums ambient, diffuse, and specular per light, then multiplies by the object's texture, so a surface needs both a material and a texture to light correctly. I started with one point light, saw surfaces fall to black, and added a second. The final setup is a warm key high and front-right doing most of the shaping and a cooler, dimmer blue fill from the left lifting the shadow side. I will be plain about this: I have no proof those exact values are objectively correct. I adjusted, re-ran, and stopped when nothing read as black and the metal caught a visible highlight. The numbers are the output of a tuning loop, not a derivation.

My approach sharpened most on failure behavior. `FindMaterial` originally reported success no matter what; I corrected it to report whether it actually found a material, so a mistyped tag now fails visibly instead of silently loading leftover values. Across the milestones I moved from getting a single object on screen toward building small, reusable, fail-loud pieces I could trust as the scene grew. That shift from "make it appear" to "make it fail in a way I can see" is what let four objects and sixteen primitives stay manageable.

### How can computer science help me in reaching my goals?

Computational graphics made the math underneath rendering concrete instead of abstract. Matrix transforms, coordinate spaces, projection, and per-fragment lighting stopped being formulas and became things I could watch break and fix. The delta-time pattern (scaling all camera movement by per-frame time so the camera travels at the same real-world rate regardless of render speed) is the kind of detail I now recognize as load-bearing rather than incidental. That habit of separating real time from frame rate, and of reasoning about what stays constant when the environment underneath shifts, carries straight into systems work where the same independence matters under uneven load.

Professionally, the transferable skill is less about graphics and more about how I handled an opaque, hard-to-verify subsystem. Lighting gave me no clean assertion to test against; I built a tight adjust-and-observe loop, kept the configurable parts behind named tags, and made the components fail visibly so problems surfaced at their source. Reusable tagged configuration, framerate-independent timing, and fail-loud defaults are the same instincts that keep larger systems debuggable. The graphics were the occasion; the working method is what I keep.
