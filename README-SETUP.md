# LynxLab — OpenCode Automation Setup

This directory configures OpenCode to build LynxLab as a daily, sprint-driven autonomous workflow: multiple specialized agents, on-demand skills, project memory, a Git flow (main/dev/feature), and Notion sprint tracking.

## What's here
```
opencode.json                 # agents, MCP (Notion), instructions
AGENTS.md                     # project memory / rules loaded by every agent
docs/LynxLab-DESIGN.md        # full design + roadmap (source of truth)
.opencode/
  agent/                      # (optional) markdown agent overrides
  skills/
    find-skills/SKILL.md      # discover & load relevant skills before building
    lynxlab-arch/SKILL.md     # condensed architecture + roadmap
    sprint-cadence/SKILL.md   # 2-week sprints, daily sizing, Notion structure
    git-flow/SKILL.md         # branching, commits, tagging, releases
    build-and-test/SKILL.md   # CMake/ctest, pytest, vitest, Docker harness
  command/
    daily.md                  # the daily one-shot, runnable as /daily
```

## Agents
- **orchestrator** (primary, default) — runs the daily loop, delegates, does git + Notion.
- **planner** (primary, read-only) — breaks roadmap into small increments.
- **core-engineer** (subagent) — C/C++ only.
- **ai-engineer** (subagent) — Python only.
- **frontend-engineer** (subagent) — TypeScript/React only.
- **reviewer** (subagent, read-only) — enforces language pool, scope, SemVer.
- **scribe** (subagent) — Notion + CHANGELOG + commit/PR messages.

Memory = `AGENTS.md` + `docs/LynxLab-DESIGN.md`, both wired into `instructions` so every agent loads them. Skills load on demand by description match.

---

## SETUP CHECKLIST — do all of this BEFORE pasting the one-shot

1. **Install OpenCode** and confirm it runs: `opencode --version`.

2. **Authenticate a model provider.** `opencode auth login` and add your provider credentials. The config sets no per-agent model, so every agent uses your configured default model. If you want a specific agent on a different model, add a `"model"` field to that agent in `opencode.json`.

3. **Place these files at your repo root.** `opencode.json`, `AGENTS.md`, the `.opencode/` folder, and `docs/LynxLab-DESIGN.md` all go in the project root. Open OpenCode from that directory so it auto-loads the config.

4. **Initialize git with the branch model.**
   ```
   git init
   git add -A && git commit -m "chore: bootstrap LynxLab opencode config"
   git branch -M main
   git checkout -b dev
   ```
   Create the GitHub remote and push both branches. Protect `main` (and ideally `dev`) in GitHub settings so only PRs can merge.

5. **Install the GitHub CLI** (`gh auth login`) so the orchestrator can open and merge PRs from the terminal. Without it, the agent can push branches but you'll merge PRs manually.

6. **Set up the Notion MCP connection.** The config points at the remote Notion MCP (`https://mcp.notion.com/mcp`). Run OpenCode once and complete the OAuth/connect flow when it prompts, or follow OpenCode's MCP auth step for remote servers. Confirm the `notion` server shows as connected.

7. **Prepare Notion.** In the workspace you connected, create (or let the agent create on Day 1) two databases: **LynxLab Sprints** and **LynxLab Tasks**, with the task properties listed in the `sprint-cadence` skill (Status, Sprint relation, Layer, Language, Roadmap, Type, Branch). Grant the Notion integration access to the parent page.

8. **Decide your toolchains exist** for what the agent will build: a C/C++ compiler + CMake, Python 3.11+ with a venv, Node + a frontend bundler, and Docker + docker-compose for the simulation harness. The agent will assume these are installable/available.

9. **Dry-run once in plan mode.** Start OpenCode, switch to the `planner` agent, and ask it to summarize the v0.1.0 plan. This verifies config, memory, and skills load without making any changes.

10. **Optional — schedule it.** To make it run "each day" unattended, wrap the one-shot in a cron job / Task Scheduler entry calling `opencode run` with the command. Start by running it manually for a few days first.

---

## THE ONE-SHOT PROMPT

You can either run the packaged command:
```
/daily
```
…or paste this prompt to the **orchestrator** agent (equivalent, self-contained):

> Run one LynxLab daily increment end to end. Obey AGENTS.md strictly.
>
> 1. Discover skills FIRST: use the find-skills skill to scan all available skills and load the relevant ones. Always load lynxlab-arch, sprint-cadence, and git-flow. State which skills you loaded. Re-scan if the task pivots.
> 2. Orient: check `git log --oneline -10`, the current branch, the CHANGELOG Unreleased section, and the active sprint in Notion. If no sprint is active, set one up first — create the Notion sprint page and ~10 tagged task cards for the next roadmap milestone, then proceed.
> 3. Pick exactly ONE small increment that meets the Definition of Done. State it in one short paragraph with its roadmap/layer/language tags. Use @planner if scope is unclear.
> 4. From up-to-date `dev`, create `feature/<roadmap>-<slug>` (create main/dev first if missing).
> 5. Load the build-and-test skill, then build via the right specialist by language: @core-engineer for C/C++, @ai-engineer for Python, @frontend-engineer for TS/React. Enforce the strict language pool.
> 6. Verify per build-and-test before commit: C/C++ → `cmake --build` + `ctest` green (agent/codec cross-compile for arm64, sanitizers clean); Python → `pytest` + `ruff` green; frontend → `vite build` + `vitest` green; multi-node behavior → bring up `docker-compose.test.yml`, run the relevant `harness/chaos.py` scenario (kill/pause/netem/partition), assert recovery, tear down. Add a test or runnable demo for the new behavior. Report RELATIVE metrics only.
> 7. Have @reviewer (read-only) confirm language-pool discipline, small scope, that build+tests ran, correct SemVer/CHANGELOG, and no premature post-1.0 work. Fix blocking issues.
> 8. Have @scribe update CHANGELOG Unreleased, write the Conventional Commit message and PR body linking the Notion task; then commit and push the feature branch.
> 9. Open the PR into `dev` and squash-merge it, then return to `dev`.
> 10. Have @scribe move the Notion task to Done with the PR URL, commit SHA, and tags (Layer, Language, Roadmap, Type, Branch).
> 11. If today is the 10th working day of the sprint, run sprint close: merge `dev` → `main`, finalize the CHANGELOG to vX.Y.0, tag `vX.Y.0`, push tags, write the Notion retro, and roll unfinished tasks forward.
>
> Keep it to a single small increment — if it grows large, ship only the first slice today and split the rest into new task cards. Never push directly to main or dev except the sprint-close merge. End with a 3-line summary: what shipped (with which skills loaded), the branch/PR, and the Notion card status.

---

## Notes on the cadence
- **Sprint = 2 weeks (10 working days).** One MINOR milestone per sprint, one tag at close.
- **Daily = one small increment**, PATCH-level, merged to `dev` via PR.
- **Sprint close = MINOR bump + tag + release notes + Notion retro.**
- The reviewer is your guardrail against the biggest risk here: an agent quietly making each "small" increment too big or drifting off the language pool. Keep it in the loop.
