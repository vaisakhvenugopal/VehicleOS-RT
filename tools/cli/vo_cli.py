#!/usr/bin/env python3
import argparse
import json
import socket
import sys
import time


DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 5555


class LineReader:
    def __init__(self, sock):
        self.sock = sock
        self.buf = b""

    def read_line(self):
        while b"\n" not in self.buf:
            chunk = self.sock.recv(4096)
            if not chunk:
                return None
            self.buf += chunk
        line, self.buf = self.buf.split(b"\n", 1)
        return line.decode("utf-8", errors="replace")


def connect(host, port, timeout=5.0):
    sock = socket.create_connection((host, port), timeout=timeout)
    return sock


def send_line(sock, line):
    if not line.endswith("\n"):
        line += "\n"
    sock.sendall(line.encode("utf-8"))


def print_json(obj):
    sys.stdout.write(json.dumps(obj) + "\n")


def render_list(items, columns):
    widths = {c: len(c) for c in columns}
    for item in items:
        for c in columns:
            widths[c] = max(widths[c], len(str(item.get(c, ""))))

    header = "  ".join(c.ljust(widths[c]) for c in columns)
    sys.stdout.write(header + "\n")
    sys.stdout.write("  ".join("-" * widths[c] for c in columns) + "\n")
    for item in items:
        line = "  ".join(str(item.get(c, "")).ljust(widths[c]) for c in columns)
        sys.stdout.write(line + "\n")


def handle_response(line, json_mode):
    if json_mode:
        sys.stdout.write(line + "\n")
        return 0

    try:
        obj = json.loads(line)
    except json.JSONDecodeError:
        sys.stdout.write(line + "\n")
        return 0

    event = obj.get("event")
    status = obj.get("status")
    cmd = obj.get("cmd", "")

    if event == "error" or status == "error":
        msg = obj.get("message", "error")
        sys.stdout.write(f"{cmd} error: {msg}\n")
        return 1

    data = obj.get("data", {})

    if cmd == "domains":
        for d in data.get("domains", []):
            sys.stdout.write(f"{d}\n")
        return 0

    if cmd == "help":
        sys.stdout.write("commands:\n")
        for c in data.get("commands", []):
            sys.stdout.write(f"  {c}\n")
        if data.get("examples"):
            sys.stdout.write("examples:\n")
            for ex in data.get("examples", []):
                sys.stdout.write(f"  {ex}\n")
        return 0

    if cmd in ("ls", "find"):
        items = data.get("items", [])
        cols = ["path", "type", "class"]
        if cmd == "find":
            cols = ["path", "type", "class", "slice", "domain", "owner"]
        render_list(items, cols)
        return 0

    if cmd == "describe":
        sys.stdout.write(f"path: {data.get('path')}\n")
        sys.stdout.write(f"handle: {data.get('handle')}\n")
        sys.stdout.write(f"type: {data.get('vss_type')}\n")
        sys.stdout.write(f"class: {data.get('class')}\n")
        sys.stdout.write(f"slice: {data.get('slice')}\n")
        sys.stdout.write(f"domain: {data.get('domain')}\n")
        sys.stdout.write(f"owner: {data.get('owner')}\n")
        sys.stdout.write(f"ack_required: {data.get('ack_required')}\n")
        sys.stdout.write(f"ack_paths: {data.get('ack_paths')}\n")
        cons = data.get("constraints", {})
        sys.stdout.write("constraints:\n")
        sys.stdout.write(f"  unit: {cons.get('unit')}\n")
        sys.stdout.write(f"  min: {cons.get('min')}\n")
        sys.stdout.write(f"  max: {cons.get('max')}\n")
        sys.stdout.write(f"  eps: {cons.get('eps')}\n")
        sys.stdout.write(f"  min_period: {cons.get('min_period')}\n")
        proc = data.get("procedure", {})
        if proc:
            sys.stdout.write("procedure:\n")
            sys.stdout.write(f"  state: {proc.get('state')}\n")
            sys.stdout.write(f"  response: {proc.get('response')}\n")
        return 0

    if cmd == "get":
        if not data.get("valid"):
            sys.stdout.write(f"{data.get('path')} = <no value>\n")
            return 0
        sys.stdout.write(
            f"{data.get('path')} = {data.get('value')} "
            f"(ts={data.get('ts')} seq={data.get('seq')} type={data.get('value_type')})\n"
        )
        return 0

    if cmd == "set":
        sys.stdout.write(f"set ok: {data.get('path')}\n")
        sys.stdout.write(f"correlation_id: {data.get('correlation_id')}\n")
        sys.stdout.write(f"ack_required: {data.get('ack_required')}\n")
        if data.get("ack_path"):
            sys.stdout.write(f"ack_path: {data.get('ack_path')}\n")
        return 0

    if cmd == "watch":
        sys.stdout.write(f"watching: {data.get('prefix')}\n")
        return 0

    if event == "response":
        sys.stdout.write(json.dumps(obj) + "\n")
        return 0

    return 0


def run_simple_command(args, line):
    sock = connect(args.host, args.port)
    reader = LineReader(sock)
    send_line(sock, line)
    resp = reader.read_line()
    sock.close()
    if resp is None:
        return 1
    return handle_response(resp, args.json)


def run_watch(args, line):
    sock = connect(args.host, args.port)
    reader = LineReader(sock)
    send_line(sock, line)
    resp = reader.read_line()
    if resp is None:
        sock.close()
        return 1
    if handle_response(resp, args.json) != 0:
        sock.close()
        return 1

    last_emit = {}
    rate_ms = args.rate_ms
    mode = args.mode

    try:
        while True:
            line = reader.read_line()
            if line is None:
                break
            if args.json:
                sys.stdout.write(line + "\n")
                continue
            try:
                obj = json.loads(line)
            except json.JSONDecodeError:
                sys.stdout.write(line + "\n")
                continue
            if obj.get("event") != "update":
                continue
            path = obj.get("path", "")
            now = time.monotonic() * 1000.0
            if mode in ("coalesced", "periodic_latest") and rate_ms:
                last = last_emit.get(path, 0)
                if now - last < rate_ms:
                    continue
                last_emit[path] = now

            value = obj.get("value")
            if obj.get("notify_only"):
                if args.auto_get:
                    send_line(sock, f"get {path}")
                    resp = reader.read_line()
                    if resp:
                        handle_response(resp, args.json)
                    continue
                value = "<notify-only>"
            sys.stdout.write(
                f"{obj.get('ts')}  {obj.get('slice')}  {path}  {value}  {obj.get('seq')}\n"
            )
    except KeyboardInterrupt:
        pass
    finally:
        sock.close()
    return 0


def run_proc(args):
    base = args.request_path
    sock = connect(args.host, args.port)
    reader = LineReader(sock)

    send_line(sock, f"describe {base}")
    desc_line = reader.read_line()
    if not desc_line:
        sock.close()
        return 1
    try:
        desc = json.loads(desc_line)
    except json.JSONDecodeError:
        sock.close()
        sys.stdout.write(desc_line + "\n")
        return 1
    if desc.get("status") == "error" or desc.get("event") == "error":
        sock.close()
        handle_response(desc_line, args.json)
        return 1

    proc = desc.get("data", {}).get("procedure", {})
    state_path = proc.get("state")
    resp_path = proc.get("response")

    send_line(sock, f"set {base} {args.payload}")
    resp = reader.read_line()
    if resp is None:
        sock.close()
        return 1
    if handle_response(resp, args.json) != 0:
        sock.close()
        return 1

    watch_prefix = base.rsplit(".", 1)[0] if "." in base else base
    send_line(sock, f"watch {watch_prefix}")
    watch_resp = reader.read_line()
    if watch_resp is None:
        sock.close()
        return 1
    if handle_response(watch_resp, args.json) != 0:
        sock.close()
        return 1

    exit_code = 0
    try:
        while True:
            line = reader.read_line()
            if line is None:
                break
            if args.json:
                sys.stdout.write(line + "\n")
                try:
                    obj = json.loads(line)
                except json.JSONDecodeError:
                    continue
                if obj.get("event") == "update" and obj.get("path") == resp_path:
                    break
                continue
            obj = json.loads(line)
            if obj.get("event") != "update":
                continue
            path = obj.get("path")
            value = obj.get("value")
            if path == state_path:
                sys.stdout.write(f"state: {value}\n")
            if path == resp_path:
                sys.stdout.write(f"response: {value}\n")
                if isinstance(value, str) and any(x in value.upper() for x in ["FAIL", "ERROR", "REJECT"]):
                    exit_code = 1
                break
    except KeyboardInterrupt:
        pass
    finally:
        sock.close()
    return exit_code


def build_parser():
    p = argparse.ArgumentParser(description="VehicleOS-RT CLI v2")
    p.add_argument("--host", default=DEFAULT_HOST)
    p.add_argument("--port", type=int, default=DEFAULT_PORT)
    p.add_argument("--json", action="store_true", help="JSON lines output")

    sub = p.add_subparsers(dest="command")

    sub.add_parser("help")
    sub.add_parser("domains")

    ls_p = sub.add_parser("ls")
    ls_p.add_argument("prefix")

    find_p = sub.add_parser("find")
    find_p.add_argument("pattern")

    desc_p = sub.add_parser("describe")
    desc_p.add_argument("path")

    get_p = sub.add_parser("get")
    get_p.add_argument("path")

    set_p = sub.add_parser("set")
    set_p.add_argument("path")
    set_p.add_argument("value")

    watch_p = sub.add_parser("watch")
    watch_p.add_argument("prefix")
    watch_p.add_argument("--mode", choices=["on_change", "coalesced", "periodic_latest"], default="on_change")
    watch_p.add_argument("--rate-ms", type=int, default=0)
    watch_p.add_argument("--auto-get", action="store_true")

    proc_p = sub.add_parser("proc")
    proc_p.add_argument("action", choices=["start"])
    proc_p.add_argument("request_path")
    proc_p.add_argument("payload")

    return p


def main():
    parser = build_parser()
    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        return 1

    if args.command == "help":
        return run_simple_command(args, "help")
    if args.command == "domains":
        return run_simple_command(args, "domains")
    if args.command == "ls":
        return run_simple_command(args, f"ls {args.prefix}")
    if args.command == "find":
        return run_simple_command(args, f"find {args.pattern}")
    if args.command == "describe":
        return run_simple_command(args, f"describe {args.path}")
    if args.command == "get":
        return run_simple_command(args, f"get {args.path}")
    if args.command == "set":
        return run_simple_command(args, f"set {args.path} {args.value}")
    if args.command == "watch":
        return run_watch(args, f"watch {args.prefix}")
    if args.command == "proc" and args.action == "start":
        return run_proc(args)

    return 0


if __name__ == "__main__":
    sys.exit(main())
