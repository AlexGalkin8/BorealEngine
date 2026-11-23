# 010 — Refactor capture-tool startup and frame capture

- Date: 2026-11-22
- Status: planned
- Depends on [command-line handling](008-command-line-arguments.md) and
  [RenderDoc capture](009-renderdoc-launch.md).

## Outcome and scope

Simplify the path from command-line parsing to frame capture while preserving
the current RenderDoc workflow. Review the design before adding another capture
backend. PIX integration is a separate task.

## Plan

1. Review how `CommandLineHandler` locates the `--tool` argument and its
   parameters. Look for a way to remove the second lookup and the parser copy
   while retaining argument boundaries and diagnostics.
2. Review how `FrameCaptureSettings` travels through `Engine::create()` and
   device creation. Keep capture-tool loading ordered before native D3D12
   initialization and ownership with the render device.
3. Review the boundary between capture state and RenderDoc API calls. The
   backend-independent state transitions currently live in
   `renderdoccapturer.cpp`. Decide whether to move them into a shared
   implementation in preparation for a second backend.

## Acceptance criteria

- Preserve the syntax in the [command-line reference](../../docs/command-line.md)
  and [GPU capture tools guide](../../docs/gpu-capture-tools.md), including paths
  with spaces, a following `--settings` command and the standalone `--` delimiter.
- Verify normal startup, malformed arguments and startup with a missing RenderDoc
  DLL.
- Verify attach-only and both UI modes. Confirm that a manual capture request
  saves one frame and that the capture
  includes presentation and submits `DepthPrepass` before `BaseColorPass`.
- Build Debug and Release x64 and record the verification results.
