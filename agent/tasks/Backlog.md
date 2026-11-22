# Prioritized development backlog

Consolidated from the Claude planning drafts dated 2026-11-22 and the repository
task index. Numbers in the source drafts identify proposals. Existing repository
task IDs retain their meaning. New entries below are proposals unless their
status is explicitly recorded elsewhere.

- **P0** is on the critical path for its stage.
- **P1** is valuable in that stage and can run alongside critical work.
- **P2** is deferred or contingent on a concrete need.

## M0–M2 — Baseline, diagnostics and test foundations

| Priority | Task | Dependency or completion check |
|---|---|---|
| P0 | Format and color-space contract (draft 024) | Document albedo, normal, depth, HDR and output formats; check existing upload and view paths. Feed M3 and M4. |
| P1 | Math tests (020), then import tests (021) | Minimal CLI test setup; known matrix, quaternion and normal cases; glTF fixture covering hierarchy, winding, normals and tangents. Pair with actionable import validation. |
| P1 | Resource lifetime tests (022) | Reuse the test setup. Exercise release and reuse after completion; keep a D3D12 validation scenario for GPU-backed behavior. Scope tests to the retirement behavior the engine supports. |
| P1 | Capture diagnostics | Reuse [009](009-renderdoc-launch.md); fill gaps in D3D12 object names and named pass markers when testing M3. PIX and [capture startup refactoring](010-capture-tool-refactor.md) are follow-ups when needed. |
| P1 | Reproducible build and launch | Check the existing build and paths first. Merge the build helper, sample preflight and CMake script audit where appropriate. Check Debug/Release, working directory independence and missing inputs. Verify settings fallback before extending settings. |
| P2 | Repository and workflow hygiene | Ignore/attributes, style examples (027), task/decision/review/devlog templates and automation, renderer review checklist. Apply where concrete friction exists. |

## M3–M4 — Rendering

| Priority | Task | Dependency or completion check |
|---|---|---|
| P0 | Directional light and linear HDR ([012](012-deferred-directional-lighting.md), L1 and first slice of L3) | Needs the format contract. Validate known diffuse reference pixels, HDR values above 1, normal maps and resize. |
| P0 | Exposure and tone mapping | Needs M3 HDR. Verify output transfer and exposure response; add runtime controls when ImGui is available. |
| P1 | Reference scenes and measurement (L7) | Start with a fixed material/light scene in M3; add a heavier load scene for M6. Record GPU pass timing, CPU frame time and frame statistics. |
| P1 | Debug views and named target registry (023, L6) | Add albedo, signed normals and HDR views as consumers arrive. Centralize target names and formats when shared by passes and UI. Add depth view when depth SRV exists. |
| P2 | Local lights (L2, L4, L5) | Depth SRV and inverse-view-projection reconstruction first; check reprojection after camera movement and resize. Then point light buffer, attenuation and radius, followed by spot cones and controls. |
| P2 | Expand material and shader contracts | Decide ambient contribution, metallic–roughness data, physical light units and alpha masking before adopting them. Share HLSL definitions and validate C++/HLSL constant layouts where the passes share data. |

## M5 — Debug UI and minimal editor

| Priority | Task | Dependency or completion check |
|---|---|---|
| P0 | ImGui integration and debug shell | The external draft numbered 010 is an ImGui proposal. Use its menu layout as design input. Check Win32/D3D12 integration, descriptors, frame ordering, shutdown and input capture; verify F1 and independent Debug/View panels. |
| P0 | Scene viewport (E1), outliner and transform inspector (E3) | Check focused input, resize and selection consistency. Start selection in the outliner. Decide stable scene IDs and per-instance data before persistence. |
| P0 | Model picker and level save/load (E6, E7) | Add/remove instances with shared geometry. Version the level schema and verify object IDs and transforms after restart. |
| P1 | Viewport picking (E2) and bounds overlay (025) | Check overlaps, empty clicks and viewport coordinates after resize. Verify transformed bounds. Reuse the bounds for culling if needed. |
| P1 | Rendering, Camera and Log panel content; wireframe (026) | Needs the debug shell. Expose narrow settings and counters. Check wireframe depth/G-buffer consistency and restoring the normal view. |
| P2 | Editing extensions (E4, E5, E8) | Gizmo and snapping, undo/redo for object commands, then Play In Editor if needed. Also consider editor settings, autosave, docking, thumbnails, drag/drop, grouping, hierarchy, light gizmos and camera bookmarks. |

## M6 — Integration and release readiness

| Priority | Task | Dependency or completion check |
|---|---|---|
| P0 | Integrated demo and performance run | Needs M3–M5. Open an editor-authored scene after restart and capture the full pipeline. Record frame times against the target with fixed render size and known VSync state. |
| P1 | Reproducibility and documentation | Reference image comparison with tolerance; camera/scene/quality snapshot; known limitations (028); milestone evidence (029, initiated in this roadmap); build/run README and dependency licenses before distribution. |
| P1 | Platform diagnostics | Record adapter, driver and debug-layer state; report unsupported required capabilities clearly. Verify live objects where relevant and assess crash logs/minidumps before wider testing. |
| P2 | Optimize measured bottlenecks | Counters before frustum culling, batching or upload memory work. Assess Release symbols, CI toolchain, build time and profiler integration when needed. |

## M7 and later — Extensions

| Priority | Task | Dependency or completion check |
|---|---|---|
| P0 for M7 | Small terrain (T1–T7, T9) | Decide heightmap and chunk representation. Render grid relief, then normals and layers in the existing passes. Add culling and seam-safe LOD as needed. Add height query and editor placement, persist terrain data, and record pass timings. |
| P2 | Large terrain (T8) | After small terrain works, compare quadtree and simpler chunk layouts, measure memory and precision, then add tiled data or streaming if needed. Sculpting, painting, vegetation, water, procedural generation, terrain collision and caves are separate candidates. |
| P2 | Image quality | Directional shadows before cascades; IBL/skybox, SSAO after depth SRV, bloom/emissive after HDR, and TAA after motion vectors and jitter. Decide forward transparency with material and output contracts. Area lights and clustered lighting need separate justification. |
| P2 | Engine scaling | Asset cache, compression/mips and hot reload as assets grow. Evaluate descriptor and upload reuse, asset IDs and scene reload lifetime with concrete workloads. Consider configurable input, fixed timesteps, shared pass abstractions, job systems and other architectural changes when a feature or measurement calls for them. |
