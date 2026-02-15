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
