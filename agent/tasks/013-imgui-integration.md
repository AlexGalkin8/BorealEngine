# 013 — Dear ImGui integration

Dear ImGui core sources are vendored at [v1.91.9b](https://github.com/ocornut/imgui/releases/tag/v1.91.9b)
under `engine/third_party/imgui`, with the upstream license.

BorealEngine feeds ImGui through `IInput` and renders its draw lists through
the RHI. The backend creates the font atlas through `GpuResourceManager`.
It records a named `ImGuiPass` after scene rendering and before presentation,
using alpha blending, per-draw scissors and indexed draws. Dynamic buffers
are reused only after the direct queue has completed their last submission.

The debug menu starts hidden. F1 toggles the menu and its panels.
**Render** opens rendering
statistics, **Engine** controls pausing on focus loss, and **Tools** opens
ImGui Metrics or requests a frame capture when a capture tool is connected.
ImGui capture suppresses camera input while the scene continues to update.
Resizing uses the current back-buffer dimensions for viewport and clipping.

This slice covers a single window, basic keyboard and mouse input, and the
debug panel. Editor panels, scene viewport, gamepad input, and platform
integration for additional windows follow separately.

## Verification

- Debug and Release x64 built with a temporary debug-information override
  for the local MSPDB issue.
- The Debug RenderDoc capture contains `ImGuiPass` after the scene preview,
  with two indexed UI draws.
- GPU readback confirmed an ImGui overlay and scene color before and after
  resize. GPU-based validation reported no D3D12 errors or corruption.
- Win32 text and wheel messages reached `IInput` and were consumed once.
- Mouse interaction verified the actions in **View**, **Render**, **Engine**,
  and **Tools**. **Tools > Capture frame** saved an RDC through RenderDoc.
- The revised Debug startup with `open-capture-tool capture` attached
  RenderDoc without saving a frame or opening its UI. The fullscreen check
  confirmed a 1920×1080 client area and swap chain, followed by restoration
  of the previous windowed dimensions.

## Follow-up

[014 — Refactor the ImGui integration](014-imgui-refactor.md).
