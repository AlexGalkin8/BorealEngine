# 014 — Refactor the ImGui integration

Status: planned. Depends on [Dear ImGui integration](013-imgui-integration.md).

- Review the boundary between `DebugMenu`, the editor, and the shared
  engine-owned `ImGuiBackend`. Keep debug and editor state independent.
- Review input capture and focus handling, including camera control while
  interacting with UI and restoring it when the UI is hidden.
- Review draw-data uploads and buffer lifetime. Preserve fence-based reuse and
  the engine resource manager's ownership as editor draw volume grows.

Verify F1, menu actions, manual RenderDoc capture, fullscreen and windowed
resizes, and D3D12 validation in Debug. Build Debug and Release x64.
