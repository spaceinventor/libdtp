#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void os_hal_start_poll_operation(uint32_t poll_period_ms, bool (*poll)(uint32_t op, void *), void *context);

#ifdef __cplusplus
}
#endif