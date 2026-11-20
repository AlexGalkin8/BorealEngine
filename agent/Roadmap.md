# Baseline engine and deferred renderer roadmap

Updated: 2026-11-20. The user selected the goal, deferred pipeline and ImGui editor.
The breakdown and criteria below are an agent proposal. No deadlines are assigned;
no milestones are complete. This document does not automatically start implementation.

| Stage | Scope | Dependencies | Acceptance criteria | Status |
|---|---|---|---|---|
| M0. Stable baseline | Builds, texture upload, ownership, buffer upload, resize, shader paths | Recheck current code | Debug/Release build; textured model displays; resize works; no debug-layer errors in the checked scenario | Proposed |
| M1. Static models and materials | Nodes/transforms, normals/tangents, material bindings, CPU cache and GPU resources | M0 | Different models and repeated instances render correctly; geometry is shared | Proposed |
| M2. Depth prepass and G-buffer | Depth target/read access, MRT, formats/layout, barriers, two passes, debug views | M1 | Depth and G-buffer channels visually checked; geometry/depth agree between passes; resize updates targets | Proposed |
| M3. Deferred lighting | G-buffer/depth reads, position reconstruction, basic materials/lights, HDR lighting target | M2 | Lighting runs separately; reference materials and lights produce explainable results | Proposed |
| M4. Postprocessing | Chain after lighting; proposed minimum: exposure and tonemapping with correct output color space | M3 | HDR converts to the final image; parameters are editable; passes/formats verified | Proposed |
| M5. Minimal editor | ImGui, viewport, object list, add/remove models, transform inspector, IDs, save/load | M1; can precede M2–M4 | Create a level, place objects, save and restore after restart | Proposed |
| M6. Integrated demonstration | Editor-authored level through the full pipeline; diagnostics and measurements | M2–M5 | Editor/rendering work together; scenario, results and limits documented; measurements include hardware/resolution | Proposed |

## Responsibilities

The user writes shaders with agent assistance. Before M2–M4 tasks, define inputs,
coordinate spaces, formats, outputs and verification scenarios. The agent assists
with explanations, C++/RHI integration, debugging and review; HLSL implementation
by the agent requires explicit assignment.

## Suggested next task

[001 — verify and stabilize baseline rendering](tasks/001-render-baseline.md).
After M0, define model/material data and the depth/G-buffer contract.

## Further development

Potential later tasks: shadows, IBL, additional postprocessing, temporal techniques,
GI, reflections, volumetrics and GPU-driven rendering. This is neither an approved
feature list nor an agreed order. Metallic–roughness PBR is a proposal for baseline
materials; details remain open.

Add metrics and debug views alongside passes. Target RTX 3060, i7-8700K, 16 GB RAM,
1920×1080 at 60 FPS (approximately 16.67 ms per frame). At M6, validate on a defined
reference scene with recorded quality settings; do not claim success without measurements.
Render graphs, async compute, Vulkan, ECS, physics and scripting require concrete needs.
Update milestone status only with links to tasks and actual verification results.
