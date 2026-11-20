# Architectural decisions

| Decision | Status |
|---|---|
| [001 — deferred pipeline](001-deferred-pipeline.md) | Accepted by the user on 2026-11-20 |
| [002 — ImGui editor](002-imgui-editor.md) | Accepted by the user on 2026-11-20 |
| [003 — resource ownership](003-resource-ownership.md) | Accepted by the user on 2026-11-20 |

Accepted direction does not mean completed implementation. Details beyond a decision's
scope remain open. Record significant choices with alternatives: pipeline, ownership,
scene format and subsystem boundaries. Minor local edits need no separate decision.
File name: `NNN-short-topic.md`.

## Template

```markdown
# NNN — Decision title

Date:
Status: proposed / accepted / superseded
Acceptance basis: user instruction or authority in a specific task
Related task:

## Context and requirements
Concrete problem, constraints and verifiable requirements.

## Alternatives
Reasonable options and implications for quality, time, memory and maintenance.

## Decision
Chosen approach and rationale. Clearly label proposals as not yet accepted.

## Consequences and verification
Tradeoffs, affected interfaces and verification approach.

## Revisit conditions
New requirements or measurements that would justify changing the choice.
```

When superseding an accepted decision, preserve history and link its replacement.
