# Doc-First Checklist

> Status: Active | Last Updated: 2026-06-21

**Mandatory steps before writing code.** Skip nothing.

## Before Writing Code

- [ ] Read `00-INDEX.md` to locate relevant docs
- [ ] Read the module doc in `03-modules/<Module>.md` (or the closest existing doc)
- [ ] Read `02-architecture/Data Structures.md` (if touching core types)
- [ ] Read `02-architecture/Coordinate Spaces.md` (if rendering/scrolling)
- [ ] Read `02-architecture/Data Flow.md` (if modifying the pipeline)
- [ ] Check `04-planning/Release Plan.md` for current release scope
- [ ] Check `04-planning/Progress Tracking.md` for current status
- [ ] Check `07-ai-collaboration/Convention Reference.md` for naming/style

## Before Making Structural Changes

- [ ] Update the relevant module doc in `03-modules/` FIRST
- [ ] Update `02-architecture/Architecture Overview.md` if dependency graph changes
- [ ] Update `02-architecture/Data Structures.md` if types change
- [ ] Update `02-architecture/Data Flow.md` if pipeline changes

## After Writing Code

- [ ] Build succeeds: `cmake --build build --parallel`
- [ ] App runs: `./build/theword` (no crashes)
- [ ] Tests pass: `./build/theword_test` (once test target exists)
- [ ] Update `04-planning/Progress Tracking.md` with completion notes
