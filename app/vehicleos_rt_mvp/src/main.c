#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "vss_handles.h"
#include "platform/store/store.h"
#include "platform/bus/bus.h"
#include "platform/gateway/gateway.h"
#include "domains/cabin/lambdas/cabin_lambdas.h"
#include "domains/body/lambdas/body_lambdas.h"
#include "domains/powertrain/lambdas/powertrain_lambdas.h"
#include "tools/cli_client.h"

LOG_MODULE_REGISTER(vehicleos_main, LOG_LEVEL_INF);

/* 
 * MVP Main Entry Point
 * Initializes Store, Bus, and Lambdas
 */
int main(void)
{
    LOG_INF("VehicleOS-RT MVP Booting...");
    
    // 1. Init Store
    store_init();

    // 2. Init Z-bus Backbone (stub)
    bus_init();

    // 3. Init Gateway (stub)
    gateway_init();

    // 4. Init Lambdas
    cabin_lambdas_init();
    body_lambdas_init();
    powertrain_lambdas_init();

    // 5. Init CLI client (stub)
    cli_client_init();

    LOG_INF("VehicleOS-RT MVP Ready. Waiting for commands...");
    
    return 0;
}
