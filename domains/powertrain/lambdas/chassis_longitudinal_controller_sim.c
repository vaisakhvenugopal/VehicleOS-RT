#include "domains/powertrain/lambdas/powertrain_lambdas.h"
#include "platform/sdk/vo_sdk.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(chassis_long_controller, LOG_LEVEL_INF);

void chassis_longitudinal_controller_sim_init(void)
{
    vss_value_t ack = { .type = VSS_TYPE_STRUCT };
    vss_value_t state = { .type = VSS_TYPE_FLOAT };

    ack.str = "ACCEPTED";
    state.scalar.f = 0.0f;

    vo_publish(VSS_VEHICLE_CHASSIS_LONGITUDINAL_ACCELTARGET_ACK, &ack);
    vo_publish(VSS_VEHICLE_CHASSIS_LONGITUDINAL_ACCELSTATE, &state);
    LOG_INF("Chassis longitudinal controller sim started");
}
