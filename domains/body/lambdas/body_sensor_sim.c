#include "domains/body/lambdas/body_lambdas.h"
#include "platform/sdk/vo_sdk.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(body_sensor_sim, LOG_LEVEL_INF);

void body_sensor_sim_init(void)
{
    vss_value_t is_open = { .type = VSS_TYPE_BOOL };
    is_open.scalar.b = false;

    vo_publish(VSS_VEHICLE_BODY_DOOR_FRONTLEFT_ISOPEN, &is_open);
    LOG_INF("Body sensor sim started");
}
