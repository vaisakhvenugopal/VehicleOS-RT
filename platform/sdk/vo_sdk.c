#include "platform/sdk/vo_sdk.h"
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(vo_sdk, LOG_LEVEL_INF);

typedef struct {
    char prefix[64];
    vo_update_cb cb;
} sub_entry_t;

#define MAX_SUBS 8
static sub_entry_t g_subs[MAX_SUBS];
static size_t g_sub_count = 0;

int vo_get(vss_handle_t handle, vss_value_t *out_value)
{
    return store_get(handle, out_value);
}

int vo_get_meta(vss_handle_t handle, vss_value_t *out_value, uint64_t *out_ts_ms, uint32_t *out_seq)
{
    return store_get_meta(handle, out_value, out_ts_ms, out_seq);
}

int vo_set(vss_handle_t handle, const vss_value_t *value)
{
    return store_set(handle, value);
}

int vo_publish(vss_handle_t handle, const vss_value_t *value)
{
    return store_publish(handle, value);
}

int vo_subscribe_prefix(const char *path_prefix, vo_update_cb cb)
{
    if (path_prefix == NULL || cb == NULL) {
        return -1;
    }
    if (g_sub_count >= MAX_SUBS) {
        return -1;
    }

    strncpy(g_subs[g_sub_count].prefix, path_prefix, sizeof(g_subs[g_sub_count].prefix) - 1);
    g_subs[g_sub_count].prefix[sizeof(g_subs[g_sub_count].prefix) - 1] = '\0';
    g_subs[g_sub_count].cb = cb;
    g_sub_count++;

    LOG_INF("Subscribed prefix: %s", path_prefix);
    return 0;
}

int vo_unsubscribe_prefix(const char *path_prefix, vo_update_cb cb)
{
    if (path_prefix == NULL || cb == NULL) {
        return -1;
    }

    for (size_t i = 0; i < g_sub_count; ++i) {
        if (g_subs[i].cb == cb && strcmp(g_subs[i].prefix, path_prefix) == 0) {
            if (i + 1 < g_sub_count) {
                memmove(&g_subs[i], &g_subs[i + 1], (g_sub_count - i - 1) * sizeof(g_subs[0]));
            }
            g_sub_count--;
            LOG_INF("Unsubscribed prefix: %s", path_prefix);
            return 0;
        }
    }
    return -1;
}

void vo_notify_update(vss_handle_t handle)
{
    const char *path = vss_registry[handle].path;
    for (size_t i = 0; i < g_sub_count; ++i) {
        if (strncmp(path, g_subs[i].prefix, strlen(g_subs[i].prefix)) == 0) {
            g_subs[i].cb(handle);
        }
    }
}

bool vo_ack_pending(vss_handle_t handle)
{
    return store_ack_pending(handle);
}
