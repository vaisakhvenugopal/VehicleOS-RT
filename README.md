# VehicleOS

VehicleOS is a Zephyr-based real-time platform prototype that uses COVESA VSS semantics as the API contract, Z-bus for in-node messaging, and a lightweight gateway layer for inter-node transport. The core MVP lives in the `VehicleOS-RT` subdirectory and targets QEMU for repeatable local runs.

This README focuses on getting the MVP app built and running, and on what “success” looks like in the console output.

## Repository layout

- `VehicleOS-RT/`: Main Zephyr app, build scripts, and architecture docs
- `VehicleOS-RT/app/vehicleos_rt_mvp/`: MVP application source
- `VehicleOS-RT/scripts/`: Build/run helpers
- `VehicleOS-RT/domains/`: VSS domain schemas and lambdas
- `VehicleOS-RT/generated/`: Codegen outputs (signal registry)
- `zephyr/`, `modules/`, `tools/`, `bootloader/`: West-managed dependencies and tooling

## Quick start (recommended: container + QEMU)

These steps match the repo’s scripts and are the most deterministic path on macOS.

1. Build the container

```bash
cd VehicleOS-RT

docker build -t vehicleos-rt:dev -f docker/Dockerfile docker
```

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

5. Build for QEMU

```bash
bash scripts/build_qemu.sh
```

6. Run in QEMU

```bash
bash scripts/run_qemu.sh
```

## Expected outcome

When QEMU starts, you should see logs similar to:

- `VehicleOS-RT MVP Booting...`
- Store initialization
- Z-bus backbone initialization
- Gateway initialization
- Lambda startup messages
- `VehicleOS-RT MVP Ready. Waiting for commands...`

The demo runner `scripts/run_demo.sh` currently prints guidance only. The real interaction layer is stubbed for the MVP.

## Troubleshooting

- If `west update` fails, confirm `VehicleOS-RT/west.yml` is present and you ran `west init -l .` from `VehicleOS-RT/`.
- If build fails due to missing signal headers, re-run `bash scripts/gen_signals.sh`.
- If QEMU fails to run, ensure you built for `qemu_cortex_m3` as used in `scripts/build_qemu.sh`.

## Docs

- Architecture overview: `VehicleOS-RT/docs/01_architecture.md`
- MVP build steps: `VehicleOS-RT/docs/03_mvp_build_steps_agent.md`
