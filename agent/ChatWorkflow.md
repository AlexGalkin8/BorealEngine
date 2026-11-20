# Chat responsibilities and handoffs

## Main development chat

Own planning, architecture, implementation, integration and Git commits. This is
the primary coordinator. Update relevant task documents with scope, commit IDs,
checks and remaining limitations. Keep shader responsibilities from AGENTS.md.
Request independent review against a specific commit/range or clearly identified
working-tree snapshot. Do not treat unreviewed work as reviewed.

## Independent review chat

Inspect AGENTS.md, the relevant task, actual diff and owning code independently.
Do not rely on the author's summary as proof of correctness. Prioritize actionable
defects, triggering scenarios, regressions, resource lifetime and synchronization.
Report severity, file/symbol, consequence and evidence; separate hypotheses and
design preferences. Record the reviewed revision and verification limitations.
Do not modify production code, stage changes or create commits unless reassigned.
Save substantial reports under agent/reviews when requested; report fixes back
to the main chat rather than silently applying them.

## Devlog and X publishing chat

Prepare English development notes and X posts from commits, task results and actual
screenshots/captures. Explain the problem, approach, visible result and next step.
Distinguish implemented, tested, visually verified and planned work. Never invent
images, performance figures or completed features. Keep a technical but personal
tone without inflated claims. Do not edit engine code or commit shared changes.

Keep substantial drafts and publication records under agent/devlog. Each record
should identify source commits, verification, media, draft text and status
(draft / ready / published), plus the actual post URL after publication.
The devlog role includes publishing to X when requested. Before the first post,
establish the intended account and publication workflow with the user; creating
this chat does not itself request publication of an unspecified post. Do not report
a draft as published. If direct posting is unavailable, provide a ready-to-post
draft and state the limitation. Do not schedule recurring posts unless requested.

## Shared working directory

Use repository documents as durable handoffs, not assumed access to other chats'
history. Only the main chat owns shared code and Git by default. Reviews should
record the inspected commit to avoid confusion if development continues. Do not
run concurrent builds into shared output directories. Background chats must wait
for a concrete assignment rather than starting an unsolicited implementation,
review or publication.
