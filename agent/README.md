# BorealEngine project materials

Durable instructions live in [agent/AGENTS.md](AGENTS.md).
All agent configuration stays in this directory. Start agent sessions with this
directory as the working directory: Claude Code then loads CLAUDE.md (which
imports AGENTS.md) and .claude/settings.json, and `claude.bat` does this for you.
Codex finds AGENTS.md here the same way. If an agent was started elsewhere,
explicitly ask it to read agent/AGENTS.md before starting work.
This directory preserves project context and substantial task results across chats.
It does not contain automatically launched agents. Write all materials in English.

| Document | Purpose |
|---|---|
| [ProjectBrief.md](ProjectBrief.md) | Confirmed goals, constraints and open questions |
| [Roadmap.md](Roadmap.md) | Proposed milestones and acceptance criteria |
| [CppStyle.md](CppStyle.md) | C++ conventions and local exceptions |
| [ChatWorkflow.md](ChatWorkflow.md) | Development, independent review and devlog roles |
| [decisions](decisions/README.md) | Accepted architectural decisions and rationale |
| [tasks](tasks/README.md) | Tasks, verification and continuation points |
| [reviews](reviews/README.md) | Significant source reviews |

## Workflow

1. Read agent/AGENTS.md and the current request.
2. For planning, read ProjectBrief and Roadmap. For local changes, read only the
   relevant task, decisions and sources.
3. Verify historical statements against current code and git diff. Old reports
   do not establish the current build state or whether a defect remains.
4. After substantial work, update the task with changes, checks, remaining work
   and a concrete next step. Link actual evidence.

Do not load every historical task/review on each request. Do not duplicate rules
in agent profiles. Add skills and test workflows when recurring needs emerge.

## Where rules belong

- General development, naming and language rules: `agent/AGENTS.md`.
- Subsystem-specific rules: a nested `AGENTS.md` near that subsystem when needed;
  do not copy general rules.
- Product goals/constraints: ProjectBrief.
- Architectural rationale: decisions.
- Single-task constraints: its document or the current request.

New directory names must be lowercase, as required by agent/AGENTS.md.
