# BorealEngine

Durable project instructions are shared with other agents (for example Codex)
and live in AGENTS.md next to this file.

@AGENTS.md

## Claude Code notes

- Claude Code sessions start in the agent/ directory. The repository root is
  the parent directory. Paths such as `engine/...`, `samples/...` and
  `scripts/...` in AGENTS.md are relative to the repository root; build and Git
  commands run from there.
- Treat AGENTS.md as authoritative. Do not duplicate its rules here.
- Read README.md for the document map. Load tasks, decisions and reviews only
  when they are relevant to the current request.
- Read CppStyle.md before any C++ change.
- Never run `git commit` or `git push` without explicit user approval
  (see "Git workflow" in AGENTS.md).
- Development, independent review and devlog roles follow ChatWorkflow.md.
  Each role runs in its own session; a session waits for a concrete assignment.
