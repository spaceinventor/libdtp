#include <inttypes.h>
#include <stdio.h>

#include <semaphore.h>
#include <time.h>

#include "dtp/critical_section.h"

void si_init_critical(critical_section_t *me) {

    if (me->isinit) {
        return;
    }
    if (sem_init(&me->data.hsem, 0, 1)) {
        printf("SI Critical Section: Failed to initialize semaphore\n");
        return;
    }
    me->isinit = true;
}

bool si_enter_critical(critical_section_t *me, uint32_t timeout_ticks) {

    if (!me->isinit) {
        return false;
    }
    int res;
    if (timeout_ticks == SI_CRITICAL_WAIT_FOREVER) {
        res = sem_wait(&me->data.hsem);
    } else {
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts)) {
            return false;
        }

        uint32_t sec = timeout_ticks / 1000;
        uint32_t nsec = (timeout_ticks - 1000 * sec) * 1000000;

        ts.tv_sec += sec;

        if (ts.tv_nsec + nsec >= 1000000000) {
            ts.tv_sec++;
        }

        ts.tv_nsec = (ts.tv_nsec + nsec) % 1000000000;

        res = sem_timedwait(&me->data.hsem, &ts);
    }

    if (res == 0) {
        return true;
    } else {
        printf("SI Critical Section: Failed to enter critical section\n");
        return false;
    }
}

void si_leave_critical(critical_section_t *me) {

    if (!me->isinit) {
        return;
    }
    int value;
    sem_getvalue(&me->data.hsem, &value);
    if (value > 0) {
        return;
    }

    sem_post(&me->data.hsem);
}
