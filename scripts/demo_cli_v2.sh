#!/usr/bin/env bash
set -euo pipefail

CLI_HOST="${CLI_HOST:-127.0.0.1}"
CLI_PORT="${CLI_PORT:-5555}"
CLI="python3 tools/cli/vo_cli.py --host ${CLI_HOST} --port ${CLI_PORT}"

echo "== domains"
$CLI domains

echo "== ls cabin signals"
$CLI ls vss.Vehicle.Cabin

echo "== describe target temperature"
$CLI describe vss.Vehicle.Cabin.HVAC.TargetTemperature

echo "== watch cabin for 5 seconds"
timeout 5s $CLI watch vss.Vehicle.Cabin --mode on_change || true

echo "== set target temperature"
$CLI set vss.Vehicle.Cabin.HVAC.TargetTemperature 22.0

echo "== start cabin precondition"
$CLI proc start vss.Vehicle.Cabin.Precondition.Request '{"targetTemp":22.0,"durationSec":300}'
