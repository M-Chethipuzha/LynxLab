---
name: git-flow
description: LynxLab branching, commit, PR, tagging, and release rules. Use whenever creating branches, committing, opening or merging PRs, bumping versions, or tagging releases.
---

# Git flow

## Branches
- `main` — stable tagged releases only. Protected. Never commit or push directly.
- `dev` — integration. All daily work lands here via PR.
- `feature/<roadmap>-<slug>` — one per daily increment, branched from `dev`. Example: `feature/v0.1.0-wire-codec-frame`.

## Daily sequence
```
git checkout dev && git pull
git checkout -b feature/<roadmap>-<slug>
# ... build the increment ...
git add -A
git commit -m "<type>(<scope>): <summary>"   # Conventional Commits
git push -u origin HEAD
# open PR feature -> dev, then merge (squash)
git checkout dev && git pull
```

## Conventional Commits
`<type>(<scope>): <summary>` where type ∈ feat|fix|chore|docs|test|build|refactor.
Scope = layer tag (agent, daemon, inference, frontend, harness, ...).
Example: `feat(codec): add length-prefixed frame encoder + decode test`.

## Versioning on commits
- Daily increment = PATCH bump in the relevant manifest (VERSION file / pyproject / package.json) when it changes shippable behavior. Many days are just additive within an unreleased minor — bump PATCH in the "Unreleased" CHANGELOG heading.
- Do NOT tag on daily merges.

## Sprint close (integration day)
```
git checkout main && git pull
git merge --no-ff dev
# finalize CHANGELOG: move Unreleased -> vX.Y.0 with date
git tag -a vX.Y.0 -m "LynxLab vX.Y.0 — <sprint goal>"
git push origin main --tags
```
- MINOR bump for a completed milestone; MAJOR only for breaking control-plane API changes (post-1.0).

## CHANGELOG
Keep `CHANGELOG.md` in Keep-a-Changelog format. Daily work appends under `## [Unreleased]` grouped by Added/Changed/Fixed. Sprint close renames the section to the tagged version with a date.

## Guardrails
- If `main` or `dev` doesn't exist yet, create them (init repo, first commit on `main`, branch `dev` from it).
- Never force-push shared branches.
- One increment = one feature branch = one PR.
