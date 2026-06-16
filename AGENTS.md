# LynxLab — Agent Rules

This file is loaded by every agent. It defines the non-negotiable project rules.
Read `docs/LynxLab-DESIGN.md` for the full design; it is the source of truth for architecture and the versioned roadmap.

## External file loading
When you encounter a file reference (e.g. `@docs/LynxLab-DESIGN.md`), load it with your read tool on a need-to-know basis. Do not preemptively load everything. When loaded, treat the content as mandatory instructions that override defaults.

## Small-batch output (STRICT — the model times out on large outputs)
The active model fails or times out on large responses. Keep every turn small and incremental. This overrides any urge to finish in one shot.

**Writes per turn**
- Write or edit ONE file per turn. Do not emit multiple files in a single response.
- Cap a single write at ~150 lines / ~6 KB. If a file is larger, create it as a skeleton first, then fill it in over several turns using targeted edits.
- Prefer small targeted edits (replace one function/block) over rewriting a whole file. Never paste an entire file back to change a few lines.
- Generate code in vertical slices: one struct/class/function (with its test) per turn, not an entire module at once.

**Tool calls per turn**
- At most 2–3 tool calls per turn. Make a call, read the result, then continue next turn.
- Never batch many shell or edit calls speculatively in one response.
- Run ONE build or test command at a time and wait for output before the next.
- For multi-file work, plan the file list in one short turn, then create them one per subsequent turn.

**If output is growing large**
- Stop and checkpoint: commit or save what's done, state what's left, and continue next turn.
- Split the increment further. A timed-out turn loses work; a small completed turn never does.
- Never let a single response try to scaffold a whole component, write its tests, and run the build together — that is three or more turns.

## Language pool (STRICT — violations fail review)
- **C** — hot path only: telemetry agent, wire-protocol codec, SIMD tensor kernels, mmap model loading.
- **C++** — core services: acquisition daemon, control plane, inference runtime orchestration, decision engine.
- **Python** — AI/ML + tooling: quantization/conversion, adaptive scheduler, policy compiler, CLI, build glue, the `chaos.py` simulation driver.
- **TypeScript/React** — frontend only.
- No other languages. If a task could go in two, it goes in the one matching the role table in the design doc, never the easiest.

## Scope discipline
- Each day produces ONE small, shippable increment. Small means: compiles, has a test or a runnable demo, and moves exactly one roadmap item forward.
- Never start a roadmap phase before the previous phase's milestone demo runs once.
- Deterministic replay, causal graphs, OTA fleet management, semantic classification are POST-1.0. Do not build them early.
- If the increment is growing large, stop and split it across days.

## Versioning (SemVer MAJOR.MINOR.PATCH)
- Pre-1.0 = unstable; minor bumps may break.
- Daily work is a PATCH bump on a feature branch.
- A completed sprint (2 weeks) produces at most one MINOR bump and a tagged release.
- Tag format: `vMAJOR.MINOR.PATCH`. Tags must match the CHANGELOG and the Notion sprint.

## Git flow
- `main` — stable, tagged releases only. Never commit directly.
- `dev` — integration branch. Daily work merges here via PR.
- `feature/<roadmap-id>-<slug>` — one branch per increment, branched from `dev`.
- Every commit uses Conventional Commits (`feat:`, `fix:`, `chore:`, `docs:`, `test:`, `build:`).
- Daily: branch from `dev`, build the increment, commit, open + merge PR into `dev`. At sprint end, merge `dev` → `main`, tag, release.

## Definition of done (per increment)
1. Builds via CMake / pyproject / Vite as applicable.
2. Has a test or a documented runnable demo.
3. CHANGELOG updated under "Unreleased".
4. Conventional commit on the correct feature branch.
5. PR opened to `dev` with a body linking the Notion task.
6. Notion task moved to Done with the right tags.
7. Produced via small-batch turns (one file / a few tool calls each); no single oversized response.