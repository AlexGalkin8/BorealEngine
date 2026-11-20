# Tasks

| Task | Status |
|---|---|
| [001 — baseline rendering](001-render-baseline.md) | Proposed; implementation not started |
| [002 — restore builds](002-build-fixes.md) | Complete; Debug/Release builds verified, runtime unverified |

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
