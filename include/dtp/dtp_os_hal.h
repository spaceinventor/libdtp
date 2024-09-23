#pragma once

#include <stdbool.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start a periodic poll operation
 * 
 * This method invokes a periodic poll operation on the function pointed to by
 * the poll argument. By using OS specific primitives the operation it kept as
 * tight as possible so the poll function is being called at poll_period_ms
 * intervals.
 * 
 * @param poll_period_ms The period between calls to poll (best effort)
 * @param op The operation argument passed on the each call of the poll function
 * @param poll A pointer to a method which will be called periodically until it returns false
 * @param context A context pointer passed on the each call of the poll function
 */
extern void os_hal_start_poll_operation(uint32_t poll_period_ms, uint32_t op, bool (*poll)(uint32_t op, void *), void *context);

#ifdef __cplusplus
}
#endif