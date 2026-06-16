---
description: Run one LynxLab daily increment end to end — discover skills, plan, build, test, git flow, Notion update.
agent: orchestrator
---

# LynxLab Daily Increment

You are running the LynxLab autonomous daily build. Execute the full loop for ONE small increment. Follow AGENTS.md strictly.

## Steps

1. **Discover skills (do this first).** Use the `find-skills` skill to scan every available skill (`.opencode/skills/*/SKILL.md` and global skills) and load the ones relevant to today's work. Always load `lynxlab-arch`, `sprint-cadence`, and `git-flow`. State in one line which skills you loaded. Re-scan and load more if the task pivots mid-increment (e.g. it turns out to need a frontend piece).

2. **Orient.** Read current state: `git log --oneline -10`, current branch, `CHANGELOG.md` Unreleased section, and the active sprint in Notion (via the notion MCP). Identify the active sprint's target roadmap version and which sub-items remain.

3. **Pick today's increment.** Delegate to @planner if scope is unclear. Choose ONE small item that satisfies the Definition of Done in AGENTS.md. State the chosen item with its roadmap/layer/language tags in one short paragraph. If no sprint is active, run the Day-1 flow from `sprint-cadence` first (create the Notion sprint page + ~10 tagged task cards), then pick Day 1's increment.

4. **Branch.** From up-to-date `dev`, create `feature/<roadmap>-<slug>`. Create `main`/`dev` first if they don't exist (see `git-flow`).

5. **Build.** Load the `build-and-test` skill, then delegate to the right specialist by language:
   - C/C++ (daemon, control plane, inference, codec, agent) → @core-engineer
   - Python (toolchain, scheduler, policy compiler, CLI, chaos.py) → @ai-engineer
   - TypeScript/React (dashboard, topology, event stream) → @frontend-engineer
   Enforce the strict language pool from AGENTS.md.

6. **Test/verify (per `build-and-test`).** The increment is not done until the matching path passes:
   - Backend C/C++ → `cmake --build` + `ctest` green; agent/codec cross-compile clean for arm64; sanitizers clean for memory-touching code.
   - Python → `pytest` + `ruff` green, with a new test for new behavior.
   - Frontend → `vite build` + `vitest` green against mocked backend.
   - Multi-node behavior → bring up `docker-compose.test.yml`, run the relevant `harness/chaos.py` scenario, assert the expected recovery/behavior, then tear down. Report RELATIVE metrics only.
   Add at least one test or a documented runnable demo for the new behavior.

7. **Review.** Delegate to @reviewer (read-only): confirm language-pool respected, scope genuinely small, build+tests actually run, SemVer/CHANGELOG correct, no premature post-1.0 work. Address blocking feedback.

8. **Document + commit.** Delegate to @scribe: update `CHANGELOG.md` Unreleased, write the Conventional Commit message and the PR body linking the Notion task. Commit on the feature branch and push.

9. **PR → dev.** Open the PR to `dev` and squash-merge it. Return to `dev`.

10. **Notion.** @scribe moves the task card to Done with PR URL + commit SHA, and sets tags (Layer, Language, Roadmap, Type, Branch).

11. **Sprint boundary check.** If today is the 10th working day of the sprint, run sprint close (see `git-flow` + `sprint-cadence`): merge `dev` → `main`, finalize CHANGELOG to `vX.Y.0`, tag, push tags, write the Notion retro, roll unfinished tasks forward.

## Rules
- find-skills runs before Build; build-and-test loads before any compile/test command.
- Exactly one increment. If it grows large, stop and split it; commit only the small first slice today.
- Never push directly to `main` or `dev` (except the sprint-close merge to `main`).
- End with a 3-line summary: what shipped (with which skills loaded), the branch/PR, and the Notion card status.
