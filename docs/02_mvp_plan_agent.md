# VehicleOS-RT: MVP Plan for an AI Agent (black repo execution plan)

**Purpose:** Provide an LLM agent (Codex-style) with a clear MVP scope, architecture targets, tasks, acceptance criteria, and a deterministic demo scenario. This is the “what to build” plan.

---

## 1. MVP goals

1) Prove the **VehicleOS-RT primitives**:
- Signal Store (typed, policy-gated)
- Z-bus backbone (slice × domain channels)
- Zenoh-pico bridge (semantic-preserving)

2) Prove the **provider vs client** model:
- providers use `Publish()`
- clients use `Set()`
- ack is required for targets

3) Prove **procedures** for composite orchestration:
- `Cabin.Precondition`
- plus at least one **Body** and one **Control** use case

4) Run in a **virtualized environment** (container + QEMU). No native macOS install.

---

## 2. MVP scope (domains and use cases)

### 2.1 Domains included
- `cabin`
- `body`
- `powertrain`

(ADAS excluded from MVP; keep L3+ scalability notes only.)

### 2.2 Use cases (minimum set)
**UC1: Cabin precondition (procedure)**
- Request: `Vehicle.Cabin.Precondition.Request`
- State: `...State`
- Response: `...Response`
- Internals:
  - Sets `Vehicle.Cabin.HVAC.TargetTemperature` (client Set)
  - HVAC controller consumes target and publishes ack/state

**UC2: Door operations (targets + acks)**
- HMI sets `Vehicle.Body.Door.FrontLeft.LockTarget`
- Door controller publishes `...LockTarget.Ack` and `...IsLocked` state

**UC3: Lateral + longitudinal control (targets + acks, simulated)**
- Targets:
  - `Vehicle.Chassis.Longitudinal.AccelTarget`
  - `Vehicle.Chassis.Lateral.SteeringAngleTarget`
- Controllers publish ack + state (simulated actuation)
- Powertrain publishes `Vehicle.Speed` measured (simulated) to show closed loop.

---

## 3. Deliverables (repo artifacts)

### 3.1 Code deliverables
1) `platform/store/` — Signal Store core
2) `platform/bus/` — Z-bus channels and notifier binding
3) `platform/policy/` — capability + ownership enforcement
4) `platform/gateway/` — Zenoh-pico bridge (per slice × domain)
5) `tools/codegen/` — schema → generated tables
6) `domains/*/signals/` — signal packages (YAML)
7) `domains/*/lambdas/` — lambda folders (C/C++)

### 3.2 Documentation deliverables
- This plan (current doc)
- Build steps (separate doc)
- Architecture doc (separate doc)

### 3.3 Test deliverables
- Unit tests for:
  - store typing + bounds
  - policy enforcement (publish/set permissions)
  - ack required enforcement
- Integration tests in QEMU/container:
  - UC1/UC2/UC3 deterministic scripts

---

## 4. Repository structure (MVP)

```
.
├─ docs/
│  ├─ 01_architecture.md
│  ├─ 02_mvp_plan_agent.md
│  └─ 03_mvp_build_steps_agent.md
├─ platform/
│  ├─ store/
│  ├─ bus/
│  ├─ policy/
│  ├─ gateway/
│  └─ sdk/
├─ tools/
│  └─ codegen/
├─ domains/
│  ├─ cabin/
│  │  ├─ signals/
│  │  └─ lambdas/
│  ├─ body/
│  │  ├─ signals/
│  │  └─ lambdas/
│  └─ powertrain/
│     ├─ signals/
│     └─ lambdas/
├─ app/
│  └─ vehicleos_rt_mvp/          # Zephyr application
├─ scripts/
│  ├─ gen_signals.sh
│  ├─ build_qemu.sh
│  └─ run_demo.sh
└─ docker/
   ├─ Dockerfile
   └─ run.sh
```

---

## 5. Signal schema (agent must implement)

### 5.1 Domain package files
Each domain has:
- `package.yaml` (name, version, owners)
- `vss.yaml` (signals list with canonical VSS paths and types)
- `constraints.yaml` (bounds, slice, class, min_period, eps, ownership, ACL)

### 5.2 Mandatory schema fields per signal
- `path` (canonical `.` path)
- `vss_type` (bool/int32/uint32/float/double/string/struct/array/enum)
- `class` (measured/target/state/config/procedure.* / ack)
- `slice` (safety/control/data/diag)
- `domain` (cabin/body/powertrain)
- `owner` (provider lambda id for publish-only signals)
- `acl` (who may Set/Publish/Subscribe)
- `ack_required` (true for targets)

---

## 6. Runtime interfaces (agent must implement)

### 6.1 Store API (SDK)
- `vo_get(handle, &out_value)`
- `vo_set(handle, value, correlation_id)`
- `vo_publish(handle, value, meta)`
- `vo_subscribe(filter, mode, cb)`

### 6.2 Notification payload (Z-bus message type)
A single message type per channel:
- `path_handle`
- `vss_type`
- `seq`
- `timestamp`
- `flags` (changed/ack/fault)
- optional inline value (only if small type; otherwise subscriber pulls from store)

> Constraint: “not CAN-like” means we do not model the bus as arbitrary ID frames; payload remains a **VSS update structure**.

### 6.3 Ack enforcement
If a signal is `target` and `ack_required=true`:
- store emits notification on Set
- controller must Publish `Ack` within configured timeout (MVP: best-effort warning)
- integration tests validate ack arrives.

---

## 7. Lambdas (minimum set)

### 7.1 Platform lambdas
- `gateway.cabin.control` (bridge instance)
- `gateway.body.control`
- `gateway.powertrain.data`
(These may be built as part of platform/gateway; still “lambda-shaped” components.)

### 7.2 Domain lambdas
**Cabin**
- `cabin.sensor_sim` (Publish cabin temp measured)
- `cabin.hvac_controller` (Subscribe target, Publish ack + state)
- `cabin.precondition_procedure` (Subscribe request, Set targets, Publish state/response)

**Body**
- `body.door_controller` (Subscribe lock target, Publish ack + state)
- `body.sensor_sim` (Publish door open/closed measured)

**Powertrain / Chassis simulation**
- `powertrain.speed_sim` (Publish speed measured)
- `chassis.longitudinal_controller_sim` (Subscribe accel target, Publish ack/state)
- `chassis.lateral_controller_sim` (Subscribe steering target, Publish ack/state)

**HMI/CLI**
- `tools.cli_client` (Get/Set/Subscribe, prints to console)

---

## 8. Demo scenario (acceptance criteria)

### 8.1 Demo: UC1 Cabin precondition
1) CLI: `set Vehicle.Cabin.Precondition.Request {...}`
2) Observe:
   - `...State` transitions (e.g., INIT → RUNNING → DONE)
   - `HVAC.TargetTemperature` set by procedure
   - `HVAC.TargetTemperature.Ack` published by controller
   - `...Response` published (success/fail)

### 8.2 Demo: UC2 Door lock
1) CLI: `set Vehicle.Body.Door.FrontLeft.LockTarget LOCKED`
2) Observe:
   - `...LockTarget.Ack`
   - `Vehicle.Body.Door.FrontLeft.IsLocked` state

### 8.3 Demo: UC3 Control targets
1) CLI: set accel target and steering target
2) Observe ack + state, and speed changes in simulation.

### 8.4 Pass criteria
- All three UCs run end-to-end in QEMU/container
- Policy enforcement rejects invalid Set/Publish attempts
- Ack signals appear for all target Sets
- Hot signal subscription is notify-only + pull-latest (demonstrated by ignition or similar shared signal)

---

## 9. L3+ scalability notes (documented constraints)
- VSS signals remain for decisions/vehicle state and control targets/states.
- High-bandwidth perception is modeled as streams (not part of MVP).
- Slicing by `slice × domain` is mandatory to avoid choke points.
- Gateway instances are sharded by slice × domain.

---

## 10. Task breakdown (agent execution order)

1) Implement schema + codegen producing:
   - handles/enums
   - routing tables
   - ACL tables
2) Implement Store core:
   - typed set/get
   - publish/set role checks
   - subscribe registry
3) Implement Z-bus channels:
   - create channels per slice × domain
   - store emits notifications
4) Implement lambda SDK
5) Implement domain lambdas (UC1/UC2/UC3)
6) Implement CLI client
7) Implement gateway skeleton (Zenoh optional in MVP run; stub acceptable if time)
8) Add tests + demo scripts

> MVP may start with a stubbed “gateway” that uses in-process calls, then upgrade to real Zenoh-pico bridging.

---

## 11. Done definition
- `scripts/run_demo.sh` prints a clear transcript proving each UC and the ack semantics.
- `scripts/run_tests.sh` passes unit + integration tests in container/QEMU.
