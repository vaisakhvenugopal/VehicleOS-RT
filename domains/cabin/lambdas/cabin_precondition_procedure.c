#include "domains/cabin/lambdas/cabin_lambdas.h"
#include "platform/sdk/vo_sdk.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(cabin_precondition_proc, LOG_LEVEL_INF);

void cabin_precondition_procedure_init(void)
{
    vss_value_t req = { .type = VSS_TYPE_STRUCT };
    vss_value_t state = { .type = VSS_TYPE_UINT32 };
    vss_value_t resp = { .type = VSS_TYPE_STRUCT };
    vss_value_t target = { .type = VSS_TYPE_FLOAT };

    req.str = "START";
    state.scalar.u32 = 1;
    resp.str = "OK";
    target.scalar.f = 22.0f;

    vo_set(VSS_VEHICLE_CABIN_PRECONDITION_REQUEST, &req);
    vo_publish(VSS_VEHICLE_CABIN_PRECONDITION_STATE, &state);
    vo_set(VSS_VEHICLE_CABIN_HVAC_TARGETTEMPERATURE, &target);
    vo_publish(VSS_VEHICLE_CABIN_PRECONDITION_RESPONSE, &resp);

    LOG_INF("Cabin precondition procedure started");
}
