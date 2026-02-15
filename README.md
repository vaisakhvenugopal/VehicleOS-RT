# VehicleOS

VehicleOS is a Zephyr-based real-time platform prototype that uses COVESA VSS semantics as the API contract, Z-bus for in-node messaging, and a lightweight gateway layer for inter-node transport. The core MVP lives at this repository root and targets QEMU for repeatable local runs.

This README focuses on getting the MVP app built and running (ARM QEMU only), how to interact with the CLI, and what “success” looks like in the console output.

## Repository layout

- `app/vehicleos_rt_mvp/`: MVP application source
- `scripts/`: Build/run helpers
- `domains/`: VSS domain schemas and lambdas
- `generated/`: Codegen outputs (signal registry)
- `zephyr/`, `modules/`, `tools/`, `bootloader/`: West-managed dependencies and tooling

## Quick start (recommended: container + ARM QEMU)

These steps match the repo’s scripts and are the most deterministic path on macOS.

1. Build the container

```bash
docker build -t vehicleos-rt:dev -f docker/Dockerfile docker
```

The Dockerfile auto-selects the correct Zephyr SDK for the container architecture (amd64 or arm64).

2. Enter the container

```bash
bash docker/run.sh
```

3. Initialize the Zephyr workspace (inside the container)

```bash
west init -l .
west update
west zephyr-export
pip3 install -r zephyr/scripts/requirements.txt
```

4. Generate signal registry code

```bash
bash scripts/gen_signals.sh
```

5. Build for ARM QEMU (Cortex-M3)

```bash
bash scripts/build_qemu.sh
```

6. Run in QEMU

```bash
bash scripts/run_qemu.sh
```

7. Use the interactive CLI (inside the QEMU console)

You will see a prompt like:
```
> 
```

Type commands to drive the three MVP use cases:

UC1: Cabin Precondition (procedure)
```bash
set Vehicle.Cabin.Precondition.Request START
```

UC2: Door Lock (target + ack)
```bash
set Vehicle.Body.Door.FrontLeft.LockTarget LOCKED
```

UC3: Control targets (targets + acks)
```bash
set Vehicle.Chassis.Longitudinal.AccelTarget 0.8
set Vehicle.Chassis.Lateral.SteeringAngleTarget 2.5
```

You can also read values:
```bash
get Vehicle.Cabin.HVAC.TargetTemperature.State
get Vehicle.Body.Door.FrontLeft.IsLocked
get Vehicle.Speed
```

To exit QEMU, press `CTRL+a` then `x`.

## Expected outcome

When QEMU starts, you should see logs similar to:

- `VehicleOS-RT MVP Booting...`
- Store initialization
- Z-bus backbone initialization
- Gateway initialization
- Lambda startup messages
- `VehicleOS-RT MVP Ready. Waiting for commands...`

The interactive CLI prints a help banner and a `>` prompt. Each `set` command should log an `Ack required` message for target signals and later an `Ack received` message when controllers publish acknowledgements.

The demo runner `scripts/run_demo.sh` currently prints guidance only. The real interaction layer is the QEMU UART console.

## Troubleshooting

- If `west update` fails, confirm `west.yml` is present and you ran `west init -l .` from the repo root.
- If build fails due to missing signal headers, re-run `bash scripts/gen_signals.sh`.
- If QEMU fails to run, ensure you built for `qemu_cortex_m3` as used in `scripts/build_qemu.sh` and that the Docker image was rebuilt after any Dockerfile changes.

## Docs

- Architecture overview: `docs/01_architecture.md`
- MVP build steps: `docs/03_mvp_build_steps_agent.md`
