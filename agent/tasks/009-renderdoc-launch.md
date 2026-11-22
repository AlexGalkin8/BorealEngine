# 009 — Capture frames with RenderDoc

Status: implemented and verified with RenderDoc v1.41. Depends on
[command-line handling](008-command-line-arguments.md).

See the [GPU capture tools guide](../../docs/gpu-capture-tools.md) for launch arguments.

## Implementation

`Engine::create()` reads launch options through `engine::CommandLineHandler`
and forwards the capture settings to `IRenderDevice::create`. The render device
factory calls `IGpuFrameCapturer::create()` before initializing D3D12. The
capturer uses an existing RenderDoc module or loads it and requests API version 1.4.1 using the
vendored MIT-licensed `renderdoc_app.h`. The module stays loaded for the
process lifetime to keep its graphics hooks valid. The sample starts normally
and logs a warning if RenderDoc is unavailable. [RenderDoc in-application API](https://github.com/baldurk/renderdoc/blob/v1.x/docs/in_application_api.rst)

The render device owns the optional `IGpuFrameCapturer`. The capturer tracks
capture requests, and its factory queues the first capture when requested.
The engine calls the device at the frame boundaries so the capture
includes presentation. The RenderDoc implementation logs the saved RDC path.
The same interface can support PIX later.

## Verification

Debug and Release x64 builds passed with a temporary MSVC debug-information
override to work around the local MSPDB error. The Debug sample passed the window resize
smoke check with normal startup, with a missing DLL override, and with
`--tool renderdoc capture-frame --settings data/settings.json`. The in-process
capture produced an RDC file.
Its exported command stream submitted `DepthPrepass` then `BaseColorPass`.
The Debug run with `--tool renderdoc open-capture-tool capture` saved exactly
one frame and launched the RenderDoc UI.

## Follow-up

[010 — Refactor capture-tool startup and frame capture](010-capture-tool-refactor.md).
