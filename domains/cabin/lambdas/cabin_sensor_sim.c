#include "domains/cabin/lambdas/cabin_lambdas.h"
#include "platform/sdk/vo_sdk.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cabin_sensor_sim, LOG_LEVEL_INF);

void cabin_sensor_sim_init(void)
{
    vss_value_t temp = { .type = VSS_TYPE_FLOAT };
    temp.scalar.f = 24.6f;
    vo_publish(VSS_VEHICLE_CABIN_HVAC_CABINTEMPERATURE, &temp);
    LOG_INF("Cabin sensor sim started");
}
