# 012 — Deferred directional lighting

- Date: 2026-11-22
- Status: implemented. HDR readback and performance verification are tracked below.
- Related roadmap milestone: M3. Deferred lighting
- Authorization: continue development under the agreed plan

## Outcome and scope

Render the existing opaque scene with one world-space directional light. Record
one fullscreen lighting draw into a linear HDR target, then pass it to
[postprocessing](016-postprocess-and-debug-views.md). Use the existing direct queue and fullscreen triangle
pattern. The scene owns the directional light settings. Schedule depth sampling and position reconstruction with the first
position-dependent effect. Point lights, shadows, PBR material channels, and the
postprocessing chain have separate scope.

## Starting point

[Task 011](011-gbuffer-pass.md) and commit `73f912a` provide the opaque depth
prepass, a two-target G-buffer, and an albedo preview. Both color targets already
have SRVs and transition back to shader-resource state after the G-buffer pass.
The depth resource is a `D32_FLOAT` DSV with a clear value of 1. The G-buffer
uses the same depth attachment with an EQUAL test and depth writes disabled.
The current D3D12 depth allocation creates only a DSV.

| Resource | Format | Contract |
|---|---|---|
| `GBufferAlbedo` | `RGBA8_UNORM_SRGB` | The SRV returns linear RGB. Alpha is 0 on the clear and 1 for covered opaque pixels. |
| `GBufferWorldNormal` | `RGBA16_FLOAT` | Signed world-space normal in RGB, including the glTF normal-map result. |
| `SceneLightingHDR` (new) | `RGBA16_FLOAT` | Linear lit RGB, with values above 1 preserved. Alpha carries opaque coverage. |
| Back buffer | `RGBA8_UNORM` | Postprocessing tone maps linear RGB and applies the sRGB transfer once. |

## Plan

1. Define the shader bindings and reference light. Use an SRV table for albedo
   and normals and per-pass constants for a normalized world-space vector
   pointing **toward** the light and an RGB linear light gain. Set the defaults
   in the scene.
2. Add `SceneLightingHDR` through the existing GPU resource manager. Allocate it
   with the depth and G-buffer replacements before publishing the resized set.
   Reuse the engine's GPU-idle resize path and preserve its allocation failure
   behavior. Name the resource and the two new command lists for captures.
3. Record `DeferredLighting` after `GBufferPass`. Bind the HDR RTV with depth
   testing and blending disabled. Load mip 0 at the integer pixel coordinates
   derived from `SV_Position`. For covered pixels with a nonzero normal, use
   `albedo * lightGain * max(dot(normalize(N), L), 0)`. The light gain folds in
   the constant diffuse scale. Write black for background or a zero normal,
   preserving coverage in alpha. Transition HDR from shader resource to render
   target before the draw and back to shader resource before the preview.
4. Pass the HDR result to the active [postprocess pass](016-postprocess-and-debug-views.md).

## Shader bindings

`deferred_lighting.hlsl` loads albedo and coverage from `t0` and signed
world normals from `t1`. `b0` contains direction toward the light
`(0.36, 0.48, -0.8)` and RGB gain `(2, 2, 2)` in two float4 registers.
The pass writes linear HDR and preserves coverage.

## Acceptance criteria

- On a flat reference surface, compare interior HDR pixels against the diffuse
  equation using the actual bound light constants and G-buffer values. A white
  texel with an aligned normal and gain 2 produces RGB 2 in HDR. Perpendicular
  and opposite normals produce RGB 0. Compare with floating-point tolerance
  and check that the coverage mask distinguishes unlit geometry from background.
- On the default `pudge.glb` scene, a fixed camera and an oblique light produce
  corresponding changes in the normal and HDR targets when normal maps are
  disabled. Use a controlled test variant or a temporary zero normal scale.
- At 1920×1080 and after a resize to a different aspect ratio and back, the
  depth, G-buffer, HDR target, viewport, scissor, and back buffer agree in size.
  A frame with no opaque draws shows black with zero HDR coverage. Consecutive
  frames and resize pass D3D12 debug-layer and GPU-based validation.
- Record a Release performance baseline at 1920×1080 using a fixed scene and
  camera. Report lighting and preview GPU time, total GPU frame time, and CPU
  frame time over a warmed-up run. Record hardware, driver, scene, resolution,
  frame sample count, and presentation settings. Compare the median and 95th
  percentile with the project target of 16.67 ms per frame. Record the actual
  hardware and mark target-hardware validation pending if it is unavailable.
  Set pass budgets after measuring the baseline.

## Verification

The active lighting path passed the Debug sample launch and resize smoke check. Performance measurement remains pending.

1. Build Debug and Release x64. Start the Debug sample from
   `samples/BorealSample1` with
   `--tool renderdoc open-capture-tool capture --settings <settings-path>`.
   Press F1 and select **Tools > Capture frame**.
   Set the window size in the capture settings and confirm the actual target
   size in RenderDoc. For a capture after a resize, use
   `--tool renderdoc open-capture-tool live` and trigger it after the new size
   has settled. See the [capture guide](../../docs/gpu-capture-tools.md).
2. In the Event Browser, verify the sequence
   `DepthPrepass → GBufferPass → DeferredLighting → Postprocess → Present`.
   In Pipeline State, verify the lighting draw has three vertices, the
   expected two PS SRVs, light constants, an HDR RTV, and no DSV. Verify that
   the preview reads the same HDR resource. Inspect resource barriers in the
   API calls. [RenderDoc Event Browser](https://github.com/baldurk/renderdoc/blob/v1.x/docs/window/event_browser.rst)
3. Use Texture Viewer at the relevant draw events to compare albedo, signed
   normal values, HDR and the back buffer at matching pixel coordinates.
   Inspect raw HDR values above 1 and confirm that only the preview clips them.
   Save the capture, revision, and representative pixel coordinates and values.
4. Use RenderDoc action durations to identify expensive draws during replay.
   Measure the steady-state frame budget outside capture and GPU-based
   validation, using direct-queue timestamps or a GPU profiler. Include the
   barriers in pass-level measurements where possible, and account for
   presentation waits when interpreting CPU frame times.

## Limitations and continuation

At 1080p the single HDR target holds about 15.82 MiB of texel data before
allocation overhead. Its full-screen write and the preview read add bandwidth.
Measure both passes before changing the G-buffer formats.

For the first position-dependent light or effect, reuse the existing depth
allocation with a typeless `R32_TYPELESS` resource, a `D32_FLOAT` DSV and an
`R32_FLOAT` SRV. Plan explicit states for the prepass, the read-only depth test,
and the shader read. Reconstruct world position with the inverse of the same
view-projection matrix used to fill the depth, using D3D depth in [0, 1],
pixel-center coordinates, viewport Y inversion and a homogeneous divide.
Validate by reprojecting known points after a camera move and resize.
[D3D12 resource states](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_resource_states)

## PBR material inputs

The opaque G-buffer multiplies sampled albedo and vertex color by the glTF
base color factor. Its linear RG8_UNORM material target stores roughness in R
and metallic in G. This stage reads scalar factors. Sampling the glTF
metallic-roughness texture and evaluating the PBR BRDF are follow-up work.

Open F1 > Scene > pbr_materials_test in BorealSample1, or start it from its sample directory with
`--settings data/pbr_materials_test_settings.json`. The test scene opts into
glTF node transforms for the sphere grid. Use F1 and select Roughness or
Metallic under Render > Debug view. GPU readback confirmed all eight
roughness levels and metallic 1.0 on covered pixels at 1280x720 and after a
resize to 960x540 with D3D12 GPU-based validation enabled. The default scene
also passed the existing diffuse lighting and debug view readback check.

## Directional light controls

Use F1 > Render > Directional light to change the enabled state, direction,
linear RGB color, and intensity. Scene JSON can set the initial values via
`directional_light.direction_to_light`, `directional_light.color`, and
`directional_light.intensity`. Changes apply to the active scene at runtime.
Reloading a scene restores its initial values.

## Point lights

The deferred pass supports up to four point lights. It reconstructs world
position from the D32 depth buffer through an R32_FLOAT SRV and the inverse
camera view-projection matrix. Point lights use clamped inverse-square
attenuation with a smooth cutoff at the configured radius.

Open F1 > Render > Point lights to add, remove, or edit lights at runtime.
The PBR materials test scene starts with two colored lights. Disable the
directional light to inspect their contribution separately. The scene JSON
stores the initial position, linear RGB color, intensity, and radius.

Debug GPU readback matched an independent CPU reference before and after
camera movement and resize. A disabled light and a second light with its
radius reduced below the distance to any surface produced zero contribution.
