# 006 — Add engine logging

Status: planned.

Add a shared logging facility for the engine. The local `logSwapChainFailure`
helper was replaced with engine assertions as an interim step. Route recoverable
swapchain diagnostics through the new logger, including the failed DXGI
operation and its HRESULT.

Replace `OutputDebugStringA` in `DeferredRenderer::resize` with the shared
logger. Keep the diagnostic for a skipped resize when the replacement depth
texture or its DSV cannot be allocated.

Define the logging API, severity levels and output destinations before
implementation. Use the swapchain as the first migration site and verify
that a failed resize emits the expected diagnostics.
