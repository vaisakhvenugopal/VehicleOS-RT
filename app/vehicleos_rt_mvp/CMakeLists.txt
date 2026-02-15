cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(vehicleos_rt_mvp)

target_sources(app PRIVATE
  src/main.c
  ../../platform/store/store.c
  ../../platform/bus/bus.c
  ../../platform/policy/policy.c
  ../../platform/gateway/gateway.c
  ../../platform/sdk/vo_sdk.c
  ../../generated/signals/vss_registry.c
  ../../domains/cabin/lambdas/cabin_sensor_sim.c
  ../../domains/cabin/lambdas/cabin_hvac_controller.c
  ../../domains/cabin/lambdas/cabin_precondition_procedure.c
  ../../domains/cabin/lambdas/cabin_lambdas_init.c
  ../../domains/body/lambdas/body_door_controller.c
  ../../domains/body/lambdas/body_sensor_sim.c
  ../../domains/body/lambdas/body_lambdas_init.c
  ../../domains/powertrain/lambdas/powertrain_speed_sim.c
  ../../domains/powertrain/lambdas/chassis_longitudinal_controller_sim.c
  ../../domains/powertrain/lambdas/chassis_lateral_controller_sim.c
  ../../domains/powertrain/lambdas/powertrain_lambdas_init.c
  ../../tools/cli_client.c
)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/../../
  ${CMAKE_CURRENT_LIST_DIR}/../../generated/signals
)
