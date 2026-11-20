# 003 — Decode model base-color textures

Date: 2026-11-20. Status: user confirmed correct model and base-color display.
Authorization: user requested displaying the model's base color.

## Cause and changes

The checked-in tinygltf v3 parser reads image metadata but never decodes image
payloads. Its TINYGLTF3_ENABLE_STB_IMAGE define has no implementation in this revision.
Consequently image dimensions/pixels remained unset and all GPU texture lookups
fell back to white. Decode embedded buffer-view PNG/JPEG data using the existing
stb_image dependency, with bounds checks and RGBA8 output. Relative image files
are also supported. Data URIs and encoded external URI paths are not implemented.

Removed the renderer's arbitrary first-texture substitution. A primitive uses its
material's base-color texture or the white fallback, never an unrelated normal map.
No HLSL changes or new third-party dependencies were required.

## Verification

A native CPU probe linked against the Debug core library loaded the actual
pudge.glb: 10 primitives, 8 materials, 15 decoded textures. Every texture had
positive dimensions, four channels, the expected byte count and nonconstant
pixel data. Every primitive referenced a material with a base-color texture.
The local probe is under engine/build/checks/texturecheck.cpp; it is a temporary,
ignored verification artifact. Debug and Release x64 solution builds passed.

The user subsequently confirmed correct model and base-color display. Shutdown
validation reported LIVE_RESOURCE warnings, tracked in task 004. A clean D3D12
validation run has not yet been confirmed.
Node transforms/skinning, general glTF support, color-space processing and
baseColorFactor handling remain separate limitations of the renderer.
