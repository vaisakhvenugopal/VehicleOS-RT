#!/usr/bin/env bash
set -euo pipefail

export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk

west build -t run
