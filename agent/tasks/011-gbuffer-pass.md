# 011 — Opaque G-buffer pass

The renderer submits `DepthPrepass`, `GBufferPass`, and
`GBufferBaseColorPreview` in that order. The G-buffer pass reuses the depth
attachment with an equal comparison and writes disabled. Both color targets
return to shader-resource state after the pass. The renderer replaces the
depth and color targets together on resize.

| Attachment | Format | Contents |
|---|---|---|
| `GBufferAlbedo` | `RGBA8_UNORM_SRGB` | Base color and opaque coverage in alpha. Shader reads return linear RGB. |
| `GBufferWorldNormal` | `RGBA16_FLOAT` | Signed world-space shading normal. |

The preview reads albedo and encodes it for the existing UNORM back buffer.
The G-buffer samples glTF normal maps as linear RGBA8 using the material's
UV set and scale. It uses imported normals and tangents when present.
Without tangents, the shader reconstructs the tangent basis from
world-position and UV derivatives. Without a usable map or tangent basis,
it writes the mesh normal, falling back to the geometric normal if needed.
MikkTSpace generation, texture transforms, other material channels, and
depth sampling for deferred lighting are future work.

## Verification

- Debug and Release x64 built with a temporary debug-information override
  for the local MSPDB issue.
- The Debug sample passed the resize smoke check with and without RenderDoc.
- RenderDoc XML confirmed the three command lists in submission order,
  the two MRT formats, and the shared depth resource.
- GPU readback confirmed visible output at 1280×720 and 960×540, with no
  D3D12 errors or corruption messages under GPU-based validation.
- On the default `pudge.glb` scene, GPU readback showed 109,885 covered
  pixels in `GBufferWorldNormal` changing when normal maps were disabled.
- A copy of the same model with `TANGENT` attributes removed also changed
  the normal target with maps enabled, exercising the derivative fallback.
