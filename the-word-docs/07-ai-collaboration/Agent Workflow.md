# Agent Workflow

> Status: Active | Last Updated: 2026-06-21

## Principles

1. **Read before you write**: Before touching any code, read the relevant module doc in `the-word-docs/03-modules/`.
2. **Doc-first changes**: Propose changes to docs before implementing code.
3. **Single responsibility**: One class per file, one module per directory.
4. **No magic**: Every design decision should be documented.

## Standard Workflow

### 1. Understand the Task
- Read `00-INDEX.md` to find relevant docs
- Read the module doc(s) for the area you're working on
- Read `02-architecture/Data Structures.md` for core types
- Read `02-architecture/Coordinate Spaces.md` if rendering or input

### 2. Plan the Change
- Check `04-planning/Progress Tracking.md` for current status
- Check `04-planning/Roadmap.md` for timeline
- Propose approach in a comment or doc update

### 3. Implement
- Follow `07-ai-collaboration/Convention Reference.md` for naming and structure
- Follow `07-ai-collaboration/Doc-First Checklist.md` before writing code
- Use `doctest` for unit tests

### 4. Verify
- Build: `cmake --build build --parallel`
- Run: `./build/theword`
- Run tests: `./build/theword_test` (once test target exists)

### 5. Document
- Update the relevant module doc in `03-modules/`
- Update `04-planning/Progress Tracking.md`
- Update `04-planning/Roadmap.md` if scope changed

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
