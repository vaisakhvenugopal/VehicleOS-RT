import argparse
import os
import yaml
import sys

def to_c_handle(path):
    return "VSS_" + path.upper().replace(".", "_")

def normalize_type(t):
    return t.strip().lower()

def normalize_class(c):
    return c.strip().lower()

def load_yaml(path):
    if not os.path.exists(path):
        return None
    with open(path, "r") as f:
        return yaml.safe_load(f)

def class_to_enum(c):
    mapping = {
        "measured": "VSS_CLASS_MEASURED",
        "target": "VSS_CLASS_TARGET",
        "state": "VSS_CLASS_STATE",
        "config": "VSS_CLASS_CONFIG",
        "procedure.request": "VSS_CLASS_PROCEDURE_REQUEST",
        "procedure.state": "VSS_CLASS_PROCEDURE_STATE",
        "procedure.response": "VSS_CLASS_PROCEDURE_RESPONSE",
        "ack": "VSS_CLASS_ACK",
    }
    return mapping.get(c, "VSS_CLASS_STATE")

def type_to_enum(t):
    mapping = {
        "bool": "VSS_TYPE_BOOL",
        "boolean": "VSS_TYPE_BOOL",
        "int32": "VSS_TYPE_INT32",
        "uint32": "VSS_TYPE_UINT32",
        "float": "VSS_TYPE_FLOAT",
        "double": "VSS_TYPE_DOUBLE",
        "string": "VSS_TYPE_STRING",
        "struct": "VSS_TYPE_STRUCT",
        "array": "VSS_TYPE_ARRAY",
        "enum": "VSS_TYPE_ENUM",
    }
    return mapping.get(t, "VSS_TYPE_STRING")

def slice_to_enum(s):
    mapping = {
        "safety": "VSS_SLICE_SAFETY",
        "control": "VSS_SLICE_CONTROL",
        "data": "VSS_SLICE_DATA",
        "diag": "VSS_SLICE_DIAG",
    }
    return mapping.get(s, "VSS_SLICE_CONTROL")

def domain_to_enum(d):
    mapping = {
        "cabin": "VSS_DOMAIN_CABIN",
        "body": "VSS_DOMAIN_BODY",
        "powertrain": "VSS_DOMAIN_POWERTRAIN",
        "chassis": "VSS_DOMAIN_CHASSIS",
    }
    return mapping.get(d, "VSS_DOMAIN_UNKNOWN")

def generate(domains_dir, out_dir):
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)

    signals = []
    constraints = {}

    # Walk domains
    for domain in os.listdir(domains_dir):
        d_path = os.path.join(domains_dir, domain)
        if not os.path.isdir(d_path):
            continue

        vss_file = os.path.join(d_path, "signals", "vss.yaml")
        constraints_file = os.path.join(d_path, "signals", "constraints.yaml")

        vss_data = load_yaml(vss_file) or {}
        cons_data = load_yaml(constraints_file) or {}

        defaults = cons_data.get("defaults", {})
        overrides = cons_data.get("overrides", {})

        for sig in vss_data.get("signals", []):
            path = sig.get("path")
            vss_type = sig.get("vss_type", sig.get("type", "string"))
            cls = sig.get("class", "state")

            meta = {
                "path": path,
                "vss_type": normalize_type(vss_type),
                "class": normalize_class(cls),
                "slice": defaults.get("slice", "control"),
                "domain": defaults.get("domain", domain),
                "ack_required": False,
                "min": None,
                "max": None,
            }

            if path in overrides:
                ov = overrides[path]
                meta["slice"] = ov.get("slice", meta["slice"])
                meta["domain"] = ov.get("domain", meta["domain"])
                meta["ack_required"] = bool(ov.get("ack_required", meta["ack_required"]))
                if "min" in ov:
                    meta["min"] = ov["min"]
                if "max" in ov:
                    meta["max"] = ov["max"]

            signals.append(meta)

    # Generate vss_handles.h
    with open(os.path.join(out_dir, "vss_handles.h"), "w") as f:
        f.write("#ifndef VSS_HANDLES_H\n#define VSS_HANDLES_H\n\n")
        f.write("typedef enum {\n")
        for i, sig in enumerate(signals):
            f.write(f"    {to_c_handle(sig['path'])} = {i},\n")
        f.write(f"    VSS_SIGNAL_COUNT = {len(signals)}\n")
        f.write("} vss_handle_t;\n\n")
        f.write("#endif\n")

    # Generate vss_registry.h
    with open(os.path.join(out_dir, "vss_registry.h"), "w") as f:
        f.write("#ifndef VSS_REGISTRY_H\n#define VSS_REGISTRY_H\n\n")
        f.write("#include <stdbool.h>\n#include <stdint.h>\n")
        f.write("#include \"vss_handles.h\"\n\n")
        f.write("typedef enum {\n")
        f.write("    VSS_TYPE_BOOL, VSS_TYPE_INT32, VSS_TYPE_UINT32, VSS_TYPE_FLOAT, VSS_TYPE_DOUBLE,\n")
        f.write("    VSS_TYPE_STRING, VSS_TYPE_STRUCT, VSS_TYPE_ARRAY, VSS_TYPE_ENUM\n")
        f.write("} vss_type_t;\n\n")
        f.write("typedef enum {\n")
        f.write("    VSS_CLASS_MEASURED, VSS_CLASS_TARGET, VSS_CLASS_STATE, VSS_CLASS_CONFIG,\n")
        f.write("    VSS_CLASS_PROCEDURE_REQUEST, VSS_CLASS_PROCEDURE_STATE, VSS_CLASS_PROCEDURE_RESPONSE,\n")
        f.write("    VSS_CLASS_ACK\n")
        f.write("} vss_class_t;\n\n")
        f.write("typedef enum {\n")
        f.write("    VSS_SLICE_SAFETY, VSS_SLICE_CONTROL, VSS_SLICE_DATA, VSS_SLICE_DIAG\n")
        f.write("} vss_slice_t;\n\n")
        f.write("typedef enum {\n")
        f.write("    VSS_DOMAIN_CABIN, VSS_DOMAIN_BODY, VSS_DOMAIN_POWERTRAIN, VSS_DOMAIN_CHASSIS, VSS_DOMAIN_UNKNOWN\n")
        f.write("} vss_domain_t;\n\n")
        f.write("struct vss_signal_meta {\n")
        f.write("    const char *path;\n")
        f.write("    vss_type_t type;\n")
        f.write("    vss_class_t class;\n")
        f.write("    vss_slice_t slice;\n")
        f.write("    vss_domain_t domain;\n")
        f.write("    bool ack_required;\n")
        f.write("    bool has_min;\n")
        f.write("    double min;\n")
        f.write("    bool has_max;\n")
        f.write("    double max;\n")
        f.write("};\n\n")
        f.write("extern const struct vss_signal_meta vss_registry[VSS_SIGNAL_COUNT];\n")
        f.write("int vss_find_handle_by_path(const char *path);\n\n")
        f.write("#endif\n")

    # Generate vss_registry.c
    with open(os.path.join(out_dir, "vss_registry.c"), "w") as f:
        f.write("#include \"vss_registry.h\"\n")
        f.write("#include <string.h>\n\n")
        f.write("const struct vss_signal_meta vss_registry[VSS_SIGNAL_COUNT] = {\n")
        for sig in signals:
            f.write("    {\n")
            f.write(f"        .path = \"{sig['path']}\",\n")
            f.write(f"        .type = {type_to_enum(sig['vss_type'])},\n")
            f.write(f"        .class = {class_to_enum(sig['class'])},\n")
            f.write(f"        .slice = {slice_to_enum(sig['slice'])},\n")
            f.write(f"        .domain = {domain_to_enum(sig['domain'])},\n")
            f.write(f"        .ack_required = {'true' if sig['ack_required'] else 'false'},\n")
            f.write(f"        .has_min = {'true' if sig['min'] is not None else 'false'},\n")
            f.write(f"        .min = {sig['min'] if sig['min'] is not None else 0.0},\n")
            f.write(f"        .has_max = {'true' if sig['max'] is not None else 'false'},\n")
            f.write(f"        .max = {sig['max'] if sig['max'] is not None else 0.0},\n")
            f.write("    },\n")
        f.write("};\n\n")
        f.write("int vss_find_handle_by_path(const char *path)\n{\n")
        f.write("    for (int i = 0; i < VSS_SIGNAL_COUNT; ++i) {\n")
        f.write("        if (strcmp(vss_registry[i].path, path) == 0) {\n")
        f.write("            return i;\n")
        f.write("        }\n")
        f.write("    }\n")
        f.write("    return -1;\n")
        f.write("}\n")

    print(f"Generated {len(signals)} signals in {out_dir}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--domains", required=True)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()
    generate(args.domains, args.out)
