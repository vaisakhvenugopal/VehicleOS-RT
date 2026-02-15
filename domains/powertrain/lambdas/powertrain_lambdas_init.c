#include "domains/powertrain/lambdas/powertrain_lambdas.h"

void powertrain_lambdas_init(void)
{
    powertrain_speed_sim_init();
    chassis_longitudinal_controller_sim_init();
    chassis_lateral_controller_sim_init();
}
