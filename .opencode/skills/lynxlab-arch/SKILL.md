---
name: lynxlab-arch
description: Condensed LynxLab architecture, language pool, and roadmap. Use when deciding which component or language a task belongs to, or what the next roadmap item is. The full source of truth is docs/LynxLab-DESIGN.md.
---

# LynxLab architecture (condensed)

Autonomous control plane that operates a distributed AI inference cluster across (simulated) heterogeneous nodes: observes, heals, and re-schedules model layers in real time.

## Components → language
- Telemetry agent — **C** (tiny static binary per node)
- Wire-protocol codec — **C** (shared by agent + daemon)
- SIMD tensor kernels, mmap model loading — **C**
- Acquisition daemon — **C++**
- Control plane API (REST + WebSocket) — **C++**
- Decision engine (rules → policy) — **C++**
- Inference runtime orchestration — **C++**
- Quantization/conversion, adaptive scheduler, policy compiler, CLI, chaos.py — **Python**
- Dashboard / topology / event stream — **TypeScript/React**

## Control loop
telemetry → typed event → decision → action → mutated system → telemetry …

## Roadmap (target order — do not skip)
- v0.1.0 nervous system (agent → daemon over TCP, C codec, SQLite)
- v0.2.0 typed events + TSDB/relational split
- v0.3.0 control plane skeleton + first heal rule (loop closes once)
- v0.4.0 single-node inference runtime (no PyTorch)
- v0.5.0 distributed inference across 2+ nodes
- v0.6.0 self-healing inference (kill node → re-partition → resume)  ← headline
- v0.7.0 React control center + incident mode
- v0.8.0 policy-driven engine
- v0.9.0 adaptive scheduling
- v1.0.0 stable
- post-1.0: replay, causal graph, OTA fleet, semantic classification, multi-cluster

## Testing
All multi-node testing is simulated with Docker (`docker-compose.test.yml` + `chaos.py`): scale containers as nodes, `docker kill`/`pause`, cgroup limits for heterogeneous nodes, `tc netem` for latency/loss, custom networks for partitions. Benchmark RELATIVE behavior, not absolute throughput.
