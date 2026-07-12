# Memory System

> Lightweight, frequently-updated project memory.
> Updated every coding session to keep AI agents in sync with reality.

## Files

| File | Purpose | Update cadence |
|------|---------|----------------|
| `State.md` | Current project snapshot — version, build status, active focus, known issues | Every session |
| `Active.md` | Current implementation checklist — granular tasks for the active feature | As tasks progress |
| `archive/` | Completed design docs and plans moved here after workstream finishes | Per release |

## Relationship to `the-word-docs/`

| Source | Content | Stability |
|--------|---------|-----------|
| `the-word-docs/` | Permanent architecture, module specs, conventions | Stable (updated per release) |
| `memory/` | Live state + active sprint plan | Ephemeral (updated per session) |

## Workflow

1. **Start of session**: Read `State.md` + `Active.md` first
2. **During session**: Check off items in `Active.md` as work completes
3. **End of session**: Update `State.md` with any new decisions or state changes
4. **Workstream complete**: Move plan docs to `memory/archive/` (session-level) or `docs/04-planning/archive/` (permanent design docs)
