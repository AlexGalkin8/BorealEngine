# BorealEngine roadmap

Updated 2026-11-22. Baseline checked against `73f912a` and the repository's
task reports. The milestones follow the accepted [deferred pipeline](decisions/001-deferred-pipeline.md)
and [ImGui editor](decisions/002-imgui-editor.md) decisions. The sequence and
scope below are proposed. See the [prioritized backlog](tasks/Backlog.md) for
dependencies and individual work items.

See the [technology roadmap](TechnologyRoadmap.md) for research candidates,
adoption prerequisites and decisions beyond the baseline.

| Stage | Scope and exit criteria | Status |
|---|---|---|
| M0. Stable baseline | Build Debug and Release, run the textured sample, verify resize and check debug-layer output. | Build, texturing and shutdown evidence in tasks 002–004. Resize smoke checks in 005 and later rendering tasks. |
| M1. Static models and materials | Verify import transforms and winding on a small fixture; check normals, tangents and shared geometry across instances. | glTF base color and normal maps exercised. Broader import and instance checks remain proposed. |
| M2. Depth and G-buffer | Opaque depth prepass and two MRTs agree across frames and resize. Verify capture, readback and normal-map behavior. | Opaque scope implemented and verified in [007](tasks/007-depth-prepass.md) and [011](tasks/011-gbuffer-pass.md); RenderDoc available via [009](tasks/009-renderdoc-launch.md). |
| M3. Deferred lighting | Approve the format and color-space contract, then run [012](tasks/012-deferred-directional-lighting.md). One directional diffuse light produces predictable linear HDR values and a visible SDR preview. | Proposed next rendering milestone. |
| M4. Postprocessing | Consume M3 HDR, apply exposure and tone mapping, and encode SDR once. Verify reference pixels and resize. | Proposed. Depends on M3. |
| M5. Minimal editor | Embed the scene in an ImGui viewport. List, select, add, remove and transform objects. Save and restore a versioned level after restart. | Proposed. The debug UI and editor data model can start in parallel with M3–M4. Final viewport integration uses the M4 output. |
| M6. Integrated demo | Render an editor-authored level through the full pipeline. Record reference captures, known limitations and steady-state frame times. | Proposed. Depends on M3–M5. |
| M7. Terrain extension | First validate a small heightmap level through the same depth, G-buffer and lighting passes. Test large-world features as a separate increment. | Candidate after M6. Terrain design remains open. |

## Sequence and decisions

1. **Next.** Write down the contract for albedo, normal, depth, HDR and back-buffer
   formats and color spaces. Implement and capture task 012. Use the existing
   normal and albedo SRVs. Add depth sampling with the first position-dependent
   effect, followed by point lights. Evaluate ambient, spot lights and PBR as
   separate increments.
2. **Parallel.** Check ImGui descriptor lifetime and input routing. Build the
   debug shell over the current output and render the UI after scene color
   conversion. Decide stable actor IDs, level schema and the editor/runtime
   boundary before save/load. Decide ID-buffer picking and gizmo scope before
   those features start.
3. **Measure.** Start with a fixed scene and camera in M3. At M6 record hardware,
   resolution, quality and presentation settings, median and 95th-percentile
   frame times, and GPU time by pass. The target is 1920×1080 at 60 FPS
   (16.67 ms per frame) on RTX 3060, i7-8700K and 16 GB RAM.
4. **Extend.** Choose image-quality and performance work from the backlog based
   on reference images and measurements. Decide terrain data, materials, LOD
   and level integration in M7; use small-level evidence to decide tiling,
   coordinate precision and streaming.

The project author writes the shaders with agent assistance. Record pass inputs,
outputs, formats, coordinate spaces and verification cases before shader work.
Update milestone status with task links and evidence.
