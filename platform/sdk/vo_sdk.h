#ifndef VEHICLEOS_SDK_H
#define VEHICLEOS_SDK_H

#include "platform/store/store.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*vo_update_cb)(vss_handle_t handle);

int vo_get(vss_handle_t handle, vss_value_t *out_value);
int vo_get_meta(vss_handle_t handle, vss_value_t *out_value, uint64_t *out_ts_ms, uint32_t *out_seq);
int vo_set(vss_handle_t handle, const vss_value_t *value);
int vo_publish(vss_handle_t handle, const vss_value_t *value);
int vo_subscribe_prefix(const char *path_prefix, vo_update_cb cb);
int vo_unsubscribe_prefix(const char *path_prefix, vo_update_cb cb);
void vo_notify_update(vss_handle_t handle);
bool vo_ack_pending(vss_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif
