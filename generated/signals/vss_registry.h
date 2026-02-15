#ifndef VSS_REGISTRY_H
#define VSS_REGISTRY_H

#include <stdbool.h>
#include <stdint.h>
#include "vss_handles.h"

typedef enum {
    VSS_TYPE_BOOL, VSS_TYPE_INT32, VSS_TYPE_UINT32, VSS_TYPE_FLOAT, VSS_TYPE_DOUBLE,
    VSS_TYPE_STRING, VSS_TYPE_STRUCT, VSS_TYPE_ARRAY, VSS_TYPE_ENUM
} vss_type_t;

typedef enum {
    VSS_CLASS_MEASURED, VSS_CLASS_TARGET, VSS_CLASS_STATE, VSS_CLASS_CONFIG,
    VSS_CLASS_PROCEDURE_REQUEST, VSS_CLASS_PROCEDURE_STATE, VSS_CLASS_PROCEDURE_RESPONSE,
    VSS_CLASS_ACK
} vss_class_t;

typedef enum {
    VSS_SLICE_SAFETY, VSS_SLICE_CONTROL, VSS_SLICE_DATA, VSS_SLICE_DIAG
} vss_slice_t;

typedef enum {
    VSS_DOMAIN_CABIN, VSS_DOMAIN_BODY, VSS_DOMAIN_POWERTRAIN, VSS_DOMAIN_CHASSIS, VSS_DOMAIN_UNKNOWN
} vss_domain_t;

struct vss_signal_meta {
    const char *path;
    vss_type_t type;
    vss_class_t class;
    vss_slice_t slice;
    vss_domain_t domain;
    bool ack_required;
    bool has_min;
    double min;
    bool has_max;
    double max;
};

extern const struct vss_signal_meta vss_registry[VSS_SIGNAL_COUNT];
int vss_find_handle_by_path(const char *path);

#endif
