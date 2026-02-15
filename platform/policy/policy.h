#ifndef VEHICLEOS_POLICY_H
#define VEHICLEOS_POLICY_H

#include "vss_registry.h"

#ifdef __cplusplus
extern "C" {
#endif

int policy_check_set(vss_handle_t handle);
int policy_check_publish(vss_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif
