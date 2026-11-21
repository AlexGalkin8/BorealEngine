# 005 — Keep the depth target in sync with window resize

Date: 2026-11-21. Related milestone: M0.

## Changes

The engine initializes the renderer and camera from the actual swapchain buffer
size. On resize, the engine waits for GPU idle and resizes the swapchain first.
It passes the acquired back-buffer dimensions to the renderer, which resizes
its depth target independently. If allocation fails, the renderer logs the
failure and keeps the previous depth target. The engine then attempts to restore
the swapchain to those dimensions. It updates the camera aspect when the resize
leaves matching color and depth targets.

The renderer factory requires valid initial depth and fallback textures before
returning a renderer. Both pointers are private, and the render pass uses the
depth texture by reference. Swapchain dimensions are zero when back buffers
are unavailable. The engine skips rendering and presentation when the swapchain
size differs from the renderer size.
The renderer asserts the color/depth size invariant before recording the pass.
The engine filters zero-size window events, and both resize methods assert
this precondition.

The swapchain resize API returns void and calls `ResizeBuffers` directly. DXGI
uses the debug policy set by the render device. The `D3D12_ASSERT` family
reports failed HRESULTs in Debug. The swapchain reacquires buffers
after a resize attempt, including when DXGI leaves the previous buffers in
place. If it cannot acquire the new buffers after a successful native resize,
it attempts to restore the previous size. When
both acquisition and recovery fail, the engine keeps running and skips rendering
until a later window size event can recover the swapchain.

The GPU resource manager releases the unused depth texture after GPU idle.
Two DSV slots allow the previous target and its replacement to coexist briefly.
General asynchronous retirement remains separate work.

## Verification

Debug and Release x64 solution builds passed. A Win32 smoke check against the
Debug sample changed the window size eight times, minimized and restored it,
then closed it normally with exit code 0. The check did not capture the
D3D12 Output or inspect the rendered image.

Before the resize methods were separated, a Debug and Release probe held a
native reference to a back buffer to make DXGI reject two resize calls from
640×480 to 800×600.
The old size remained available after both failures. After releasing the
reference, the next resize succeeded at 800×600. An earlier incremental Debug
build following a PDB write error crashed on resize. The crash did not
reproduce after a full Debug rebuild.

Engine assertions replace the local swapchain log helper until the shared
logger in [task 006](006-engine-logging.md) is available. After that change,
both configurations built, the Debug sample passed the normal resize smoke
check, and the Release probe passed both forced failures and the subsequent retry.

After separating the resize methods, Debug and Release x64 builds passed.
The Debug sample passed the eight-resize, minimize/restore and close check.
A Release probe confirmed that each resize changes only its own target. The
swapchain preserved the old buffers through two forced DXGI failures and
resized successfully after the native reference was released.
