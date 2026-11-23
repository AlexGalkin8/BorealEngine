# Command-line reference

Startup commands begin with `--`. Tool parameters follow the tool name without
a prefix.

| Command | Behavior |
|---|---|
| `--settings <path>` | Loads the specified settings file. The default is `data/settings.json`. |
| `--tool <name> [options]` | Enables a tool and passes it the following parameters. See [GPU capture tools](gpu-capture-tools.md) for supported tools and parameters. |

A tool's parameters end at the next startup command or at the end of the
argument list. Quote paths that contain spaces. A standalone `--` ends startup
command parsing.

Run from `samples/BorealSample1`.

```powershell
.\build\bin\x64\Debug\BorealSample1.exe --tool renderdoc --settings 'data/settings.json'
```
