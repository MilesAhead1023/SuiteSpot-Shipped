# SuiteSpot Documentation

## Quick Start

| Need | Go To |
|------|-------|
| Build the project | `development/DEVELOPMENT_GUIDE.md` |
| Understand the codebase | `reference/codemap.md` |
| BakkesMod API reference | `api/` folder |
| AI agent context | Root `CLAUDE.md` (auto-loaded) |

## Directory Structure

```
docs/
├── agents/           # AI agent configs (Copilot, Gemini)
├── api/              # BakkesMod API reference
│   ├── *-api.md           Wrapper classes (GameWrapper, CarWrapper, etc.)
│   ├── *-component.md     Components (Boost, Dodge, Jump, etc.)
│   └── *.instructions.md  Detailed usage guides
├── architecture/     # Architecture decisions
├── development/      # Development & deployment guides
│   ├── DEVELOPMENT_GUIDE.md
│   ├── deployment-guide.md
│   ├── plugin-creation.md
│   └── BUILD_TROUBLESHOOTING.md
├── features/         # Feature-specific documentation
│   └── Overlay/      # Post-match overlay (RocketStats integration)
└── reference/        # Project reference
    ├── codemap.md         Project structure map
    ├── architecture.md    System architecture
    └── SUMMARY.md         Feature summary
```

## Key Files

| File | Purpose |
|------|---------|
| `CLAUDE.md` (root) | Auto-loaded context for Claude Code sessions |
| `api/gamewrapper-api.md` | Core game state access |
| `api/thread-safety.instructions.md` | Threading constraints |
| `development/DEVELOPMENT_GUIDE.md` | Build and development workflows |
| `reference/codemap.md` | File locations and purposes |

## For AI Agents

The main context file is `CLAUDE.md` at the project root (not in docs/). It contains:
- Project overview and critical constraints
- Build commands
- Key CVars and event hooks
- Common task patterns

Additional agent configs in `docs/agents/`:
- `COPILOT.md` - GitHub Copilot context
- `GEMINI.md` - Google Gemini context

## Build Command

```powershell
msbuild SuiteSpot.sln /p:Configuration=Release /p:Platform=x64
```

## Critical Constraints

1. **Threading**: Access game state only via `SetTimeout()` callbacks
2. **Wrappers**: Never store across frames - get fresh each time
3. **Null checks**: Always verify wrappers before use
4. **CVars**: Use safe access patterns
5. **File I/O**: Always check `is_open()` and `fail()`

See `CLAUDE.md` for complete constraint details.

---
**Last Updated:** 2025-12-28
