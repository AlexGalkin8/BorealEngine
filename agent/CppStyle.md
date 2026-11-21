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
- Name conversion functions `translate<Source>To<Target>`, for example
  `translateLogLevelToString` for converting an `ELogLevel` to its text label.
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
- Follow [swapchain.h](../engine/include/engine/render/interface/swapchain.h)
  for header spacing. Indent class members with one tab and align access labels
  with the class declaration.
- Use one blank line between include groups, logical groups of class members,
  and access sections. Keep related declarations together and place comments
  immediately above the declaration they describe.
- Use two blank lines before the first namespace after the includes, between
  namespace blocks, and between a forward declaration group and a class
  definition. Use one blank line between the end of a class and its namespace's
  closing brace.
- Use one space after a comma and around binary operators such as `=`.
  In a single-line function body, use one space inside each brace. Use two spaces
  before the namespace closing comment: `}  // namespace render`.
- Short getters and simple functions may be inline on one line.
- Keep out-of-line method definitions in the `.cpp` file in the same order as
  their declarations in the class header (`.h` or `.hpp`).
- Pointer/reference markers attach to the type: `ITexture* texture`, `const Scene& scene`.
- Constructor initializers occupy separate lines with leading `:` and `,`.
- Preserve local include grouping and blank lines. Avoid unrelated sorting/alignment.
- The cpp's own header usually comes first. Local includes use quotes, cross-module
  includes use `<engine/...>`; standard/platform headers form separate groups.
- Keep library headers out of `.h` and `.hpp` files where possible, especially
  third-party and platform SDK headers. Forward-declare types when their complete
  definitions are unnecessary and include implementation-only dependencies in the
  `.cpp`. Include required headers directly when the header needs the complete type.
- In the D3D12 backend, include Windows SDK, D3D12, DXGI, DXC and WRL headers
  only from `d3d12common.hpp`. Use that header when an SDK definition is required.
- Headers use `#pragma once`; namespace closing comments are common.
- Comment only what the code cannot reasonably convey on its own, such as
  intent, invariants, or external API constraints. Avoid restating the code.
- All new or updated comments and documentation must be in English. Commit subjects
  and bodies must also be in English. User-facing conversation remains in Russian.

## Language and API patterns

- Use the existing C++20 baseline, standard containers, `nullptr`, `enum class`,
  `override`, `final` and `= default` where semantically appropriate.
- Small data/descriptor types are structs with public fields and default values;
  classes encapsulate state and expose methods.
- Both `{}` and `= {}` initialization occur; follow surrounding code.
- Spell out short type names explicitly, including pointer and reference types.
  Use `auto` only when spelling out the type would be excessively long or when
  type deduction is necessary. Lambda objects may be declared with `auto`.
- Prefer explicit C++ casts. Existing C-style casts are not a requirement for new code.
- Ownership commonly uses `unique_ptr`; actors also use `shared_ptr`. Raw pointers
  and references may observe objects or serve APIs, but identify the owner.
  Existing leaks are defects, not style conventions.
- Use references for required borrowed objects. Keep nullable or replaceable
  pointers local or private where possible, and handle absence once at the
  operation boundary. Avoid introducing `std::reference_wrapper` or
  `std::optional` when a direct reference or a scoped failure check suffices.
- D3D12 uses `ComPtr`, descriptor wrappers and HRESULT checks. Respect backend
  contracts and completion of GPU work.

## Errors

Use the engine's existing assertion macros to check invariants and caller contracts.
Use `ASSERT` or `ASSERT_MSG` for a diagnostic check, and select the appropriate
`ASSERT_MSG_RETURN` variant when execution must leave the function on failure.
Use the `D3D12_ASSERT` family for D3D12 and DXGI calls that can fail with an
`HRESULT`. Use the appropriate `D3D12_ASSERT_RETURN*` variant when failure
must exit the current function. If the caller can recover, store the `HRESULT`,
check it with `D3D12_ASSERT`, and handle the failure in both Debug and Release.
Handle expected or recoverable failures, such as a window resize that can be
skipped, through the normal error path and log the reason where it is handled.
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
