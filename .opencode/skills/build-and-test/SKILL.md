---
name: build-and-test
description: How to build and test each LynxLab layer — CMake for C/C++ backend, pytest for Python, vitest for frontend, and the Docker simulation harness for integration. Use whenever compiling, writing or running tests, or validating an increment before commit.
---

# Build and test

Every increment must build and be verified before commit. Use the path that matches the layer.

## C / C++ backend (daemon, control plane, inference, codec, agent)
- Build via the single CMake tree at repo root.
  ```
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
  cmake --build build -j
  ```
- Unit tests with CTest (back it with a lightweight framework, e.g. Catch2/doctest for C++, a tiny assert harness for C):
  ```
  ctest --test-dir build --output-on-failure
  ```
- Each new C/C++ component ships at least one CTest target. The agent and codec must also cross-compile clean for `linux/arm64` (verify with the toolchain file before claiming done).
- Run with sanitizers in Debug for memory-touching code: `-fsanitize=address,undefined`.

## Python (toolchain, scheduler, policy compiler, CLI, chaos.py)
- Work inside the project venv; deps in `pyproject.toml`.
  ```
  python -m venv .venv && . .venv/bin/activate
  pip install -e ".[dev]"
  pytest -q
  ruff check .   # lint
  ```
- New Python code ships a pytest test. The scheduler is advisory-only — test its recommendations, never let tests depend on it being on the hot path.

## Frontend (React/TS)
  ```
  cd frontend
  npm ci
  npm run build      # vite build must pass
  npm run test       # vitest
  ```
- Components that consume live data are tested against a mocked WebSocket/REST, not a running backend.

## Integration — Docker simulation harness
This is how multi-node behavior is tested without real machines.
- Bring up the simulated cluster:
  ```
  docker compose -f docker-compose.test.yml up --build --scale infer=3 -d
  ```
- Drive scenarios with the Python chaos driver (kill / pause / throttle / latency / partition):
  ```
  python harness/chaos.py kill   lynx-infer-2
  python harness/chaos.py pause  lynx-infer-1
  python harness/chaos.py netem  lynx-infer-3 --delay 200ms --loss 5%
  python harness/chaos.py partition lynx-infer-2
  ```
- For the v0.6.0 self-heal milestone, the integration test asserts: inference stalls on kill, the decision engine re-partitions, and generation resumes — measured by recovery time, not absolute throughput.
- Tear down: `docker compose -f docker-compose.test.yml down -v`.

## Definition of "verified" before commit
- The relevant build command exits 0.
- The relevant test command passes (and a new test was added for new behavior).
- For increments touching multi-node behavior, the harness scenario runs and the assertion holds.
- Benchmark numbers, when reported, are RELATIVE (before/after, recovery time) — never presented as real-hardware throughput, since containers share one kernel.
