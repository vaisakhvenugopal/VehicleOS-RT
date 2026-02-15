#!/usr/bin/env bash
set -euo pipefail

export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk

west build -s app/vehicleos_rt_mvp -b qemu_cortex_m3 -p auto
