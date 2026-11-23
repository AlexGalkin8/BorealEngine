# BorealEngine technology roadmap

Updated 2026-11-23. This proposal consolidates the attached planning and research
notes into adoption candidates. The [project roadmap](Roadmap.md) owns the
baseline milestones and the [backlog](tasks/Backlog.md) owns near-term work.
Technology choices below are open unless covered by an existing decision.

## Current baseline

The opaque G-buffer is implemented. At `8c93067`, the C++ directional
lighting path is behind a local renderer switch. The two HLSL files and
lighting-path verification are pending in [task 012](tasks/012-deferred-directional-lighting.md).
ImGui integration is visible in the working tree and its completion has not been
verified here. Finish the M3 HDR contract and M4 exposure and tone mapping.
Keep the minimal ImGui editor on its parallel track.

## Adoption order

| When | Candidate work | Evidence for proceeding |
|---|---|---|
| During M3–M6 | Shared pass contracts, capture markers, reference scenes and GPU timings; editor diagnostics | Document depth, coverage, spaces, formats, lifetime and color conversion. Establish a reproducible raster baseline. |
| First image-quality work after M4 | One directional shadow map with PCF and bias controls; gradient sky; then sky-derived diffuse lighting, depth-based AO and optional bloom or grading | Compare fixed reference captures. Add cascades when one map cannot cover the reference scene. Share the sun direction with the directional light. Add analytic sky, LUT atmosphere and specular IBL as separate models or capabilities. |
| First local lighting and screen effects | Sample depth and reconstruct position; point and spot lights through a simple light loop; GTAO, SSR with fallback, then evaluate SSGI | Validate reconstruction after resize; compare lights at known radii and angles. Profile light counts before tiling or clustering. Use a depth pyramid only when a selected consumer needs it. |
| Before the first temporal effect | Motion vectors, jitter, reprojection, history ownership and resets; validate with TAA | Check static and moving geometry, camera cuts, disocclusion and resize. Record history cost before reusing it for denoising, temporal upscaling, motion blur or volumetric fog. |
| Scene scale, alongside feature work | Draw counters, CPU bounds and frustum culling; instancing and mesh LOD; small terrain. Decide forward transparency before particles, glass or water; start with height fog before froxels | Measure with a heavier scene. Use the results to prioritize Hi-Z occlusion, indirect draws, meshlets, terrain tiling or streaming. Keep visibility buffers and virtualized geometry as separate architecture candidates. |
| When pass composition needs it | Incremental render graph | Show a concrete recurring problem in toggles, resource lifetime, transitions or pass ordering. Compare the same reference frame throughout migration. |
| After raster and temporal baselines | DXR prototype and dynamic GI comparison | Validate a small RT feature and a fallback, then compare indirect-light candidates at the same quality settings. |
| Separate game UI track | One Figma-authored HUD through a versioned export contract and a runtime prototype | Verify layout, text, assets, scaling, focus and clipping. Compare a small custom runtime, RmlUi and NoesisGUI against the same screen before selecting a dependency. |
| Research only | Vendor upscalers and frame generation, neural textures and radiance caching, RTX Mega Geometry and emerging DirectX features | Verify SDK availability, primary specs, feature queries, licensing, driver/GPU support and the fallback before proposing integration. |

## Incremental infrastructure prototypes

**Render graph.** Inventory the current frame, then wrap it as one graph pass
with imported resources. Split depth, G-buffer, lighting and output in order.
Validate resource transitions and capture parity at each split. Add persistent
history, optional features and diagnostics when they have a consumer. Measure
memory and scheduling pressure before aliasing or async compute.

**DXR and GI.** Query capabilities on each test adapter. Make geometry readable
to shaders, build BLAS for static meshes and TLAS for instances, then validate
a single RayQuery for visibility or AO. Use an offline or reference-mode
path-traced image for comparisons. Evaluate sky ambient plus AO as a baseline,
SSGI as a screen-space option, and DDGI or ReSTIR GI as later candidates.
Define scene-change and temporal stability cases, include denoising cost where
applicable, and retain a raster fallback. The RTX 3070 Ti noted in the DXR draft
is a prototype test adapter candidate. The project target remains RTX 3060,
i7-8700K and 16 GB at 1920×1080 and 60 FPS.

**Game UI and Figma.** Exercise one small HUD with a versioned manifest, named
assets, layout constraints, style tokens and fonts. Test import and runtime
output before building a full exporter. Treat interaction, localization,
font licensing and world-space UI as distinct requirements. ImGui remains the
editor and debug UI under the existing editor decision.

## Decisions by first consumer

| Before | Decide |
|---|---|
| Depth sampling and first shadow map | Record the existing depth and G-buffer contract. Decide independently whether scene and shadow depth need a different convention. |
| PBR, expanded lights and sky | Material channels, light units and exposure, sky interface and which models to support. Confirm the scope of the proposed fully dynamic scene lighting, including which precomputed tables and runtime caches are acceptable. |
| First temporal effect | Velocity semantics, jitter convention, rejection rules and lifetime. |
| Render graph or DXR prototype | Resource ownership and descriptor access for the actual scope; pass and barrier ownership during migration. |
| Game UI or terrain persistence | Runtime and export boundaries for UI; level schema and placement for terrain. |
| Any vendor SDK | Target support, license, dependency cost and fallback. |

Use fixed camera, scene, settings and hardware for comparisons. Record CPU
and GPU frame times, GPU time by pass, memory where relevant, and image
limitations. The 16.67 ms target applies to the whole frame. Assign pass
budgets after measuring the baseline.

## Source drafts

The planning inputs are `TechnologyRoadmap.md`, `PlanningSummary.md`,
`PendingDecisions.md`, `LightsPlan.md`, `ShadowPlan.md`, `SkyPlan.md`,
`ScreenEffectsPlan.md`, `RenderFeaturesPlan.md`, `UiPlan.md`, and the
two Russian-language DXR and Render Graph plans. Research inputs are
`temporal-infrastructure.md`, `global-illumination.md`,
`ui-runtime-options.md`, `figma-ui-export.md`,
`nvidia-rtx-neural-and-mega-geometry.md`, and their attached research index.
Vendor and performance claims in those drafts require primary-source or
runtime verification before adoption.
