#!/usr/bin/env bash
set -euo pipefail

export ZEPHYR_TOOLCHAIN_VARIANT=host

west build -s app/vehicleos_rt_mvp -b qemu_x86 -p auto
