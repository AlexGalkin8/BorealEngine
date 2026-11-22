# Tasks

| Task | Status |
|---|---|
| [001 — baseline rendering](001-render-baseline.md) | Proposed; implementation not started |
| [002 — restore builds](002-build-fixes.md) | Complete; Debug/Release builds verified, runtime unverified |
| [003 — base-color textures](003-base-color-textures.md) | User confirmed correct display; shutdown warning tracked in 004 |
| [004 — renderer resource lifetime](004-render-resource-lifetime.md) | Complete. User verified sample shutdown on 2026-11-21. |
| [005 — window resize](005-window-resize.md) | Builds and resize smoke check passed. Visual and D3D12 Output checks pending. |
| [006 — engine logging](006-engine-logging.md) | Implemented. Release resize diagnostics verified. |
| [007 — depth prepass](007-depth-prepass.md) | Implemented for opaque meshes. Resize, GPU readback and D3D12 validation passed. |
| [008 — command-line arguments](008-command-line-arguments.md) | Implemented. Parser and sample startup verified. |
| [009 — RenderDoc capture](009-renderdoc-launch.md) | In-process capture and pass order verified. |
| [010 — refactor capture-tool startup](010-capture-tool-refactor.md) | Planned. |
| [011 — opaque G-buffer pass](011-gbuffer-pass.md) | Implemented. Capture and GPU readback verified. |

Document substantial tasks that benefit from continuity across chats, not every
minor edit. File name: `NNN-short-topic.md`. Keep the table current.

## Template

```markdown
# NNN — Task title

Date:
Status: proposed / planned / in progress / blocked / complete / cancelled
Related roadmap milestone:
Authorization: user request

## Outcome and scope
Observable result; included and excluded work.

## Acceptance criteria
- Verifiable completion conditions.

## Plan
- Small steps and dependencies.

## Completed work
Actual changes only; links to code and decisions.

## Verification
Version/commit and uncommitted changes, environment, configuration, commands,
expected/actual results and existing logs/screenshots.
Use passed / failed / blocked / not-run. Use visual-unverified when the image
was not observed. Never invent evidence paths.

## Limitations and continuation
Remaining work, any blocking reason and a concrete next step.
```

A task document does not authorize implementation. Completion requires acceptance
criteria to be met or scope explicitly revised; unavailable verification is not success.
