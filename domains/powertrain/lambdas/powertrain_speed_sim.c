#include "domains/powertrain/lambdas/powertrain_lambdas.h"
#include "platform/sdk/vo_sdk.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(powertrain_speed_sim, LOG_LEVEL_INF);

void powertrain_speed_sim_init(void)
{
    vss_value_t speed = { .type = VSS_TYPE_FLOAT };
    speed.scalar.f = 0.0f;

    vo_publish(VSS_VEHICLE_SPEED, &speed);
    LOG_INF("Powertrain speed sim started");
}
