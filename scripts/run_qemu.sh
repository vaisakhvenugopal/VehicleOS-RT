#!/usr/bin/env bash
set -euo pipefail

export ZEPHYR_TOOLCHAIN_VARIANT=host

west build -t run
