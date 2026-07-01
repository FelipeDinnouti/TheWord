# TheWord Documentation Index

> Start here. This index tells you what to read based on what you're doing.

## Quick Navigation

| If you're... | Read this first |
|---|---|
| New to the project | `01-vision/Project Specification.md` → `02-architecture/Architecture Overview.md` |
| Understanding data sources | `02-architecture/Data Source Architecture.md` |
| Implementing a feature | `03-modules/<Module>.md` → `02-architecture/Data Structures.md` |
| Planning cross-module work | `02-architecture/Architecture Overview.md#cross-cutting-concerns` |
| Fixing a bug | `02-architecture/Data Flow.md` → relevant `03-modules/` doc |
| Setting up the build | `06-ops/Build Guide.md` → `06-ops/Environment Setup.md` |
| Integration testing | `06-ops/Integration Testing.md` |
| An AI agent | `04-planning/Release Plan.md` → `07-ai-collaboration/Agent Workflow.md` → `07-ai-collaboration/Doc-First Checklist.md` |
| Planning work | `04-planning/Release Plan.md` → `04-planning/Development Plan.md` → `04-planning/Progress Tracking.md` |
| Using the API | `03-modules/Bible API.md` → `05-reference/YouVersion API.md` |
| Rich text / segments | `02-architecture/Data Structures.md#segment` → `03-modules/Text Layout Engine.md` |
| Font rendering / crispness | `05-reference/Raylib Notes.md#crisp-font-rendering--directives` |
| UI design / navigation model | `02-architecture/UI Philosophy.md` → `03-modules/UI Layer.md` |

## Document Map

```
the-word-docs/
├── 00-INDEX.md                         ◀ YOU ARE HERE
├── 01-vision/                          # Why this project exists
│   ├── Project Specification.md        # Core vision, motivation, design goals
│   └── Product Requirements.md         # Feature requirements and constraints
├── 02-architecture/                    # How it works
│   ├── Architecture Overview.md        # Four-layer architecture, module dependency graph
│   ├── Data Source Architecture.md     # Dual-source design: ChapterProvider, USFM vs API
│   ├── Data Structures.md              # Core types: Word, Span, Segment, ChapterData, Highlight
│   ├── Coordinate Spaces.md            # Document space vs Screen space
│   ├── Data Flow.md                    # End-to-end request pipeline
│   └── UI Philosophy.md                # Navigation model, screens, bottom bar, design principles
├── 03-modules/                         # Module specifications
│   ├── Core.md                         # Config, APIClient, EnvLoader
│   ├── Bible API.md                    # BibleClient: HTML parsing, ChapterProvider impl
│   ├── USFM Parser.md                  # USFM → ChapterData (Phase 5)
│   ├── Text Layout Engine.md           # LayoutEngine: segment-aware layout, word wrap
│   ├── Document Manager.md             # DocumentManager: ChapterProvider-based, dual source
│   ├── Highlighting System.md          # Highlighter spec (Phase 7)
│   ├── Persistence.md                  # SQLite persistence spec (Phase 8)
│   └── UI Layer.md                     # Renderer + Input spec (Phase 9)
├── 04-planning/                        # Project management
│   ├── Development Plan.md             # Full 10-phase development plan
│   ├── Progress Tracking.md            # Current status, what's done, what's next
│   └── Roadmap.md                      # Phase order with acceptance criteria
├── 05-reference/                       # External context
│   ├── USFM Format.md                  # USFM format reference
│   ├── YouVersion API.md               # API endpoints, HTML format, Bible versions
│   └── Raylib Notes.md                 # Raylib patterns and conventions used
├── 06-ops/                             # Operations
│   ├── Build Guide.md                  # Build and run instructions
│   ├── Environment Setup.md            # Platform-specific setup
│   ├── Integration Testing.md          # xdotool-based GUI automation
│   └── Troubleshooting.md              # Common issues and fixes
└── 07-ai-collaboration/               # Agent/AI workflows
    ├── Agent Workflow.md               # How agents approach tasks
    ├── Doc-First Checklist.md          # Mandatory steps before writing code
    └── Convention Reference.md         # Naming, file structure, coding patterns
```

## Quick Reference Links

- **Build commands**: See `06-ops/Build Guide.md`
- **Coding conventions**: See `07-ai-collaboration/Convention Reference.md`
- **Core data structures**: See `02-architecture/Data Structures.md`
- **Data source architecture**: See `02-architecture/Data Source Architecture.md`
- **Phase plan**: See `04-planning/Development Plan.md`
- **Current progress**: See `04-planning/Progress Tracking.md`
- **UI philosophy**: See `02-architecture/UI Philosophy.md`
