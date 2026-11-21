# 004 — Release renderer resources on shutdown

Date: 2026-11-20. Status: complete. Sample shutdown verified by the user on 2026-11-21.
Basis: the user confirmed correct model/base-color display and reported
D3D12 LIVE_RESOURCE warning #575 with an external reference count of one on close.

## Findings and changes

Architecture correction: the user confirmed centralized manager ownership in
[decision 003](../decisions/003-resource-ownership.md) and requested implementation.
The provisional renderer-owned unique_ptr fix has been replaced:

- GpuResourceManager is owned by IRenderDevice and retains all created buffers and
  textures. Only the manager can call protected device factories. Backend overrides
  are private. Consumers receive borrowed pointers, stable across vector growth.
- Mesh buffers, cached images, depth and fallback textures are created through the
  manager. Renderer/RenderMesh only keep non-owning pointers; no GPU resource deletion
  or GPU-idle wait is needed in the renderer destructor.
- Manager destruction waits for GPU completion. D3D12 device explicitly destroys
  the manager while queues, descriptor heaps and native device are still alive.
- This first version retains resources until device shutdown. Individual eviction,
  resize resource reclamation and per-queue fence retirement are not yet implemented;
  recreating resources repeatedly would retain the old allocations until shutdown.

The following paragraph records the original provisional fix, not current ownership.

The renderer released unique_ptr ownership into raw mesh-buffer and texture
pointers, while its destructor performed no cleanup. Mesh vertex/index buffers,
cached textures, depth and fallback textures now retain unique_ptr ownership.
Material texture pointers remain non-owning references to the cache or fallback.
The renderer waits for GPU completion before its owned resources are destroyed.

The device previously reported live objects before destroying its queue pools,
descriptor heaps and cached pipeline states. It now waits, destroys these owners,
then reports remaining graphics objects. The report is preserved, not suppressed.
Engine shutdown now checks the device pointer before waiting; queue waits tolerate
partially initialized devices.

## Verification and limitations

Commit 5dc3d04 was undone at the user's request. Commit 4f8ebbf contains the
replacement implementation with centralized ownership and shutdown reporting.

The user reported LIVE_DEVICE #274 with Refcount 2. The device-scoped report ran
while device_ and its queried debug interface still held references, accounting
for that count. Reporting now uses IDXGIDebug1::ReportLiveObjects(DXGI_DEBUG_ALL)
after releasing the device, adapter and factory, avoiding these diagnostic references.
Reference: https://learn.microsoft.com/en-us/windows/win32/api/dxgidebug/nf-dxgidebug-idxgidebug-reportliveobjects

The user also observed MONZA::IgcThreadingContext<...Gen12LP>::msg_end and confirmed
normal window closure. This is consistent with a handled first-chance exception;
the responsible module/stack has not been captured. No application catch-all,
debugger exception filter or driver change was introduced to hide it. If shutdown
fails or the debugger stops, capture the stack and exit code before attributing
the exception to a specific component.
Reference: https://learn.microsoft.com/en-us/visualstudio/debugger/managing-exceptions-with-the-debugger

Debug and Release x64 solution builds passed. The
native D3D12 smoke check passed three device lifetimes with 81 buffers/21 textures
each, including uploads and pointer stability after storage growth. Source and
reproduction instructions: engine/tests/render. This did not capture debug output
or exercise the full sample window shutdown.

On 2026-11-21 the user confirmed correct rendering and reported no D3D12 errors
or live-object warnings in the sample Output. The MONZA exception notification
still appears. This records the observed sample run. Resize and early resource
retirement remain separate M0 work.
