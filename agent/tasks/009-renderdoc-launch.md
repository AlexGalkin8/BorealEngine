# 009 — Launch the sample with RenderDoc

Status: planned. Depends on [command-line handling](008-command-line-arguments.md).

## Outcome

Start the Windows x64 sample in a RenderDoc capture session when requested
through command-line arguments. Use the session to inspect `DepthPrepass` and
`BaseColorPass` in one frame.

## Launch arguments

- `--renderdoc` requests a RenderDoc launch.
- `--renderdoc-path <path>` specifies the path to `qrenderdoc.exe` when the
  default installation cannot be found.

```powershell
& 'path\to\BorealSample1.exe' --renderdoc --renderdoc-path 'C:\Program Files\RenderDoc\qrenderdoc.exe'
```

## Plan

Handle the flags at the sample entry point before `Engine::create` and D3D12
initialization. Use RenderDoc's auto-start capture settings workflow to open
its UI and launch the sample under capture. The settings must use the actual
sample executable, preserve its working directory for `data/settings.json`,
and forward application arguments after consuming the RenderDoc launch flags.
This gives the bootstrap process one launch and prevents the captured child
from relaunching itself. [RenderDoc capture documentation](https://github.com/baldurk/renderdoc/blob/v1.x/docs/how/how_capture_frame.rst)

If RenderDoc is missing or its process cannot start, report the reason through
the engine logger and continue with ordinary sample startup.

## Acceptance criteria

- Launching the sample without `--renderdoc` behaves as before.
- A valid RenderDoc installation opens the UI and starts a single captured
  sample. Paths with spaces work, and the sample loads its normal scene.
- A missing or invalid RenderDoc path produces a diagnostic and still allows
  the sample to run normally.
- Capture a frame in Debug x64 and confirm the depth-only draw pass precedes
  the base-color pass in the event list.
