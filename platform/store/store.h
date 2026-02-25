#ifndef VEHICLEOS_STORE_H
#define VEHICLEOS_STORE_H

#include <stdbool.h>
#include <stdint.h>
#include "vss_registry.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    vss_type_t type;
    union {
        bool b;
        int32_t i32;
        uint32_t u32;
        float f;
        double d;
    } scalar;
    const char *str;
} vss_value_t;

void store_init(void);
int store_get(vss_handle_t handle, vss_value_t *out_value);
int store_get_meta(vss_handle_t handle, vss_value_t *out_value, uint64_t *out_ts_ms, uint32_t *out_seq);
int store_set(vss_handle_t handle, const vss_value_t *value);
int store_publish(vss_handle_t handle, const vss_value_t *value);
bool store_ack_pending(vss_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif
