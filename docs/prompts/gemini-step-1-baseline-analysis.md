# Step 1: Baseline Analysis - Git History & Changed Files

## Overview

This is **Step 1 of 7** in a comprehensive diagnostic session. Complete this step fully before moving to Step 2.

**Goal:** Understand exactly what changed in recent commits that caused the performance and logic regressions.

**Estimated token usage:** ~2000-3000 tokens
**Time to complete:** 10-15 minutes

## Context

Read **`GEMINI.md`** for project architecture (especially sections on performance, threading, and rendering).

## Task: Git Analysis & Regression Timeline

### Part 1: Current State

Run these commands and document the **exact output**:

```bash
git status
git log --oneline -10
git log --graph --oneline -15
```

**Deliverable:** Show current branch, modified files, and last 10-15 commits in order.

### Part 2: Identify Regression Point

Find when things broke:

```bash
git log --oneline --all | head -20
git log --format="%h %ai %s" -10
```

**Key question:** What is the MOST RECENT commit? When did the frame rate regression appear?

### Part 3: Deep Diff Analysis

For EACH file modified in the last 3 commits, run:

```bash
git diff HEAD~3 HEAD -- WorkshopDownloader.cpp
git diff HEAD~3 HEAD -- TrainingPackUI.cpp
git diff HEAD~3 HEAD -- SettingsUI.cpp
git diff HEAD~3 HEAD -- AutoLoadFeature.cpp
git diff HEAD~3 HEAD -- SuiteSpot.cpp
```

**Deliverable:** For each file with changes, document:
- **Lines added/removed**
- **New code patterns introduced**
- **Deleted patterns (if any)**
- **Critical changes** (mutex usage, render loops, SetTimeout, CVar access)

### Part 4: Look for Risky Changes

Specifically search for:

```bash
# Lines that were deleted (potential cause of logic bugs)
git diff HEAD~3 HEAD --word-diff-regex='[^\s]' | grep -E "^\[-.*-\]"

# Lines that were added (potential cause of performance issues)
git diff HEAD~3 HEAD --word-diff-regex='[^\s]' | grep -E "^\{\+.*\+\}"

# Changes to SetTimeout calls
git diff HEAD~3 HEAD | grep -A 3 -B 3 "SetTimeout"

# Changes to mutex usage
git diff HEAD~3 HEAD | grep -A 3 -B 3 "std::mutex\|lock_guard"

# Changes to render functions
git diff HEAD~3 HEAD | grep -A 3 -B 3 "Render()"
```

**Deliverable:** List any suspicious patterns:
- SetTimeout calls removed
- Mutexes added/removed
- Render loops changed
- New blocking operations

### Part 5: File-by-File Summary

Create a table showing WHAT CHANGED:

| File | Lines Changed | Type of Change | Risk Level |
|------|---|---|---|
| WorkshopDownloader.cpp | [+30, -5] | [New/Modified/Deleted] | High/Medium/Low |
| TrainingPackUI.cpp | [+0, -0] | No changes | - |
| ... | | | |

For each file with changes, note:
- **What was added?** (new logic)
- **What was removed?** (deleted logic)
- **Why was it changed?** (commit message)
- **Risk assessment** (could this cause the 3 issues?)

### Part 6: Identify Root Cause Candidates

Based on the diffs, list **candidate causes** for each issue:

**Issue 1: Frame Rate Kill (Workshop Browser)**
- Candidate cause 1: [File:Line] - [Description]
- Candidate cause 2: [File:Line] - [Description]
- Candidate cause 3: [File:Line] - [Description]

**Issue 2: Post-Match Logic Broken**
- Candidate cause 1: [File:Line] - [Description]
- Candidate cause 2: [File:Line] - [Description]
- Candidate cause 3: [File:Line] - [Description]

**Issue 3: Map Selection State Corruption**
- Candidate cause 1: [File:Line] - [Description]
- Candidate cause 2: [File:Line] - [Description]
- Candidate cause 3: [File:Line] - [Description]

## Success Criteria for Step 1

✅ Git history clearly shows which commits changed what
✅ Diff analysis identifies all code changes in last 3 commits
✅ High-risk changes flagged (SetTimeout, mutexes, render loops)
✅ Candidate causes listed for each of the 3 critical issues
✅ Commit messages explain the context

## Next Step

Once you've completed this analysis, reply with your findings. Then I'll provide **Step 2: Performance Analysis Deep Dive** which will examine the frame rate regression in detail.

---

**Do NOT skip any part of this step.** Token usage is low because we're gathering raw data. Be comprehensive and precise. Each finding here will guide the next 6 steps.
