#include <stdbool.h>
#include <stdio.h>

#include "csp/autoconfig.h"

#if defined (CSP_POSIX)
#error "The OS hal for poll'ing is not implemented for POSIX platforms"
#elif defined (CSP_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif

#include "dtp/dtp_os_hal.h"

void os_hal_start_poll_operation(uint32_t poll_period_ms, bool (*poll)(uint32_t op, void *), void *context) {

    TickType_t wakeupTime = xTaskGetTickCount();
    TickType_t ticks = pdMS_TO_TICKS(poll_period_ms);
    bool carryon = true;

    printf("Poll: Start polling function at: %p every %"PRIu32" ms\n", poll, ticks * configTICK_RATE_HZ);

    while (carryon) {
        BaseType_t xWasDelayed = xTaskDelayUntil(&wakeupTime, ticks);
        if (!xWasDelayed) {
            continue;
        }
        /* Call the poll method passed in as argument */
        carryon = (*poll)(1, context);
    }

}