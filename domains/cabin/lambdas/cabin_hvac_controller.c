#include "domains/cabin/lambdas/cabin_lambdas.h"
#include "platform/sdk/vo_sdk.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cabin_hvac_controller, LOG_LEVEL_INF);

void cabin_hvac_controller_init(void)
{
    vss_value_t ack = { .type = VSS_TYPE_STRUCT };
    vss_value_t state = { .type = VSS_TYPE_FLOAT };

    ack.str = "ACCEPTED";
    state.scalar.f = 22.0f;

    vo_publish(VSS_VEHICLE_CABIN_HVAC_TARGETTEMPERATURE_ACK, &ack);
    vo_publish(VSS_VEHICLE_CABIN_HVAC_TARGETTEMPERATURE_STATE, &state);
    LOG_INF("Cabin HVAC controller started");
}
