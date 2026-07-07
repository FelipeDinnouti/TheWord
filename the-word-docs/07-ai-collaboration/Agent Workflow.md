# Agent Workflow

> Status: Active | Last Updated: 2026-07-05

## Principles

1. **Read before you write**: Before touching any code, read the relevant module doc in `the-word-docs/03-modules/`.
2. **Read project memory**: Check `memory/STATE.md` for current project snapshot and `memory/ROADMAP.md` for the active release checklist.
3. **Doc-first changes**: Propose changes to docs before implementing code.
4. **Single responsibility**: One class per file, one module per directory.
5. **No magic**: Every design decision should be documented.

## Standard Workflow

### 1. Understand the Task
- Read `memory/STATE.md` — current project state, version, known issues
- Read `memory/ROADMAP.md` — active release checklist
- Read `00-INDEX.md` to find relevant docs
- Read the module doc(s) for the area you're working on
- Read `02-architecture/Data Structures.md` for core types
- Read `02-architecture/Coordinate Spaces.md` if rendering or input

### 2. Plan the Change
- Check `04-planning/Release Plan.md` for the current release's scope
- Check `04-planning/Progress Tracking.md` for historical progress
- Write the implementation plan to `memory/ROADMAP.md` with a granular checklist
- Propose approach in a comment or doc update

### 3. Implement
- Follow `07-ai-collaboration/Convention Reference.md` for naming and structure
- Follow `07-ai-collaboration/Doc-First Checklist.md` before writing code
- Use `doctest` for unit tests
- Tick off items in `memory/ROADMAP.md` as tasks complete
- Update `memory/STATE.md` with any new state or decisions

### 4. Verify
- Build: `cmake --build build --parallel`
- Run: `./build/theword`
- Run tests: `./build/theword_test`

### 5. Document & Release
- Update the relevant module doc in `03-modules/`
- Update `04-planning/Release Plan.md`
- Update `04-planning/Progress Tracking.md`
- Bump version in `CMakeLists.txt` and tag per [Versioning](../../AGENTS.md#versioning) conventions

## Communication

When proposing changes, use this format:
```
## Proposal
[What you want to change]

## Rationale
[Why it should change]

## Impact
[What other modules/docs need updating]

## Implementation Plan
[Rough steps]
```
