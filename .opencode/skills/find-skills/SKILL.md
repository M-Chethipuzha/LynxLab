---
name: find-skills
description: How to discover which skills are relevant before starting any task. Use FIRST at the start of every increment to scan available skills and load the ones that match the work (language, layer, build, test, git, sprint).
---

# Find skills

Before building anything, discover and load the skills that match today's task. Skipping this means missing project-specific constraints.

## Procedure
1. **List what's available.** Scan the skills directory (`.opencode/skills/*/SKILL.md` and any global `~/.config/opencode/skills/`). Read only the frontmatter `name` + `description` of each — cheap to scan.
2. **Match to the task.** Pick every skill whose description overlaps the increment's language, layer, or activity. Several may apply at once.
3. **Load fully.** Read the full body of each matched skill before writing code or running commands.
4. **Re-scan on pivot.** If the task changes mid-increment (e.g. you discover it needs a frontend piece too), re-run the match and load the newly relevant skill.

## Matching guide for LynxLab
- Any task → `lynxlab-arch` (which component/language), `git-flow` (branching/commits).
- Planning / sprint boundary → `sprint-cadence`.
- Compiling, running tests, Docker harness → `build-and-test`.
- C/C++ work → `lynxlab-arch` (component map) + `build-and-test` (CMake/ctest).
- Python work → `build-and-test` (venv/pytest).
- Frontend work → `build-and-test` (vite/vitest).

## Rule
Never start step "Build" of the daily loop until find-skills has run and the matched skills are loaded. State which skills you loaded in one line.
