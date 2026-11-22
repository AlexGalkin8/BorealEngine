# 012 — Deferred directional lighting

- Date: 2026-11-22
- Status: in progress. C++ integration is staged behind a local renderer switch.
- Related roadmap milestone: M3. Deferred lighting
- Authorization: continue development under the agreed plan

## Outcome and scope

Render the existing opaque scene with one world-space directional light. Record
one fullscreen lighting draw into a linear HDR target, then show it through a
temporary SDR preview. Use the existing direct queue and fullscreen triangle
pattern. Schedule depth sampling and position reconstruction with the first
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
| Back buffer | `RGBA8_UNORM` | The preview clamps linear RGB to [0, 1] and applies the existing sRGB transfer function once. |

## Plan

1. Define the shader bindings and reference light. Use an SRV table for albedo
   and normals and per-pass constants for a normalized world-space vector
   pointing **toward** the light and an RGB linear light gain. Set the light
   in the renderer for this task.
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
4. Replace the main-path albedo preview with `LightingPreview`. Read the HDR
   SRV and write every pixel to the UNORM back buffer. Reuse the existing
   present-to-RTV-to-present sequence. M4 can consume the same linear HDR SRV
   for exposure and tone mapping.
5. The project author writes the HLSL. The development chat handles C++/RHI
   integration and assists with the shader contract, debugging, and review.

## Shader handoff

Both shaders use the existing `vertexShader` and `pixelShader` entry points
and the fullscreen triangle from `gbuffer_base_color.hlsl`. Use integer
`SV_Position.xy` with `Load(..., mip 0)` for all texture reads.

| File | Shader inputs | Output |
|---|---|---|
| `engine/shaders/deferred_lighting.hlsl` | `t0` albedo and coverage, `t1` signed world normal, `b0` directional light constants | Linear RGB and the original coverage in `SceneLightingHDR`. |
| `engine/shaders/lighting_preview.hlsl` | `t0` linear HDR with coverage in alpha | Clamp linear RGB to [0, 1] and apply the sRGB transfer used by the albedo preview. Write black for zero coverage. |

`b0` contains eight 32-bit floats: direction toward the light in XYZ and one
padding float, then RGB light gain and one padding float. The reference
direction is `(0.36, 0.48, -0.8)` and the gain is `(2, 2, 2)`.
The lighting pixel shader should preserve coverage for dark geometry. Before
normalization, check for a zero normal and output black in that case.

Set `kEnableDeferredLighting` in `deferredrenderer.cpp` to `true` once both
shader files are ready. The disabled path retains the existing albedo preview
and allocates only the prepass and G-buffer targets.

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

With lighting disabled, Debug and Release x64 built using a temporary
debug-information override for the local MSPDB issue. Debug passed resize and
RenderDoc capture smoke checks. GPU readback confirmed visible albedo at
1280×720 and 960×540 with no GPU-based validation errors.

Lighting-path checks below are pending the two HLSL files.

1. Build Debug and Release x64. Start the Debug sample from
   `samples/BorealSample1` with
   `--tool renderdoc open-capture-tool capture --settings <settings-path>`.
   Set the window size in the capture settings and confirm the actual target
   size in RenderDoc. For a capture after a resize, use
   `--tool renderdoc open-capture-tool live` and trigger it after the new size
   has settled. See the [capture guide](../../docs/gpu-capture-tools.md).
2. In the Event Browser, verify the sequence
   `DepthPrepass → GBufferPass → DeferredLighting → LightingPreview → Present`.
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
