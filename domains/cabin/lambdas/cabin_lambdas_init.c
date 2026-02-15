#include "domains/cabin/lambdas/cabin_lambdas.h"

void cabin_lambdas_init(void)
{
    cabin_sensor_sim_init();
    cabin_hvac_controller_init();
    cabin_precondition_procedure_init();
}
