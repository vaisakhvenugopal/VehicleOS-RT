#include "platform/store/store.h"
#include "platform/policy/policy.h"
#include <zephyr/logging/log.h>
#include <string.h>
#include <errno.h>

LOG_MODULE_REGISTER(vo_store, LOG_LEVEL_INF);

typedef struct {
    bool has_value;
    vss_value_t value;
} store_slot_t;

static store_slot_t g_store[VSS_SIGNAL_COUNT];
static bool g_ack_pending[VSS_SIGNAL_COUNT];

static bool is_numeric_type(vss_type_t type)
{
    return (type == VSS_TYPE_INT32 || type == VSS_TYPE_UINT32 ||
            type == VSS_TYPE_FLOAT || type == VSS_TYPE_DOUBLE);
}

static int validate_type(vss_handle_t handle, const vss_value_t *value)
{
    vss_type_t expected = vss_registry[handle].type;
    if (value == NULL) {
        return -EINVAL;
    }
    if (value->type != expected) {
        LOG_ERR("Type mismatch for %s: expected %d, got %d",
                vss_registry[handle].path, expected, value->type);
        return -EINVAL;
    }
    return 0;
}

static int validate_range(vss_handle_t handle, const vss_value_t *value)
{
    const struct vss_signal_meta *meta = &vss_registry[handle];
    if (!is_numeric_type(meta->type)) {
        return 0;
    }

    double val = 0.0;
    switch (meta->type) {
    case VSS_TYPE_INT32:
        val = (double)value->scalar.i32;
        break;
    case VSS_TYPE_UINT32:
        val = (double)value->scalar.u32;
        break;
    case VSS_TYPE_FLOAT:
        val = (double)value->scalar.f;
        break;
    case VSS_TYPE_DOUBLE:
        val = value->scalar.d;
        break;
    default:
        break;
    }

    if (meta->has_min && val < meta->min) {
        LOG_ERR("Value below min for %s", meta->path);
        return -ERANGE;
    }
    if (meta->has_max && val > meta->max) {
        LOG_ERR("Value above max for %s", meta->path);
        return -ERANGE;
    }

    return 0;
}

static void mark_ack_required(vss_handle_t target_handle)
{
    if (!vss_registry[target_handle].ack_required) {
        return;
    }

    g_ack_pending[target_handle] = true;
    LOG_INF("Ack required for %s", vss_registry[target_handle].path);
}

static void handle_ack_published(vss_handle_t ack_handle)
{
    const char *ack_path = vss_registry[ack_handle].path;
    const char *suffix = ".Ack";
    size_t len = strlen(ack_path);
    size_t s_len = strlen(suffix);

    if (len <= s_len || strcmp(ack_path + (len - s_len), suffix) != 0) {
        return;
    }

    char target_path[256];
    if (len - s_len >= sizeof(target_path)) {
        return;
    }
    memcpy(target_path, ack_path, len - s_len);
    target_path[len - s_len] = '\0';

    int target_handle = vss_find_handle_by_path(target_path);
    if (target_handle < 0) {
        return;
    }

    g_ack_pending[target_handle] = false;
    LOG_INF("Ack received for %s", vss_registry[target_handle].path);
}

void store_init(void)
{
    memset(g_store, 0, sizeof(g_store));
    memset(g_ack_pending, 0, sizeof(g_ack_pending));
    LOG_INF("Initializing Signal Store... [Signals: %d]", VSS_SIGNAL_COUNT);
}

int store_get(vss_handle_t handle, vss_value_t *out_value)
{
    if (handle < 0 || handle >= VSS_SIGNAL_COUNT || out_value == NULL) {
        return -EINVAL;
    }
    if (!g_store[handle].has_value) {
        return -ENOENT;
    }
    *out_value = g_store[handle].value;
    return 0;
}

int store_set(vss_handle_t handle, const vss_value_t *value)
{
    int rc;

    if (handle < 0 || handle >= VSS_SIGNAL_COUNT) {
        return -EINVAL;
    }

    rc = policy_check_set(handle);
    if (rc != 0) {
        return rc;
    }

    rc = validate_type(handle, value);
    if (rc != 0) {
        return rc;
    }

    rc = validate_range(handle, value);
    if (rc != 0) {
        return rc;
    }

    g_store[handle].value = *value;
    g_store[handle].has_value = true;

    LOG_INF("Set %s", vss_registry[handle].path);
    mark_ack_required(handle);

    return 0;
}

int store_publish(vss_handle_t handle, const vss_value_t *value)
{
    int rc;

    if (handle < 0 || handle >= VSS_SIGNAL_COUNT) {
        return -EINVAL;
    }

    rc = policy_check_publish(handle);
    if (rc != 0) {
        return rc;
    }

    rc = validate_type(handle, value);
    if (rc != 0) {
        return rc;
    }

    rc = validate_range(handle, value);
    if (rc != 0) {
        return rc;
    }

    g_store[handle].value = *value;
    g_store[handle].has_value = true;

    LOG_INF("Publish %s", vss_registry[handle].path);

    if (vss_registry[handle].class == VSS_CLASS_ACK) {
        handle_ack_published(handle);
    }

    return 0;
}
