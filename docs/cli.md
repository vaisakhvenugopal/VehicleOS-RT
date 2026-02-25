# VehicleOS-RT CLI v2

This is the primary debugging and introspection CLI for VehicleOS-RT.

## Usage

Run the host client (default: `127.0.0.1:5555`):

```
python3 tools/cli/vo_cli.py help
python3 tools/cli/vo_cli.py domains
python3 tools/cli/vo_cli.py ls vss.Vehicle.Cabin
python3 tools/cli/vo_cli.py find temperature
python3 tools/cli/vo_cli.py describe vss.Vehicle.Cabin.HVAC.TargetTemperature
python3 tools/cli/vo_cli.py get vss.Vehicle.Cabin.HVAC.CabinTemperature
python3 tools/cli/vo_cli.py set vss.Vehicle.Cabin.HVAC.TargetTemperature 22.0
python3 tools/cli/vo_cli.py watch vss.Vehicle.Cabin --mode on_change
python3 tools/cli/vo_cli.py proc start vss.Vehicle.Cabin.Precondition.Request '{"targetTemp":22.0,"durationSec":300}'
```

JSON lines output for agents/CI:

```
python3 tools/cli/vo_cli.py --json watch vss.Vehicle.Cabin
```

## Transport

The CLI server runs inside Zephyr and listens on TCP port `5555`.

## Notes

- `vss.` prefix is optional in commands.
- `watch` streams `update` events until interrupted.
- `proc start` auto-watches procedure state/response and exits on response.
