# VehicleOS-RT: MVP Build Steps for AI Agents (container + QEMU)

**Purpose:** Provide deterministic, step-by-step instructions so an AI agent can build and run the MVP in a fresh “black repo” without local Zephyr installation.

Assumptions:
- Host is a MacBook with Docker Desktop or Podman available.
- All builds happen inside container.
- Runtime is QEMU (Zephyr’s qemu_x86 or qemu_cortex_m depending on chosen target).

---

## 0. Quick start (agent checklist)
1) Build container image
2) Initialize Zephyr workspace (west)
3) Generate signals (codegen)
4) Build app for QEMU
5) Run demo script

---

## 1. Container environment

### 1.1 Dockerfile (recommended baseline)
Create `docker/Dockerfile`:

```dockerfile
FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    git cmake ninja-build gperf \
    ccache dfu-util device-tree-compiler wget \
    python3 python3-pip python3-venv python3-dev \
    xz-utils file make gcc g++ \
    qemu-system-x86 \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir west pyelftools pyyaml

# Optional: tools for formatting/linting
RUN pip3 install --no-cache-dir ruff==0.6.9 yamllint==1.35.1

WORKDIR /work
```

Create `docker/run.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail
docker run --rm -it \
  -v "$(pwd)":/work \
  -w /work \
  vehicleos-rt:dev \
  bash
```

Build image:
```bash
docker build -t vehicleos-rt:dev -f docker/Dockerfile docker
```

Enter container:
```bash
bash docker/run.sh
```

---

## 2. Zephyr workspace bootstrap (inside container)

### 2.1 Initialize west
From repo root (inside container):

```bash
west init -l .
west update
west zephyr-export
pip3 install -r zephyr/scripts/requirements.txt
```

> In a pure “black repo”, you typically vendor Zephyr via `west.yml`.  
> The agent should include a minimal `west.yml` referencing Zephyr and required modules.

### 2.2 Verify toolchain
For QEMU x86, GCC is enough.
Confirm:
```bash
west boards | head
```

---

## 3. Signals schema and code generation

### 3.1 Inputs
Ensure domain packages exist:
- `domains/cabin/signals/package.yaml`
- `domains/cabin/signals/vss.yaml`
- `domains/cabin/signals/constraints.yaml`
(similar for `body`, `powertrain`)

### 3.2 Codegen outputs
Generator writes into:
- `generated/signals/`
  - `vss_handles.h`
  - `vss_registry.c`
  - `vss_acl.c`
  - `vss_routing.c`

### 3.3 Run generation
Create `scripts/gen_signals.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail
python3 tools/codegen/gen_signals.py \
  --domains domains \
  --out generated/signals
```

Run:
```bash
bash scripts/gen_signals.sh
```

Sanity-check:
```bash
ls -la generated/signals
```

---

## 4. Build the Zephyr application (QEMU)

### 4.1 App path
Zephyr app at:
- `app/vehicleos_rt_mvp/`

### 4.2 Build command (QEMU x86 example)
Create `scripts/build_qemu.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail
west build -s app/vehicleos_rt_mvp -b qemu_x86 -p auto
```

Run:
```bash
bash scripts/build_qemu.sh
```

---

## 5. Run the MVP in QEMU

### 5.1 Run
Create `scripts/run_qemu.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail
west build -t run
```

Run:
```bash
bash scripts/run_qemu.sh
```

You should see logs from:
- store init
- z-bus channels init
- lambdas init
- CLI ready

---

## 6. Demo execution

### 6.1 Demo script
Create `scripts/run_demo.sh` that sends CLI commands to the running instance.
Two approaches:

**Approach A (recommended for MVP):** CLI is interactive over UART/console; the demo is a manual script in docs.

**Approach B (agent-friendly):** implement CLI over a simple TCP socket on host (in QEMU user networking) and have `run_demo.sh` send commands.

For MVP agent determinism, pick B.

### 6.2 Example demo commands
- Start cabin precondition:
  - `set Vehicle.Cabin.Precondition.Request {"targetTemp":22.0,"durationSec":300}`
- Door lock:
  - `set Vehicle.Body.Door.FrontLeft.LockTarget "LOCKED"`
- Control targets:
  - `set Vehicle.Chassis.Longitudinal.AccelTarget 0.8`
  - `set Vehicle.Chassis.Lateral.SteeringAngleTarget 2.5`

Expected outputs:
- state transitions and responses
- ack signals for each target

---

## 7. Tests

### 7.1 Unit tests (native test runner inside Zephyr)
- policy tests (reject invalid publish/set)
- store tests (type/bounds)
- ack-required checks (integration)

Create `scripts/run_tests.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail
west test -T app/vehicleos_rt_mvp/tests
```

---

## 8. Zenoh-pico gateway (MVP staging)

### 8.1 MVP phase-1 (stub)
- Implement gateway interface, but use in-process calls
- Validate routing and sharding logic

### 8.2 MVP phase-2 (real zenoh-pico)
- Add zenoh-pico as module (west)
- Map keys (confirmed option 2):
  - `vss.Vehicle.Cabin.HVAC.TargetTemperature`
- Bridge rules:
  - no semantic change
  - shard by slice × domain

---

## 9. Common failure modes (agent troubleshooting)
- Missing `west.yml` or wrong module refs → `west update` fails
- Codegen not run → missing `generated/signals/*`
- QEMU board mismatch → ensure `qemu_x86` exists in installed Zephyr
- Path mismatches between schema and runtime → validate schema with generator

---

## 10. “Definition of done” for build steps
- `docker build ...` succeeds
- `bash scripts/gen_signals.sh` generates outputs
- `bash scripts/build_qemu.sh` builds
- `bash scripts/run_qemu.sh` runs and prints init logs
- `bash scripts/run_tests.sh` passes
- demo scenario proves UC1/UC2/UC3 and ack semantics
