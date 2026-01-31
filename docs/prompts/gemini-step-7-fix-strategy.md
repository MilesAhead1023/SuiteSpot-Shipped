# Step 7: Fix Strategy & Implementation Plan - Complete Diagnostic

## Overview

**Step 7 of 7** - FINAL STEP. Complete **Steps 1-6** first.

This step consolidates all findings into a comprehensive fix strategy.

**Estimated token usage:** ~3000 tokens
**Time to complete:** 15 minutes

## Prerequisites

You should have findings from:
- ✅ Step 1: Baseline analysis (what changed)
- ✅ Step 2: Performance deep dive (FPS killer identified)
- ✅ Step 3: Post-match logic trace (auto-load break point)
- ✅ Step 4: Map selection state (corruption point)
- ✅ Step 5: Function signature verification (API mismatches)
- ✅ Step 6: Collision & race detection (threading issues)

## Task: Create Fix Strategy

### Part 1: Consolidate Root Causes

Based on all 6 steps, identify the EXACT root causes:

```
═══════════════════════════════════════════════════════════════

ROOT CAUSE #1: Frame Rate Regression

Primary cause: [What exactly kills FPS?]
  - Is it missing ImGuiListClipper? [Yes/No]
  - Is it mutex lock during render? [Yes/No]
  - Is it image loading on render thread? [Yes/No]
  - Is it deep copy every frame? [Yes/No]

Location: [File:Line]
Mechanism: [Exactly how does this hurt FPS?]
Evidence: [Reference findings from Step 2]

Severity: ✓ CRITICAL (game unplayable) / ✓ HIGH (noticeable FPS drop)

───────────────────────────────────────────────────────────────

ROOT CAUSE #2: Post-Match Logic Broken

Primary cause: [What fails after match ends?]
  - Is it SetTimeout(0.0f) crash? [Yes/No]
  - Is it useBagRotation hardcoding? [Yes/No]
  - Is it code resolution failure? [Yes/No]
  - Is it settings not read correctly? [Yes/No]

Location: [File:Line]
Mechanism: [Exactly what happens and why it fails?]
Evidence: [Reference findings from Step 3]

Severity: ✓ CRITICAL (auto-load broken) / ✓ HIGH (sometimes fails)

───────────────────────────────────────────────────────────────

ROOT CAUSE #3: Map Selection State Corruption

Primary cause: [What causes wrong map to load?]
  - Is it CVar writes in render loop? [Yes/No]
  - Is it selectedIndex ≠ path sync issue? [Yes/No]
  - Is it stale callback overwriting? [Yes/No]
  - Is it missing callback on CVar change? [Yes/No]

Location: [File:Line]
Mechanism: [How do selection and load state diverge?]
Evidence: [Reference findings from Step 4]

Severity: ✓ CRITICAL (loads wrong map) / ✓ HIGH (selects wrong map)

═══════════════════════════════════════════════════════════════
```

### Part 2: Secondary Issues to Fix

From Steps 5-6, identify secondary issues:

```
Secondary Issue 1: [Issue type]
  Location: [File:Line]
  Impact: [What breaks?]
  Severity: ✓ High / ✓ Medium / ✓ Low

Secondary Issue 2: [Issue type]
  Location: [File:Line]
  Impact: [What breaks?]
  Severity: ✓ High / ✓ Medium / ✓ Low

Secondary Issue 3: [Issue type]
  Location: [File:Line]
  Impact: [What breaks?]
  Severity: ✓ High / ✓ Medium / ✓ Low

[List any others found in verification steps]
```

### Part 3: Fix Priority & Sequence

Determine fix order (critical first):

```
PRIORITY 1 (Critical - Fix First):
  Root cause #[X]: [Brief description]
  Why first: [What breaks if not fixed?]
  Estimated complexity: ✓ Simple / ✓ Medium / ✓ Complex
  Estimated lines of code: [~N lines to change]

PRIORITY 2 (Critical - Fix Second):
  Root cause #[X]: [Brief description]
  Why second: [Dependencies? Prerequisites?]
  Estimated complexity: ✓ Simple / ✓ Medium / ✓ Complex
  Estimated lines of code: [~N lines to change]

PRIORITY 3 (Critical - Fix Third):
  Root cause #[X]: [Brief description]
  Why third: [Why not first/second?]
  Estimated complexity: ✓ Simple / ✓ Medium / ✓ Complex
  Estimated lines of code: [~N lines to change]

SECONDARY FIXES (High priority but not breaking):
  [List in order]
```

### Part 4: Fix Recommendations - Priority 1

**For the FIRST critical issue to fix:**

```
═══════════════════════════════════════════════════════════════
FIX #1: [Name/Title]

Problem: [Restate the issue]
Root Cause: [Location and mechanism]

Solution Overview:
  [Brief description of what to change]

Detailed Fix Strategy:

Option A: [Recommended]
  File: [File:Line]
  Change: [What to do]
  Code pattern:
    Before: [Show current code]
    After: [Show fixed code]
  Why this works: [Explanation]
  Risk: [Any side effects?]

Option B: [Alternative]
  File: [File:Line]
  Change: [What to do]
  Why this works: [Explanation]
  Risk: [Why not recommended?]

Recommended approach: Option A / Option B

Implementation steps:
  1. [Step 1]
  2. [Step 2]
  3. [Step 3]
  ...

Files to modify:
  - [File 1:Line X-Y]
  - [File 2:Line Z-A]

Verification (How to test):
  Test case 1: [Procedure to verify fix]
  Test case 2: [Another test]
  Expected result: [What should happen?]

═══════════════════════════════════════════════════════════════
```

Repeat **Part 4** for Priority 2 and 3.

### Part 5: Fix Interdependencies

Document any dependencies:

```
Fix Dependency Graph:

Fix #1 depends on: [None / Fix #X / External event]
  Why: [Explanation]

Fix #2 depends on: [None / Fix #1 / Fix #X]
  Why: [Explanation]

Fix #3 depends on: [None / Fix #1 / Fix #2 / etc.]
  Why: [Explanation]

Can fixes be applied in parallel? [Yes/No/Partially]
  If yes: [Which ones?]
  If no: [What must go first?]

Critical path: [Fix #X → Fix #Y → Fix #Z]
```

### Part 6: Rollback & Risk Assessment

For each fix, assess risk:

```
FIX #1 Risk Assessment:

Severity of breaking this fix:
  If broken: [What fails?]
  Severity: ✓ CRITICAL / ✓ HIGH / ✓ MEDIUM

Rollback procedure:
  - How to revert? [git revert, manual undo, etc.]
  - Time to rollback: [Minutes?]

Side effects of this fix:
  - Could it break other features? [Yes/No/Which?]
  - Could it cause performance regression? [Yes/No]
  - Could it cause new race conditions? [Yes/No]

Mitigation:
  - Additional testing needed? [Yes/No - what?]
  - New logging to add? [Yes/No - where?]

[Repeat for Fix #2, #3, etc.]
```

### Part 7: Complete Fix Implementation Checklist

```
PRE-IMPLEMENTATION:
  ☐ All root causes identified and documented
  ☐ All secondary issues identified
  ☐ Fix priority and sequence determined
  ☐ Dependencies understood
  ☐ Rollback procedures documented

FIX #1: [Name]
  ☐ Code changes written
  ☐ Builds without errors
  ☐ Verification test case 1 passes
  ☐ Verification test case 2 passes
  ☐ No new compiler warnings
  ☐ No new memory leaks (review code)
  ☐ No introduced race conditions (review)
  ☐ Commit created with message

FIX #2: [Name]
  ☐ Code changes written
  ☐ Builds without errors
  ☐ Verification tests pass
  ☐ No regressions from Fix #1 still visible
  ☐ Commit created

FIX #3: [Name]
  ☐ Code changes written
  ☐ Builds without errors
  ☐ Verification tests pass
  ☐ No regressions from Fix #1 & #2
  ☐ Commit created

POST-IMPLEMENTATION:
  ☐ All three critical issues resolved
  ☐ No new issues introduced
  ☐ Frame rate restored
  ☐ Auto-load working
  ☐ Map selection correct
  ☐ Ready for two-panel browser implementation
```

### Part 8: Summary - Diagnostic Complete

```
DIAGNOSTIC SUMMARY:

Total issues identified: [N]
  - Critical: [X] (breaks functionality)
  - High: [Y] (degrades performance/UX)
  - Medium: [Z] (minor issues)

Root causes:
  1. [Issue]: [Location] → [Fix needed]
  2. [Issue]: [Location] → [Fix needed]
  3. [Issue]: [Location] → [Fix needed]

Fix sequence:
  Step 1: Fix [Name] at [Location]
  Step 2: Fix [Name] at [Location]
  Step 3: Fix [Name] at [Location]

Estimated implementation time: [X hours]
Estimated testing time: [Y hours]

Next action: Begin implementation following the fix recommendations above.
```

## Success Criteria for Step 7

✅ All root causes from steps 1-6 consolidated
✅ Root causes ranked by severity
✅ 3 primary fixes identified and prioritized
✅ Secondary issues documented
✅ Fix dependencies mapped
✅ Implementation steps detailed for each fix
✅ Verification procedures defined
✅ Rollback strategies documented
✅ Risk assessment complete
✅ Implementation checklist created

## Deliverable

Reply with:
1. **Root Cause Consolidation** - The 3 main issues causing all problems
2. **Secondary Issues** - Other problems found (threading, etc.)
3. **Fix Priority & Sequence** - Order to apply fixes
4. **Detailed Fix Recommendations** - For each priority level
5. **Fix Interdependencies** - Which depends on which
6. **Risk Assessment** - What could go wrong
7. **Implementation Checklist** - Step-by-step todo
8. **Summary** - Complete diagnostic status

---

## After Completion

Once you've delivered this complete diagnostic (Steps 1-7), the user will have:
- ✅ Exact root causes identified
- ✅ Specific line numbers for each fix
- ✅ Complete implementation strategy
- ✅ Verification procedures
- ✅ Risk mitigation

**This diagnostic is now COMPLETE and ready for implementation.**

The two-panel browser plan can proceed AFTER these 3 critical issues are resolved.

---

**CRITICAL:** This is the last diagnostic step. Be comprehensive and precise. Every detail matters for implementation.
