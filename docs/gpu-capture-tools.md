# GPU capture tools

BorealSample1 selects a GPU capture tool with `--tool <name>`. RenderDoc is
currently supported. Run the examples from `samples/BorealSample1` or add the
flags to the sample project's **Debugging > Command Arguments** in Visual Studio.
See [Run the sample](../README.md#run-the-sample) for the working directory and
runtime dependencies.

## Launch options

Use `--tool <name> [options]`. See the [command-line reference](command-line.md)
for how to combine it with other startup commands.

| Option | Behavior |
|---|---|
| `tool-path <path>` | Overrides the selected tool's library path. |
| `open-capture-tool live` | Opens the tool interface at startup. |
| `open-capture-tool capture` | Opens a saved capture after you request a frame capture. |
| `capture-frame` | Captures the first rendered frame. Omit `open-capture-tool` to save it without opening the interface. |

## RenderDoc

Use `--tool renderdoc` to enable the in-process capture API. The sample loads
`%ProgramFiles%\RenderDoc\renderdoc.dll` by default before initializing D3D12.
For a custom installation, pass `tool-path` with the path to the DLL. If the
capture API is unavailable, the sample logs a warning and starts normally.
Press **F1** in the sample, then select **Tools > Capture frame**. The sample logs the saved `.rdc` path.

```powershell
.\build\bin\x64\Debug\BorealSample1.exe --tool renderdoc
.\build\bin\x64\Debug\BorealSample1.exe --tool renderdoc open-capture-tool live
.\build\bin\x64\Debug\BorealSample1.exe --tool renderdoc open-capture-tool capture
.\build\bin\x64\Debug\BorealSample1.exe --tool renderdoc tool-path 'C:\Tools\RenderDoc\renderdoc.dll'
```
