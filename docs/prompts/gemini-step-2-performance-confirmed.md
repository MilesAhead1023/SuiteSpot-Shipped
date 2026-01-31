# Step 2: Performance Analysis - Deep Dive Into FPS Kill

## Overview

**Step 2 of 7** - Building on your baseline analysis from Step 1.

You've identified three **critical performance candidates**. This step drills into each one with surgical precision.

**Estimated token usage:** ~3000 tokens
**Time to complete:** 15 minutes

## Confirmed Candidates (From Step 1)

```
Issue 1: Frame Rate Kill (Workshop Browser)

Candidate 1: SettingsUI.cpp:715 (approx)
- RLMAPS_RenderSearchWorkshopResults locks mutex and deep copies entire result list every frame

Candidate 2: SettingsUI.cpp:725 (approx)
- Lazy loading of preview images on render thread during loop

Candidate 3: SettingsUI.cpp:755 (approx)
- Search results loop lacks ImGuiListClipper, rendering dozens of cards every frame
```

## Task: Confirm Performance Killer

### Part 1: Read & Document Workshop Render Function

**File:** `SettingsUI.cpp` - Find the `RLMAPS_RenderSearchWorkshopResults()` function.

Document the EXACT code:

```
Function: RLMAPS_RenderSearchWorkshopResults()
Location: SettingsUI.cpp:[Start Line] - [End Line]

Code structure:
1. Mutex lock line: [Show the exact lock_guard line]
2. Data copy line: [Show where cachedResultList is populated]
3. Loop structure: [Show the for loop that renders items]
4. Image loading: [Show where images are loaded]
5. End of function: [Show where lock releases]

Total lines in function: [X]
Total mutex lock duration: [Approximately how many lines are inside the lock?]
```

### Part 2: Measure Render Cost - Candidate 1 (Mutex + Deep Copy)

**Question:** How expensive is the deep copy every frame?

For each map in the result list, what data is copied?

```bash
# Look at RLMAPS_MapResult structure
grep -A 20 "struct RLMAPS_MapResult\|struct.*MapResult" SettingsUI.h MapList.h
```

**Deliverable:**
```
RLMAPS_MapResult structure size:
- Members: [List all members]
- Estimated size per item: [Rough byte estimate]
- Number of results in list: [When user searches, typically how many results?]
- Copy cost per frame: [N items × M bytes = Total bytes copied per frame]
- At 60 FPS, copies per second: [Total bytes/sec]
- Conclusion: Is this significant? Yes/No/Why
```

### Part 3: Measure Render Cost - Candidate 2 (Image Loading)

**Question:** When and where are images loaded?

```bash
# Find image loading in workshop render
grep -n "std::make_shared<ImageWrapper>\|image\|texture\|Image(" SettingsUI.cpp | head -20
```

**Deliverable:**
```
Image Loading in Workshop Browser:

Location 1: [SettingsUI.cpp:Line X]
- What triggers image load? [On demand? Every frame?]
- Is it on render thread? [Yes/No - blocking?]
- How many images loaded per frame? [N items × Y FPS = total loads/sec]
- Impact: Does this block rendering?

[Repeat for each image load location]

Conclusion: Is image loading the killer?
- If async: Acceptable
- If blocking: Critical issue
```

### Part 4: Measure Render Cost - Candidate 3 (No ImGuiListClipper)

**Question:** How many items render every frame?

```bash
# Find the loop structure in RLMAPS_RenderSearchWorkshopResults
grep -n "for\|while" SettingsUI.cpp | grep -A 2 -B 2 "result"
```

**Deliverable:**
```
Workshop Search Results Loop:

Loop location: SettingsUI.cpp:[Start] - [End]
Loop structure: [Show the exact for/while statement]

Items per search: [Typical search results: 10? 50? 100+?]
Work per item:
  - ImGui::Selectable() or similar: [Line number]
  - Image rendering: [Line number]
  - Text rendering: [Line number]
  - Total work per item: [Estimated microseconds]

Total work per frame (with N results):
  N items × [work per item] = [Total microseconds]

Virtual scrolling check:
  - Is ImGuiListClipper used? [Yes/No]
  - If no, ALL items render every frame = Major issue

Estimated FPS impact:
  - 60 FPS = 16.67ms per frame budget
  - This loop uses: [X]ms
  - Remaining for game logic: [16.67 - X]ms
  - Verdict: Can game even run at 60 FPS?
```

### Part 5: Identify THE Performance Killer (Singular)

Based on measurements above, which is the PRIMARY cause?

```
The ONE THING killing FPS:

Primary killer: [Candidate 1/2/3 or combination?]
Evidence: [Your measurements showing why]
Impact: [How many microseconds this adds per frame?]
Threshold: [At what point does FPS drop below 60?]

Secondary contributors:
- [Issue 2]
- [Issue 3]
```

### Part 6: Reference - Training Pack Browser (Working)

Compare to a **working** example. How does `TrainingPackUI.cpp` handle 2000+ packs?

```bash
# Find the training pack render loop
grep -n "ImGuiListClipper\|for.*RLTraining\|filteredPacks" TrainingPackUI.cpp | head -30
```

**Deliverable:**
```
Training Pack Browser (Why does it work?):

List size: 2000+ packs
Loop structure: [Show ImGuiListClipper pattern]

Items rendered per frame: [Only visible items, typically 10-15]
Virtual scrolling: [Yes - ImGuiListClipper]

FPS impact: Minimal (60 FPS sustained)

Key difference vs Workshop:
[What does training pack do that workshop doesn't?]
```

### Part 7: Root Cause Statement

Write your final diagnosis:

```
FRAME RATE REGRESSION ROOT CAUSE:

Primary mechanism:
[Explain the exact mechanism causing FPS drops]

Location: [File and line number(s)]

Why it wasn't a problem before:
[What changed in recent commits?]

Evidence:
1. [Measurement 1]
2. [Measurement 2]
3. [Measurement 3]

Fix direction:
[Based on your analysis, what needs to change?]
- Option A: [What to remove/optimize]
- Option B: [Alternative fix]
- Option C: [Another option]
```

## Success Criteria for Step 2

✅ All three candidates measured with concrete numbers
✅ Mutex lock duration quantified
✅ Data copy cost measured
✅ Image load mechanism identified
✅ ImGuiListClipper presence/absence confirmed
✅ Comparison to working example shows the difference
✅ Primary killer identified (singular focus)
✅ Root cause statement with evidence chain

## Deliverable

Reply with:
1. **Candidate 1 Analysis** - Mutex + copy costs
2. **Candidate 2 Analysis** - Image loading mechanism
3. **Candidate 3 Analysis** - Loop rendering cost
4. **Comparative Analysis** - Training pack browser reference
5. **Root Cause Statement** - The one thing killing FPS

## Next Step

Once performance is diagnosed, **Step 3: Post-Match Logic Trace** will investigate why auto-load is broken.

---

**Critical focus:** Don't approximate. Use the actual code measurements. The more precise your analysis, the more obvious the fix will be.
