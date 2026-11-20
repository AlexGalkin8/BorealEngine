# Code reviews

No standalone reviews have been saved yet. Initial stabilization observations are
in [task 001](../tasks/001-render-baseline.md) and require rechecking.
Save significant reviews useful to later work as `YYYY-MM-DD-short-topic.md`.
Do not rewrite old findings as though later fixes existed at review time;
link the fixing task instead.

## Template

```markdown
# Review: scope

Date, commit/branch and uncommitted changes:
Scope:
Method: source inspection / build / execution / visual inspection / profiling

## Findings
For each: priority, file/symbol, triggering conditions, consequences, evidence,
confidence and proposed fix. Separate hypotheses and architectural preferences
from confirmed defects.

## Verification and limitations
Completed checks, unexecuted checks and open questions.

## Follow-up
Links to concrete tasks. Confirm fixes through their verification results.
```
