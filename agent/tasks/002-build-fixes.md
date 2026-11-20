# 002 — Restore Debug and Release builds

Date: 2026-11-20. Status: complete (build verification only).
Authorization: user requested fixing build errors. Related milestone: M0.

## Changes

- Implement the missing ITexture uploadPixels contract for newly created,
  single-mip RGBA8 textures. Copy rows using D3D12 footprints, transition from
  COPY_DEST to PIXEL_SHADER_RESOURCE, and wait for GPU completion before releasing
  temporary upload resources. Imported textures use the same initialization path.
- Define ASSERT_FAIL in Release consistently with other diagnostic-only assertions.
- Complete Release D3D12 return macros, fix the undefined cond reference and accept
  variadic debug names. Debug HRESULT checks now evaluate expressions only once.
- Supply the sample's engine/D3D libraries in Release and use configuration-aware paths.

## Verification

Both Debug and Release x64 solution builds completed with exit code 0 using
Visual Studio 18 Community MSBuild, /m:1 /nr:false /verbosity:minimal.
Commands: MSBuild engine/BorealEngine.slnx /p:Configuration=Debug (or Release)
/p:Platform=x64, with the flags above.
Executables were produced under samples/BorealSample1/build/bin/x64/{Debug,Release}.
The sandboxed Debug link failed with LNK1101/MSPDB140.DLL; the same build succeeded
outside the sandbox. The sandboxed Release link was stopped and retried outside
the sandbox successfully. No toolchain reinstall was required.

## Limitations

Application launch, image correctness and GPU-based validation were not performed.
Upload is intentionally synchronous and limited to one-time RGBA8 initialization;
it is not a streaming/update API. Broader M0 ownership, resize and upload-error
propagation work remains. No shaders were modified. An unrelated README.md change
appeared during the task and is excluded from this change.

Next: run the sample under validation and inspect the texture path before claiming
visual correctness or completion of M0.
