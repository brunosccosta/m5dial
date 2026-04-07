# Claude Code Instructions

## Before starting any task

1. Read `MEMORY.md` (auto-memory index) and any relevant memory files under `.claude/` — check for feedback, project context, and workflow rules.
2. Read `docs/architecture.md` — understand the system before touching it.
3. Read `docs/learnings.md` — avoid repeating past mistakes.
4. Read `docs/TODO.md` — know what's planned and how features are expected to behave.
5. For significant features, check for a working doc at `docs/<feature>.md`.

## Workflow rules

- Never make file changes without proposing the plan and getting explicit approval first.
- Always branch off `main` before starting work. Main uses squash merge commits only.
- Always propose the commit message and get explicit approval before running `git commit`.
- Keep `docs/architecture.md`, `docs/learnings.md`, and `agents.md` up to date as part of every feature — not after.
- For significant features, create `docs/<feature>.md` as a scratchpad at the start; delete when done.
- Ask before launching research agents — propose what to look up first.
