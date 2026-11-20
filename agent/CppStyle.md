# BorealEngine C++ style

Recorded on 2026-11-20 from scene, render, math, core and platform code.
The repository is not entirely uniform: camera uses camelCase for some variables,
while parts of the importer/platform use spaces instead of tabs. Preserve local
conventions and do not rewrite the project solely for consistency.

## Naming

- Types: PascalCase (`StaticMeshActor`, `MaterialHandle`, `D3D12Buffer`).
- Interfaces: `I` prefix (`IBuffer`, `IRenderDevice`).
- Scoped enums usually use `E` (`EBufferUsage`); enumerators use `kVertex`.
  Preserve existing exceptions, such as math/conventions.
- Functions/methods: lowerCamelCase (`createBuffer`, `setLookAt`).
- Default for new locals, parameters and struct fields: snake_case (`render_device`).
  Private fields use a trailing underscore (`mesh_slots_`). Extend existing types
  using their local convention.
- Named constants: `kPascalCase`, typically `constexpr` or `static constexpr`.
- Namespaces: lowercase (`render`, `scene`, `math`). Macros: UPPER_SNAKE_CASE.
- Public headers typically use `.h`, internal headers `.hpp`, implementations `.cpp`.
  File names are typically lowercase without separators (`d3d12buffer.cpp`).

## Formatting

- Predominant style: tabs; opening braces of classes, functions and multiline
  control blocks on a separate line. Namespace braces stay on the declaration line.
- Short getters and simple functions may be inline on one line.
- Pointer/reference markers attach to the type: `ITexture* texture`, `const Scene& scene`.
- Constructor initializers occupy separate lines with leading `:` and `,`.
- Preserve local include grouping and blank lines. Avoid unrelated sorting/alignment.
- The cpp's own header usually comes first. Local includes use quotes, cross-module
  includes use `<engine/...>`; standard/platform headers form separate groups.
- Headers use `#pragma once`; namespace closing comments are common.
- All new or updated comments and documentation must be in English. Commit subjects
  and bodies must also be in English. User-facing conversation remains in Russian.

## Language and API patterns

- Use the existing C++20 baseline, standard containers, `nullptr`, `enum class`,
  `override`, `final` and `= default` where semantically appropriate.
- Small data/descriptor types are structs with public fields and default values;
  classes encapsulate state and expose methods.
- Both `{}` and `= {}` initialization occur; follow surrounding code.
- Use `auto` for iterators, factories and obvious types; keep explicit types when
  they clarify data representation or API contracts.
- Prefer explicit C++ casts. Existing C-style casts are not a requirement for new code.
- Ownership commonly uses `unique_ptr`; actors also use `shared_ptr`. Raw pointers
  and references may observe objects or serve APIs, but identify the owner.
  Existing leaks are defects, not style conventions.
- D3D12 uses `ComPtr`, descriptor wrappers and HRESULT checks. Respect backend
  contracts and completion of GPU work.

## Errors

The code uses `ASSERT`, `ASSERT_MSG_RETURN*`, `D3D12_ASSERT_RETURN*` and early returns.
Exceptions also occur in shader compilation and parsing. There is no established
blanket prohibition on exceptions; do not introduce one implicitly.
Preserve an API's error-reporting contract. External data and loading failures
must be handled in Release too: inspect macro definitions instead of relying on
debug-only assertions. Change project-wide error policy in a separate task.

## Reference examples

- [Actors/components](../engine/include/engine/scene/actor.h)
- [Buffer interface](../engine/include/engine/render/interface/buffer.h)
- [D3D12 buffer](../engine/source/render/source/interface/d3d12/d3d12buffer.cpp)
- [Camera naming exceptions](../engine/source/scene/source/camera.cpp)
- [Math templates](../engine/include/engine/math/vector.h)

This document does not introduce an automatic formatter or a line-length limit.
