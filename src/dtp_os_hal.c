#include <stdbool.h>
#include <stdio.h>

#include "csp/autoconfig.h"

#if defined (CSP_POSIX)
#include <unistd.h>
#include "csp/arch/csp_time.h"
#elif defined (CSP_FREERTOS)
#include "FreeRTOS.h"
#include "task.h"
#endif

#include "dtp/dtp_os_hal.h"

void os_hal_start_poll_operation(uint32_t poll_period_ms, uint32_t op, bool (*poll)(uint32_t op, void *), void *context) {

    bool carryon = true;

#if defined (CSP_POSIX)
    uint32_t wakeupTime = csp_get_ms();
    /* On the POSIX platform, this is just a tight loop */
    while (carryon) {
        /* Calculate the sleep period */
        uint32_t time_since_last = csp_get_ms() - wakeupTime;
        uint32_t sleep_period;
        if (poll_period_ms > time_since_last) {
            /* Calculate remaining sleep period */
            sleep_period = (poll_period_ms - time_since_last);
            usleep(sleep_period * 1000);
        } else {
            /* We have overslept, so go right ahead and do the poll */
        }
        /* Update the wakeup time */
        wakeupTime = csp_get_ms();
        /* Execute the poll operation */
        carryon = (*poll)(op, context);
    }
#elif defined (CSP_FREERTOS)
    TickType_t wakeupTime = xTaskGetTickCount();
    TickType_t ticks = pdMS_TO_TICKS(poll_period_ms);

    printf("Poll: Start polling function at: %p every %"PRIu32" tick(s)\n", poll, ticks);

    while (carryon) {
        BaseType_t xWasDelayed = xTaskDelayUntil(&wakeupTime, ticks);
        if (!xWasDelayed) {
            continue;
        }
        /* Call the poll method passed in as argument */
        carryon = (*poll)(op, context);
    }
#endif

}