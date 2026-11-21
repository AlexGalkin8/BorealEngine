# 006 — Add engine logging

Status: implemented. Release diagnostics verified.

## Implementation

The engine logger in `engine/core/utility/log.h` provides `LOG_INFO`,
`LOG_WARNING`, and `LOG_ERROR`. Each call takes a category and a C++20 format
string. The logger records the source file and line automatically.

```cpp
LOG_WARNING("Renderer", "Resize to {}x{} skipped.", width, height);
```

Every log line starts with `[BorealEngine]`, followed by the level, category,
and message. The source file name and line appear at the end, for example
`(deferredrenderer.cpp:149)`.

Debug and Release write to `stderr` and the Windows debugger output stream.
To save a session, redirect `stderr` when launching the process.

D3D12 device initialization and resize diagnostics use the shared logger.
Swapchain failures include the failed DXGI operation and its `HRESULT` where
available. `DeferredRenderer::resize` logs skipped resizes when the replacement
depth texture or its DSV cannot be allocated. In Debug, the `D3D12_ASSERT`
family logs the failed expression and `HRESULT` at the call site before
breaking into the debugger. The existing error paths handle recovery.

## Verification

Debug and Release x64 builds passed. The Debug sample passed eight resizes,
minimize and restore, and a normal close. A Release probe captured `stderr`
and confirmed that two rejected DXGI resizes log the same failing operation,
dimensions, `HRESULT`, and call site. Exhausting the DSV heap logged the reason
for skipping the renderer resize. Both resize paths succeeded on retry.
The Visual Studio Output window was outside this check.

A separate assertion probe verified the expression, `HRESULT`, and macro call
site in Debug, with one debugger break. The return variant evaluated the
expression once and returned the fallback value in both configurations.
The Debug sample and Release resize probe both emitted the `[BorealEngine]`
prefix and short source locations in `stderr`.
