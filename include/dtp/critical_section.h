#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "libdtp.h"

#ifdef LIBDTP_FREERTOS
#include "FreeRTOS.h"
#include "semphr.h"
#endif
#ifdef LIBDTP_POSIX
#include <semaphore.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LIBDTP_FREERTOS
#define SI_CRITICAL_WAIT_FOREVER portMAX_DELAY
#define SI_CRITICAL_WAIT_MS(_mS) pdMS_TO_TICKS(_mS)
#endif

#ifdef LIBDTP_POSIX
#define SI_CRITICAL_WAIT_FOREVER UINT32_MAX
#define SI_CRITICAL_WAIT_MS(_mS) _mS
#endif

typedef struct critical_section_s critical_section_t;

typedef struct csection_data_s {
#ifdef LIBDTP_FREERTOS
    SemaphoreHandle_t hsem;
    StaticSemaphore_t mutex;
#endif
#ifdef LIBDTP_POSIX
    sem_t hsem;
#endif
} csection_data_t;

typedef struct critical_section_s {
    bool isinit;
    csection_data_t data;
} critical_section_t;

extern void si_init_critical(critical_section_t *me);
extern bool si_enter_critical(critical_section_t *me, uint32_t timeout_ticks);
extern void si_leave_critical(critical_section_t *me);

#ifdef __cplusplus
}
#endif
