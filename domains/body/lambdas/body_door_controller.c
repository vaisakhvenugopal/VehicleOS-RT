#include "domains/body/lambdas/body_lambdas.h"
#include "platform/sdk/vo_sdk.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(body_door_controller, LOG_LEVEL_INF);

void body_door_controller_init(void)
{
    vss_value_t ack = { .type = VSS_TYPE_STRUCT };
    vss_value_t locked = { .type = VSS_TYPE_BOOL };

    ack.str = "ACCEPTED";
    locked.scalar.b = true;

    vo_publish(VSS_VEHICLE_BODY_DOOR_FRONTLEFT_LOCKTARGET_ACK, &ack);
    vo_publish(VSS_VEHICLE_BODY_DOOR_FRONTLEFT_ISLOCKED, &locked);

    LOG_INF("Body door controller started");
}
