# GPU resource manager smoke check

Requires a working Debug x64 engine build, D3D12 hardware/driver and the matching
DXC DLLs already available in the sample's Debug executable directory.
Run from Developer PowerShell at the repository root:

```powershell
cl /nologo /std:c++20 /EHsc /MDd /Iengine/include engine/tests/render/gpuresourcemanager_smoke.cpp /Fe:engine/build/checks/gpumanagercheck.exe /Fo:engine/build/checks/gpumanagercheck.obj /link /LIBPATH:engine/build/lib/render/x64/Debug /LIBPATH:engine/build/lib/core/x64/Debug /LIBPATH:engine/build/lib/platform/x64/Debug render.lib core.lib platform.lib d3d12.lib dxgi.lib dxguid.lib user32.lib advapi32.lib /PDB:engine/build/checks/gpumanagercheck.pdb
```

Create `engine/build/checks` first if absent. If compilation succeeds, run the
executable with `samples/BorealSample1/build/bin/x64/Debug` as its working directory
so DXC is discoverable (or provide the matching DXC DLLs through PATH).

The check repeats device creation/destruction three times, creates 81 buffers and
21 textures per device, uploads through the first pointers after storage growth,
and checks their descriptions. Nonzero exit means failure. It requires no window.
It exercises pointer stability, creation, upload and teardown; it does not capture
debug output or prove the absence of all leaks or sample rendering regressions.
