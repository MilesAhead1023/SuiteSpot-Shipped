# SuiteSpot Documentation Hub

Welcome! This is your entry point for all SuiteSpot documentation.

## Quick Navigation by Role

### 👨‍💻 **Developers (Start Here)**
- **New to the project?** → `architecture/CLAUDE_AI.md` (5 min read - core constraints)
- **Ready to code?** → `development/DEVELOPMENT_GUIDE.md` (workflows & patterns)
- **Need to fix something?** → `sessions/2025-12-27/UI_BUG_FIXES.md` (recent fixes with patterns)
- **Looking up APIs?** → `reference/API/` (GameWrapper, ServerWrapper, etc.)

### 🤖 **AI Agents (Claude, Gemini, Copilot)**
- **Claude Code:** `agents/CLAUDE.md` (your entry point)
- **Gemini:** `agents/GEMINI.md` (your rules)
- **Copilot:** `agents/COPILOT.md` (your template)

### 🏗️ **Architecture & Design**
- **Design decisions:** `architecture/DECISIONS.md` (7 ADRs)
- **Core constraints:** `architecture/CLAUDE_AI.md` (threading, null checks, file I/O)
- **System overview:** `architecture/overview.md` (coming soon)

### ⚙️ **Development Workflows**
- **Build & test:** `development/DEVELOPMENT_GUIDE.md`
- **IDE setup (Clangd/LSP):** `development/IDE_SETUP.md`
- **CI/CD & releases:** `development/CI_CD.md` (coming soon)

### 📚 **Reference Documentation**
- **API docs:** `reference/API/` (GameWrapper, ServerWrapper, CarWrapper, etc.)
- **Code structure:** `reference/CodeMap.md`
- **BakkesMod template:** `reference/BakkesMod-Template/`

### 🎨 **Features (Implementation Guides)**
- **Post-match overlay:** `features/Overlay/`
  - Implementation guide: `features/Overlay/IMGUI_OVERLAY_FIX_GUIDE.md`
  - Quick reference: `features/Overlay/QUICK_REFERENCE.md`
  - RocketStats integration: `features/Overlay/RocketStats/INTEGRATION_PATTERNS.md`

### 📋 **Session Archives (Work Logs)**
- **Latest session (2025-12-27):** `sessions/2025-12-27/`
  - Summary: `SESSION_SUMMARY.md`
  - Status report: `FINAL_STATUS_REPORT.md`
  - Issue fixes: `UI_BUG_FIXES.md`
- **Previous sessions:** `sessions/2024-12-19/`

### 📦 **Archive (Historical, Reference Only)**
- **Deprecated/historical docs:** `archive/`
- Use when you need historical context or patterns from prior work

---

## Documentation Map

```
docs/
├── README.md                          ← You are here
├── architecture/
│   ├── CLAUDE_AI.md                  ← MANDATORY: Core reference
│   ├── DECISIONS.md                  ← Design decisions (7 ADRs)
│   └── overview.md                   ← Architecture diagram (coming)
├── development/
│   ├── DEVELOPMENT_GUIDE.md          ← Workflows & common tasks
│   ├── IDE_SETUP.md                  ← Clangd/LSP configuration
│   └── CI_CD.md                      ← Build & release (coming)
├── reference/
│   ├── API/                          ← GameWrapper, ServerWrapper, etc.
│   ├── CodeMap.md                    ← Code structure
│   └── BakkesMod-Template/           ← Generic template (reference)
├── features/
│   └── Overlay/
│       ├── IMGUI_OVERLAY_FIX_GUIDE.md ← Implementation reference
│       ├── QUICK_REFERENCE.md         ← Quick lookup
│       └── RocketStats/               ← Integration patterns & reference
├── agents/
│   ├── CLAUDE.md                     ← Claude Code rules (NEW)
│   ├── GEMINI.md                     ← Gemini rules
│   └── COPILOT.md                    ← Copilot template (NEW)
├── sessions/
│   ├── 2025-12-27/                   ← Latest work log
│   │   ├── SESSION_SUMMARY.md
│   │   ├── FINAL_STATUS_REPORT.md
│   │   └── UI_BUG_FIXES.md
│   └── 2024-12-19/                   ← Previous sessions
└── archive/                           ← Historical docs (reference)
    ├── AI_OPTIMIZATION_GUIDE.md
    ├── BUILD_TROUBLESHOOTING.md
    └── [11 more historical files]
```

---

## Common Questions (with Answers)

### "How do I build the project?"
→ See `development/DEVELOPMENT_GUIDE.md` → "Build & Test" section
```powershell
msbuild SuiteSpotv2.0\SuiteSpot.sln /p:Configuration=Release /p:Platform=x64
```

### "What are the critical constraints?"
→ See `architecture/CLAUDE_AI.md` → "Critical Constraints" section
- Threading: All game state via `SetTimeout()`
- Wrappers: Never store across frames
- CVars: Use `SetCVarSafely<T>()` template
- File I/O: Always check `is_open()` and `fail()`

### "How do I fix a bug?"
→ See `development/DEVELOPMENT_GUIDE.md` → "Fix a Bug" workflow
→ Reference `sessions/2025-12-27/UI_BUG_FIXES.md` for pattern examples

### "Where are the API docs?"
→ See `reference/API/` → Find your class (GameWrapper, ServerWrapper, etc.)

### "What was done in the last session?"
→ See `sessions/2025-12-27/FINAL_STATUS_REPORT.md`

### "What's pending?"
→ See `sessions/2025-12-27/FINAL_STATUS_REPORT.md` → "What's Next" section

### "How do I access CVars safely?"
→ See `sessions/2025-12-27/UI_BUG_FIXES.md` → Issue #5
→ Pattern: Use `SetCVarSafely<T>("name", value)` with triple-layer null checks

### "Why was decision X made?"
→ See `architecture/DECISIONS.md` → Find ADR number
→ Each ADR explains rationale, alternatives, consequences

---

## For AI Agents

**Your entry point:**
- Claude Code → `agents/CLAUDE.md`
- Gemini → `agents/GEMINI.md`
- Copilot → `agents/COPILOT.md`

Each agent file explains:
- Where to find authoritative information
- Critical constraints you must follow
- How to reference documentation
- Common workflows and paths

---

## Maintaining This Documentation

After each development session:
1. Update `sessions/YYYY-MM-DD/` with new work
2. Update `architecture/DECISIONS.md` if new decisions
3. Update `development/DEVELOPMENT_GUIDE.md` if new patterns
4. Archive old work to `archive/` as needed

---

**Last Updated:** 2025-12-27
**Status:** ✅ Complete and organized
**Next Steps:** See `sessions/2025-12-27/FINAL_STATUS_REPORT.md` → "What's Next"

