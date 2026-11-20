# 002 — Minimal Dear ImGui level editor

Date: 2026-11-20. Status: accepted.
Basis: the user selected a minimal custom ImGui editor.
Related milestone: M5 in [Roadmap](../Roadmap.md).

## Context and choice

The baseline must support level creation and adding objects. Options discussed
were a custom editor, Blender and JSON editing. A custom Dear ImGui editor was
selected; external tools may remain sources of models.

## Scope and consequences

Proposed initial scope: viewport, object list, add/remove models, transform inspector,
and level save/load. This breakdown is an agent proposal; expand it in separate tasks.
UI/input/render integration, object identity and serialization are needed.
Editor code placement, level format, gizmos and UI layout remain open.
Editor dependencies should have a clear boundary with runtime code.

## Verification and revisit conditions

Create a level, add and modify objects, save, restart the application and restore
the result. Verify input and viewport resize. Revisit if the user changes the
level-authoring approach or tooling requirements expand substantially.
The editor has not been implemented as part of this documentation task.
