#include "dtp/critical_section.h"

void si_init_critical(critical_section_t *me) {

    if (me->isinit) {
        return;
    }
    me->data.hsem = xSemaphoreCreateMutexStatic(&me->data.mutex);
    me->isinit = true;
}

bool si_enter_critical(critical_section_t *me, uint32_t timeout_ticks) {

    if (!me->isinit) {
        return false;
    }
    return xSemaphoreTake(me->data.hsem, timeout_ticks);
}

void si_leave_critical(critical_section_t *me) {

    if (!me->isinit) {
        return;
    }
    xSemaphoreGive(me->data.hsem);
}
