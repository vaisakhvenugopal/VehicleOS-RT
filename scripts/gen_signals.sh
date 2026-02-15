#!/usr/bin/env bash
set -euo pipefail
python3 tools/codegen/gen_signals.py \
  --domains domains \
  --out generated/signals