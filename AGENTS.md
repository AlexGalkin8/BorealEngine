# BorealEngine agent instructions

## Project goals

BorealEngine is a personal game engine project by a Render Engineer. It should
support model loading, level creation and rendering, with an emphasis on modern,
realistic graphics. The selected baseline is depth prepass → G-buffer → deferred
lighting → postprocessing, with a minimal Dear ImGui level editor.
Target baseline hardware: RTX 3060, i7-8700K, 16 GB RAM. Target: 1920×1080 at
60 FPS (approximately 16.67 ms per frame). These are targets, not measured results.
There is no fixed schedule. See `agent/ProjectBrief.md` for details.
Do not treat proposals as accepted decisions or interpret the long-term ambition
as a request to implement every rendering technology at once. Vulkan and other
platform priorities remain open.

## Collaboration and language

- Discuss the project with the user in Russian.
- Write code comments, commit subjects and bodies, and project documentation in English.
  This includes agent instructions, task reports, reviews and architectural decisions.
- Analysis, planning and review requests do not implicitly authorize implementation.
- Implement the requested scope without unrelated rewrites.
- Work collaboratively and explain algorithms, approaches and significant decisions.
- The user wants to write shaders with agent assistance. Help define inputs/outputs,
  explain the math, debug and review their code. Do not implement complete shaders
  on their behalf without an explicit assignment. Define responsibilities for mixed
  C++/HLSL tasks; carry out explicit requests to modify shaders.
- Writing every subsystem from scratch is not required. Choose dependencies for
  concrete needs. Work proceeds as time permits; do not invent deadlines.
- Explain tradeoffs in image quality, CPU/GPU time, memory and maintenance.
  Do not claim performance improvements without measurements.
- Report changes, completed checks and remaining limitations.

## Repository map

- `engine/BorealEngine.slnx`: main Visual Studio solution.
- `engine/include/engine`: public headers.
- `engine/source`: core, math, platform, render, scene, engine and geometry code.
- `engine/source/render/source/interface/d3d12`: D3D12 backend.
- `engine/shaders`: HLSL.
- `samples/BorealSample1`: sample and its data.
- `engine/third_party`: dependencies; tinygltf uses a submodule.

Inspect current sources and git diff before changes. Historical reviews, README
claims and class names do not prove implemented behavior. Only modify third-party
code when required by the current task.

## Naming

- New directory names must be lowercase throughout the project.
- Rename existing mixed-case directories only as part of a renaming task, updating
  build and code references. Preserve upstream dependency structure.
- Follow module conventions for file names. The directory rule does not require
  renaming files such as `AGENTS.md`.

## Project materials

Start with [agent/README.md](agent/README.md).
- For planning and architecture, consult [ProjectBrief](agent/ProjectBrief.md)
  and [Roadmap](agent/Roadmap.md).
- Read only relevant tasks, decisions and reviews; do not load the entire archive.
- Keep durable rules here, goals and open questions in ProjectBrief, milestones in
  Roadmap and architectural rationale in decisions.
- Track substantial work spanning chats with scope, acceptance criteria, checks,
  limitations and the next step. Do not create a report for every minor edit.
- Update relevant statuses and evidence after milestones. Never promote a proposal
  to an accepted decision or mark unfinished work complete without grounds.
- Documents do not automatically authorize execution; the current request sets scope.

## Architecture and code

- Read [CppStyle](agent/CppStyle.md) before C++ changes. Preserve local conventions
  and module boundaries; do not reformat unrelated code.
- Separate CPU asset import, GPU resource ownership and command recording.
  Avoid repeated asset reads inside drawing passes.
- Make ownership explicit. Resource, descriptor and upload memory reclamation or
  reuse must respect completion of GPU work.
- Check matrix/coordinate conventions in `engine/include/engine/math/conventions.h`.
  Import changes must account for winding, normals, tangents and node transforms.
- Account for formats, linear/sRGB, resource states, synchronization, resize and lifetime.
- Fence values from different queues are not a shared timeline.
- Extend the RHI for concrete requirements; justify large subsystems and dependencies
  through the current task.

## Git workflow

- The agent handles most Git work: inspect status/diffs and prepare and create
  focused local commits for completed tasks within the assigned scope.
- Keep commits small but coherent. Each should cover a meaningful task; neither
  split related changes arbitrarily nor combine independent tasks.
- Review the staged diff and run appropriate checks before committing. Stage explicit
  paths; exclude others' changes, temporary artifacts and logs.
- Local commit authorization does not imply automatic push, rewriting published
  history or deleting user work. Perform those actions only when appropriately requested.

## Build and verification

Current vcxproj files specify C++20, x64 and toolset v145; verify when changing builds.
Use the existing Visual Studio solution. Do not migrate the build in unrelated work.
`scripts/generate_sln.bat` invokes CMake; verify its configuration exists before use.

From Developer PowerShell at the repository root:

```powershell
msbuild engine/BorealEngine.slnx /m /p:Configuration=Debug /p:Platform=x64
msbuild engine/BorealEngine.slnx /m /p:Configuration=Release /p:Platform=x64
```

These are verification commands, not evidence of a successful current build.
Locate MSBuild with vswhere if absent from PATH. Check the sample working directory,
`data/settings.json`, shaders and DXC runtime before launching.

- Build affected C++ code; verify both configurations when configuration differences matter.
- Use the D3D12 debug layer; for upload/barrier/lifetime changes use GPU-based
  validation when runtime access permits.
- Match runtime checks to the change: images, resize, scene load/unload or multiple
  frames. Compilation does not establish visual correctness.
- Add focused tests for nontrivial math, import and resource management. Do not
  create test infrastructure for documentation edits.
- Distinguish runtime verification, source inspection and hypotheses. Incomplete
  checks must not be reported as passed.

## Task modes

These are collaboration conventions, not automatically launched agents.
- Planning: inspect relevant systems, propose small stages, dependencies and
  acceptance criteria, and identify open decisions.
- Implementation: deliver a verifiable change within the current architecture.
- Rendering review: prioritize correctness, GPU lifetime, synchronization, formats
  and regressions; cite code locations and triggering conditions.
- Explanation: explain algorithms and tradeoffs at Render Engineer level; let the
  request determine how much code to provide.
