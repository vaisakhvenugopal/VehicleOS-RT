#!/usr/bin/env bash
set -euo pipefail
docker run --rm -it \
  -v "$(pwd)":/work \
  -w /work \
  vehicleos-rt:dev \
  bash