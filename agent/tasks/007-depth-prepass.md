# 007 — Add an opaque depth prepass

Date: 2026-11-22. Related milestone: M2. Status: implemented.

## Changes

`DepthPrepass` uses `depth_prepass.hlsl` and a vertex-only graphics pipeline.
It binds and clears the D32 depth target, then draws the visible opaque meshes
with `Less` depth testing and writes enabled. The base-color pass draws the
same meshes with `Equal` depth testing and depth writes disabled. It writes
base color to the swapchain buffer.

Both shaders use `mesh_transform.hlsli` for the same precise clip-space
position calculation. The renderer prepares the draw list and camera constants
once per frame. It transitions depth to read state between passes and back
to write state before the next frame.

The D3D12 command list can bind a depth target without a color target. It
clears stencil only for formats that contain it. Graphics shader stages now
use distinct bits so the pipeline can select the vertex stage alone.

## Verification

Debug and Release x64 engine and sample builds passed. The Debug sample
survived eight window resizes, minimize and restore, and normal close.

A Debug GPU readback probe rendered the sample scene twice at 1280×720 and
twice after resizing to 960×540. It found 457,423 and 257,332 color pixels
above the background threshold, respectively, with identical counts across
both frames at each size. GPU-based validation reported no D3D12 errors in
the probe. A frame capture was outside this check.

## Scope

This pass covers the current opaque geometry. G-buffer attachments and
deferred lighting are separate work.
