#include "platform/policy/policy.h"
#include <zephyr/logging/log.h>
#include <errno.h>

LOG_MODULE_REGISTER(vo_policy, LOG_LEVEL_INF);

static bool is_set_class(vss_class_t cls)
{
    return (cls == VSS_CLASS_TARGET ||
            cls == VSS_CLASS_CONFIG ||
            cls == VSS_CLASS_PROCEDURE_REQUEST);
}

static bool is_publish_class(vss_class_t cls)
{
    return (cls == VSS_CLASS_MEASURED ||
            cls == VSS_CLASS_STATE ||
            cls == VSS_CLASS_PROCEDURE_STATE ||
            cls == VSS_CLASS_PROCEDURE_RESPONSE ||
            cls == VSS_CLASS_ACK);
}

int policy_check_set(vss_handle_t handle)
{
    if (!is_set_class(vss_registry[handle].class)) {
        LOG_ERR("Set not allowed for %s", vss_registry[handle].path);
        return -EPERM;
    }
    return 0;
}

int policy_check_publish(vss_handle_t handle)
{
    if (!is_publish_class(vss_registry[handle].class)) {
        LOG_ERR("Publish not allowed for %s", vss_registry[handle].path);
        return -EPERM;
    }
    return 0;
}
