# 017 — Standardize CRLF line endings

- Date: 2026-11-23
- Status: planned
- Authorization: user request to record the migration task

## Outcome and scope

Use CRLF in Windows working copies of tracked project text files and LF in
the Git index. Cover source, shaders, project files, configuration, scripts,
and documentation. Preserve binary assets and leave submodule contents to
their own repositories. Identify any files whose consumers require LF and
document those exceptions explicitly.

## Plan

1. Inventory tracked files and their line endings with `git ls-files --eol`.
   Verify that Git and the migration script classify binary files correctly.
2. Add root `.gitattributes` and `.editorconfig` rules so checkout, staging and
   editor saves agree. Use `text=auto eol=crlf` as the Git default for text.
3. Run `git add --renormalize .` in a clean worktree and refresh tracked text
   files to CRLF. Preserve encodings, BOMs, final-newline state and content.
4. Review the staged diff for line-ending-only changes and keep the migration
   in its own commit.

## Acceptance criteria

- A fresh Windows checkout reports `i/lf w/crlf` for nonempty project text
  files, except documented format requirements.
- Saving representative C++, HLSL and project files in Visual Studio does not
  introduce mixed endings or an unexpected Git diff.
- Binary assets retain identical hashes and submodule revisions are unchanged.
- Debug x64 builds and the sample launches from the normalized checkout.
