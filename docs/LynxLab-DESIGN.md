# LynxLab — Project Design Document

> An autonomous control plane that operates a distributed AI inference cluster across heterogeneous hardware — observing it, healing it, and scheduling model layers in real time.

**Status:** Design / Pre-`v0.1.0`
**Doc version:** 1.0.0
**License:** TBD (MIT or Apache-2.0 recommended)

---

## 1. One-paragraph thesis

LynxLab treats a homelab (or any small cluster) as a living system. A C/C++ acquisition daemon ingests telemetry from every node, stores it in a dual time-series + relational model, and feeds a decision engine that converts raw signal into action. The headline workload it manages is a **distributed LLM inference engine**: a custom C/C++ runtime that splits a quantized model across nodes. When a node degrades or dies, LynxLab detects it, re-partitions the model layers away from the failing node, and inference continues. Python handles the AI/ML layer (quantization, model conversion, the adaptive scheduler), and a TypeScript/React frontend turns the cluster into a live control center. The whole thing ships as static binaries and Docker images.

---

## 2. Language pool (strict — three languages, each with one job)

| Language | Role | Used for | Never used for |
|----------|------|----------|----------------|
| **C** | Hot path | Telemetry agent, wire protocol codec, tensor kernels (SIMD), memory-mapped model loading | Business logic, orchestration |
| **C++** | Core services | Acquisition daemon, control plane, inference runtime, decision engine | ML model math, UI |
| **Python** | AI/ML + tooling | Model quantization/conversion, the adaptive scheduler, the rule/policy compiler, CLI, build glue | Hot-path data handling |
| **TypeScript/React** | Frontend only | Dashboard, control center, live event stream UI | Any backend logic |

> The discipline: if a task could go in two languages, it goes in the one whose role it matches, not the one that's easiest. C and C++ share a build but stay separated by concern — C for the things that must be tiny/fast/portable (agents, codecs, kernels), C++ for everything that benefits from RAII, abstractions, and structure.

---

## 3. Architecture (layered)

```
┌──────────────────────────────────────────────────────────┐
│  Frontend (TypeScript / React)                             │
│  control center · topology graph · live events · incident  │
└───────────────▲────────────────────────────────────────────┘
                │ REST + WebSocket
┌───────────────┴────────────────────────────────────────────┐
│  Control Plane API (C++)                                    │
│  resource mgmt · node registry · health · orchestration     │
└───┬───────────────────────┬───────────────────────┬─────────┘
    │                       │                       │
┌───▼─────────┐   ┌─────────▼──────────┐   ┌────────▼─────────┐
│ Decision    │   │ Storage            │   │ Inference Runtime │
│ Engine (C++)│   │ TSDB + relational  │   │ (C++ orch +       │
│ rules→policy│   │                    │   │  C kernels)       │
└───▲─────────┘   └─────────▲──────────┘   └────────▲─────────┘
    │                       │                       │
┌───┴───────────────────────┴───────────────────────┴─────────┐
│  Acquisition Daemon (C++)  — the "nervous system"           │
│  ingests over TCP/HTTP, normalizes to typed events           │
└───────────────▲──────────────────────────────────────────────┘
                │ wire protocol (C codec)
┌───────────────┴──────────────────────────────────────────────┐
│  Telemetry Agents (C)  — one tiny static binary per node      │
└────────────────────────────────────────────────────────────────┘

Python sits to the side: model conversion/quantization (offline),
the adaptive scheduler (advises the decision engine), policy compiler, CLI.
```

### Closed control loop
`telemetry → typed event → decision → action → mutated system → telemetry …`
The system always converges toward a declared desired state.

---

## 4. Components

**Telemetry Agent (C).** One small static binary per node. Reads `/proc`, GPU stats, service/db health. Emits over a compact binary wire protocol. No dependencies, cross-compiles to ARM (Raspberry Pi) and x86.

**Wire protocol codec (C).** Shared library defining the on-wire frame format and encode/decode. Used by both the agent and the daemon so there's one source of truth.

**Acquisition Daemon (C++).** Accepts connections, decodes frames, normalizes raw metrics into typed events with identity and context, writes to storage, and pushes a live stream to the control plane.

**Storage (TSDB + relational).** Time-series for metrics; relational (start with SQLite, grow to Postgres) for topology, node config, and resource state. v1 is simple on purpose.

**Control Plane API (C++).** The unified interface. Node registry, health tracking, resource provisioning (containers, db instances), and orchestration. Abstracts Docker vs VM vs bare metal. Exposes REST + WebSocket.

**Decision Engine (C++).** Starts rule-based (threshold → action). Evolves to policy-driven: behaviors declared, engine evaluates continuously against state. Consumes advice from the Python adaptive scheduler.

**Inference Runtime (C++ orchestration + C kernels).** Loads a quantized model via mmap, runs the transformer forward pass with C SIMD kernels, splits layers across nodes over the wire protocol, manages KV-cache and batching. This is the managed workload and the headline demo.

**Adaptive scheduler (Python).** Offline/advisory. Learns workload patterns, predicts which node will stall the pipeline, recommends re-partitioning. Talks to the decision engine; never on the hot path.

**Model toolchain (Python).** Converts and quantizes models (INT8/INT4) into the runtime's mmap-friendly format.

**Automation layer.** Idempotent actions (provision, reconfigure, heal) so retries and partial failures stay consistent.

**CLI (Python).** Precise control for power users.

**Frontend (React/TS).** Topology graph, real-time event stream, health indicators, **incident mode**, and a node-kill simulation that visibly demonstrates self-healing inference.

---

## 5. Versioned roadmap (SemVer: MAJOR.MINOR.PATCH)

Pre-1.0.0 = not yet production-stable; minor bumps may break. `v1.0.0` = first stable, self-healing inference demo working end to end.

### Phase A — Close the loop trivially

**`v0.1.0` — Nervous system**
- C telemetry agent (CPU/mem/node-health) → C++ daemon over TCP
- C wire-protocol codec shared by both
- SQLite storage of raw metrics
- `0.1.1` agent cross-compiles to ARM · `0.1.2` reconnect/backoff

**`v0.2.0` — Typed events + storage split**
- Normalize raw metrics → typed events with identity/context
- Add time-series store alongside relational
- `0.2.1` event schema versioning · `0.2.2` retention/compaction

**`v0.3.0` — Control plane skeleton + first loop**
- C++ control plane API: node registry, health
- Decision engine with exactly one rule: unhealthy node → restart via automation layer
- Idempotent action execution
- **Milestone: telemetry → decision → action → re-observation works once.**
- `0.3.1` action audit log · `0.3.2` retry/partial-failure handling

### Phase B — Add the workload

**`v0.4.0` — Single-node inference runtime**
- Python toolchain: quantize a small model to mmap format
- C++ runtime loads it, C SIMD kernels run the forward pass (no PyTorch)
- KV-cache + greedy decode
- `0.4.1` INT4 support · `0.4.2` batching

**`v0.5.0` — Distributed inference**
- Split model layers across two nodes over the wire protocol
- Pipeline the forward pass
- Benchmark: tokens/sec vs single node
- `0.5.1` >2 nodes · `0.5.2` heterogeneous (Pi + GPU box) partitioning

### Phase C — Make the loop about inference (the headline)

**`v0.6.0` — Self-healing inference**
- Decision engine re-partitions layers when a node degrades
- KV-cache migration off the failing node
- **Milestone demo: kill a node live → inference stalls → system heals → resumes.**
- `0.6.1` graceful drain · `0.6.2` recovery-time metrics

**`v0.7.0` — Frontend control center**
- React topology graph, live event stream (WebSocket), health indicators
- Incident mode + node-kill simulation button
- `0.7.1` system score · `0.7.2` historical incident replay view

**`v0.8.0` — Policy-driven engine**
- Declarative desired-state policies replace ad-hoc rules
- Python policy compiler → engine-evaluable form
- `0.8.1` policy dry-run · `0.8.2` conflict detection

**`v0.9.0` — Adaptive scheduling**
- Python scheduler predicts pipeline stalls, advises pre-migration
- Behavioral baselines per node
- `0.9.1` workload pattern learning · `0.9.2` cost/throughput optimization

### Phase D — Stable

**`v1.0.0` — Stable control plane**
- Full closed loop on a real inference workload, self-healing, with frontend, CLI, binaries, and Docker images all shipping. Documented and reproducible.

### Post-1.0 (the buffet — pick deliberately)
- `v1.1.0` deterministic event replay / incident reconstruction
- `v1.2.0` causal event graph (action → network → perf)
- `v1.3.0` fleet lifecycle: OTA agent updates, version pinning, staged rollouts, auto-rollback
- `v1.4.0` semantic signal classification + environment awareness
- `v2.0.0` scale from single machine to a genuinely distributed multi-cluster control plane (breaking API changes)

---

## 6. Build, binaries & Docker

### Build system
- **CMake** drives the C and C++ build (single tree, separate targets for `liblynx-codec` (C), `lynx-agent` (C), `lynxd` daemon (C++), `lynx-control` (C++), `lynx-infer` (C++/C)).
- **Python** packaged with `pyproject.toml` (toolchain, scheduler, CLI).
- **Frontend** built with Vite → static assets.

### Completed-binary process
1. Compile C/C++ targets static where possible (`-static` / musl for the agent) so the agent is a single dependency-free file.
2. Cross-compile the agent for `linux/amd64` and `linux/arm64`.
3. Bundle: `lynxd`, `lynx-control`, `lynx-infer`, the Python wheel, and the built frontend assets into a release tarball per platform.
4. Tag the git release matching the SemVer tag (e.g. `v0.6.0`); CI attaches the tarballs as release artifacts.

### Docker image process
- **Multi-stage builds.** Stage 1 builds C/C++ with CMake; stage 2 builds the frontend; stage 3 is a slim runtime (`debian:slim` or `distroless`) copying only the artifacts.
- One image per service: `lynxlab/daemon`, `lynxlab/control`, `lynxlab/infer`, `lynxlab/frontend`, plus a tiny `lynxlab/agent`.
- **docker-compose** spins up a full local cluster (daemon + control + N infer nodes + frontend) for development and the demo. A separate `docker-compose.test.yml` plus `chaos.py` form the simulation harness — see Section 7.
- Images tagged with the matching SemVer tag and `latest`; multi-arch via `buildx`.

---

## 7. Simulation harness (testing without a fleet)

Owning multiple machines is impractical, so the entire cluster is simulated with Docker. Each container is a node from LynxLab's point of view: it runs the agent, has its own network identity, and can be killed, paused, throttled, or partitioned on command. This covers nearly every scenario in the roadmap on a single host, and is the primary integration-test and demo environment — not just a fallback.

### Scenario mapping

| What's tested | How it's simulated |
|---------------|--------------------|
| Multiple nodes | Multiple containers from the same agent + infer images: `docker compose up --scale infer=5`. The daemon sees N nodes and doesn't know they share a host. |
| Node death (the `v0.6.0` demo) | `docker kill lynx-infer-3` — instant, clean, scriptable. |
| Hung/unresponsive node | `docker pause lynx-infer-3` — a different failure the decision engine should handle differently from a clean death. |
| Heterogeneous hardware | Per-container cgroup limits: `cpus`, `mem_limit`, `cpuset` to pin cores. A "Raspberry Pi" node is a container capped at `cpus: 0.5, mem_limit: 512m`. Tests partitioning across uneven nodes. |
| Network degradation | `tc netem` inside the container (or a sidecar): `tc qdisc add dev eth0 root netem delay 200ms loss 5%`. Proves the pipeline stalls under a slow link and the adaptive scheduler reacts. Pumba wraps netem + kill/pause as a Docker chaos CLI. |
| Network partition | Custom Docker networks: disconnect a container to simulate a node that's alive but unreachable, then reconnect and watch recovery. |

### Honest limits of the simulation

- **Shared kernel and shared physical cores.** Absolute performance numbers (true tokens/sec at scale) are not representative because containers contend for the same hardware. Benchmark *relative* behavior — recovery time, throughput before vs. after healing, scheduler decisions — not headline hardware figures.
- **No true multi-GPU.** One GPU shared across containers behaves differently from several discrete GPUs, so real GPU partitioning can't be fully validated this way.

### Harness components

- **`docker-compose.test.yml`** — brings up the daemon, control plane, frontend, and N infer nodes as one local cluster.
- **`chaos.py`** (Python, fits the CLI/tooling role) — drives scenarios on a reproducible timeline: kill, pause, throttle, inject latency, partition. This script doubles as the **incident-mode backend** and the **CI integration test**, so demos and tests run the same code path.

---

## 8. Scope discipline (read this before coding)

The roadmap contains years of work if everything is pursued at full depth. The single failure mode is building four layers half-way. Rules:

1. **Never start a phase before the previous phase's milestone demo runs once.**
2. **Deterministic replay, causal graphs, OTA fleet management, and semantic classification are all post-1.0.** Designing their data model early is fine; building them before the control loop runs is the over-engineering trap.
3. **Each `v0.x.0` is independently demoable** — it's a portfolio milestone and a blog post on its own.
4. **One name (LynxLab), one repo, one language per role.** Consistency is the signal that you know what you're building.

---

## 9. What makes this a standout portfolio piece

- Real artifact people can clone, `docker compose up`, and watch heal itself.
- Hard, measurable results: tokens/sec, recovery time, cost-per-token under failure.
- Touches systems programming (C kernels, mmap, codecs), distributed systems (the control plane + partitioning), and AI (the from-scratch inference runtime + adaptive scheduler) — at the seams, which is where the hard, interesting problems live.
- The demo tells a story in 30 seconds: kill a node, watch the cluster keep thinking.
