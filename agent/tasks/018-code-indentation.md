# 018 — Standardize C++ and HLSL indentation

- Date: 2026-11-23
- Status: planned
- Authorization: user request to record the migration task

## Outcome and scope

Normalize block indentation across all tracked project C++ source and header
files and HLSL source and include files. Use one tab per level in C++ with
a tab width of four columns. Use four spaces per level in HLSL. Spaces may
be used for alignment within lines. Preserve third-party code and generated
files.

## Plan

1. Inventory tracked C++ and HLSL files, including conditional code and shader
   includes. Identify mixed indentation and continuation alignment.
2. Add file-type sections to the root `.editorconfig` so Visual Studio inserts
   tabs for C++ and spaces for HLSL. Coordinate with
   [Task 017](017-crlf-line-endings.md) to preserve the CRLF policy.
3. Normalize leading indentation with a standalone whitespace change.
   Preserve continuation alignment, preprocessor directives, and whitespace
   that is part of string or other literal values.
4. Commit the formatting separately from functional changes.

## Acceptance criteria

- All in-scope files follow the indentation rule for their language.
- Saving representative C++ and HLSL files in Visual Studio retains the
  configured indentation.
- The migration diff contains only the intended indentation changes.
- Debug x64 builds and the sample launches with its default shaders.
