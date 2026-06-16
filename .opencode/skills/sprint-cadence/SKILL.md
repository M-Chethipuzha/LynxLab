---
name: sprint-cadence
description: How LynxLab runs 2-week sprints, daily increments, and the Notion board. Use whenever planning the day's work, opening or closing a sprint, or deciding how big today's increment should be.
---

# Sprint cadence

## Sprint = 2 weeks (10 working days)
- Each sprint targets exactly ONE `v0.x.0` minor milestone from the roadmap, or a meaningful slice of one if the milestone is large.
- Sprint produces at most one MINOR version bump and one git tag at close.
- Daily work is a PATCH-level increment merged to `dev`.

## Daily increment sizing
A good increment: compiles, has a test or runnable demo, moves exactly one roadmap sub-item forward, and is reviewable in a few minutes. If it's bigger than that, split it.

Examples of one day's work:
- Define the wire-protocol frame struct in C + a unit test.
- Add reconnect-with-backoff to the agent.
- Add one decision-engine rule + a simulated trigger test.
- Wire one WebSocket event type into the React event stream.

## Sprint structure
- **Day 1**: planner agent drafts the sprint goal + breaks it into ~10 daily tasks. scribe creates/updates the Notion sprint page and task cards with tags.
- **Days 2–9**: one increment per day (the daily one-shot).
- **Day 10**: integration day — merge `dev` → `main`, MINOR bump, tag `vX.Y.0`, release notes, sprint retro note in Notion, roll any unfinished tasks to next sprint.

## Notion structure
- Database: **LynxLab Sprints** (one page per sprint) and **LynxLab Tasks** (cards).
- Task properties: `Status` (Backlog / Todo / In Progress / Review / Done), `Sprint` (relation), `Layer` tag, `Language` tag, `Roadmap` tag (e.g. v0.3.0), `Type` tag (feat/fix/chore/docs/test/build), `Branch`.
- Each Done task links its PR URL and commit SHA.

## Tags vocabulary
- Layer: `agent`, `codec`, `daemon`, `storage`, `control-plane`, `decision-engine`, `inference`, `scheduler`, `frontend`, `harness`, `ci`.
- Language: `c`, `cpp`, `python`, `ts`.
- Type: `feat`, `fix`, `chore`, `docs`, `test`, `build`.
- Roadmap: the target version, e.g. `v0.2.0`.
