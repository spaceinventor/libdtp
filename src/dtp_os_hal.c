#include <stdbool.h>
#include <stdio.h>

#include "csp/autoconfig.h"

#if defined (CSP_POSIX)
#include "csp/csp.h"
#elif defined (CSP_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif

#include "dtp/dtp_os_hal.h"

void os_hal_start_poll_operation(uint32_t poll_period_ms, bool (*poll)(uint32_t op, void *), void *context) {

    bool carryon = true;

#if defined (CSP_POSIX)
    uint32_t current_time = csp_get_ms();
    /* On the POSIX platform, this is just a tight loop */
    while (carryon) {
        /* Calculate the sleep period */
        carryon = (*poll)(1, context);
    }
#elif defined (CSP_FREERTOS)
    TickType_t wakeupTime = xTaskGetTickCount();
    TickType_t ticks = pdMS_TO_TICKS(poll_period_ms);

    printf("Poll: Start polling function at: %p every %"PRIu32" ms\n", poll, ticks);

    while (carryon) {
        BaseType_t xWasDelayed = xTaskDelayUntil(&wakeupTime, ticks);
        if (!xWasDelayed) {
            continue;
        }
        /* Call the poll method passed in as argument */
        carryon = (*poll)(1, context);
    }
#endif

}