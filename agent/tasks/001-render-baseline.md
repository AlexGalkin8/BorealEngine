# 001 — Verify and stabilize baseline rendering

Date: 2026-11-20.
Status: proposed; implementation not started.
Related milestone: M0 in [Roadmap](../Roadmap.md).
Basis: proposal from the initial analysis. Documentation setup does not authorize
implementing the engine fixes.

## Outcome and scope

Reproducible builds and a textured model with correct resize and resource lifetime.
PBR, a new editor, Vulkan and a complete RHI rewrite are outside this task.

## Findings to recheck

These observations come from source inspection on 2026-11-20, not a new runtime test.
Confirm that each remains applicable before fixing it:
- In `deferredrenderer.cpp`, imported texture upload was commented out, while the
  fallback called `uploadPixels`, which was absent from `ITexture`.
- `D3D12Buffer::upload` checked capacity using `size >= desc_.size`.
- Renderer resources used `unique_ptr::release()` with an empty destructor.
- `Engine::handleResize` did not recreate the renderer's depth texture.
- DXC used an absolute shader path; include resolution needs verification.

## Plan

1. Inspect git diff, current sources, build settings and dependencies; obtain diagnostics.
2. Fix confirmed blockers and complete texture upload with barriers and GPU lifetime.
3. Fix resource ownership, buffer upload bounds and resize.
4. Verify the sample and a small scene with multiple materials and instances.
5. Record results and remaining issues without expanding into lighting.

## Acceptance criteria

- Debug and Release x64 build from the main solution.
- The sample launches with documented working directory and available resources.
- Model textures and fallback are visually checked.
- Resize updates dependent targets and camera aspect correctly.
- No D3D12 debug-layer errors in the tested scenario. GPU-based validation is run
  for upload/barriers, or its unavailability is explicitly recorded.
- Resources have owners and destruction respects GPU completion.

## Completed work and checks

Only this task document exists; no implementation has been performed under it.
During the initial analysis, a Debug MSBuild attempt was stopped without a result
or diagnostics. It does not establish a successful build. Release and visual checks
were not performed in that analysis. Task verification status: not-run.

## Next step

When M0 is assigned, begin with a reproducible build and refresh the confirmed issue
list. Preserve commands and diagnostic output.
