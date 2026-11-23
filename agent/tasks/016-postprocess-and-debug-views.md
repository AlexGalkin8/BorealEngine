# 016 — Postprocess and debug render views

The frame executes `DepthPrepass → GBufferPass → DeferredLighting → Postprocess`.
The ImGui overlay, when visible, is submitted after postprocessing.

`DeferredLighting` writes linear HDR to `SceneLightingHDR` (RGBA16F). The
final view applies fixed exposure (1.0), Reinhard tone mapping, and an sRGB
transfer function before writing the UNORM back buffer.

Press F1 and use **Render > Debug view** to switch between final output,
G-buffer albedo, signed world normals remapped to RGB, and HDR lighting
clipped for display on the SDR back buffer. Debug views preserve the pass
sequence so all resources remain available in a capture.

Depth visualization needs a depth SRV. The directional lighting pass uses
only the G-buffer SRVs.

## Verification

- Debug x64 built with the local MSPDB workaround. The sample passed startup,
  eight window resizes, minimize/restore, and shutdown.
- GPU readback on the default scene matched the diffuse equation within
  0.0075 absolute error, including sRGB decode precision and HDR quantization.
  It found 11,851 HDR channel samples above 1 and confirmed zero background
  coverage, distinct output views, and clean GPU-based validation after resize.
- Release runtime diagnosis and the 1080p performance baseline are pending.
