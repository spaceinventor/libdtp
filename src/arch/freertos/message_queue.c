#include <stdint.h>

/* FreeRTOS */
#include "FreeRTOS.h"
#include "queue.h"

#include "dtp/message_queue.h"

void message_queue_create(message_queue_t *me, uint32_t item_size, uint32_t length, void *storage) {

    me->handle = xQueueCreateStatic(length, item_size, storage, (StaticQueue_t *)&me->obj[0]);
    vQueueAddToRegistry(me->handle, "MsgQ");
}

int message_queue_send(message_queue_t *me, void *item) {

    xQueueSend(me->handle, item, portMAX_DELAY);

    return 0;
}

int message_queue_send_isr(message_queue_t *me, void *item) {

    portBASE_TYPE woken = pdFALSE;

    xQueueSendFromISR(me->handle, item, &woken);
    if (woken == pdTRUE) {
        portYIELD_FROM_ISR(woken);
    }

    return 0;
}

int message_queue_receive(message_queue_t *me, void *item) {

    xQueueReceive(me->handle, item, portMAX_DELAY);

    return 0;
}

int message_queue_receive_wto(message_queue_t *me, void *item, uint32_t to_ms) {

    TickType_t ticks = (to_ms / portTICK_PERIOD_MS);
    if (ticks == 0) {
        ticks = 1;
    }
    BaseType_t res = xQueueReceive(me->handle, item, ticks);
    if (res != pdTRUE) {
        return -1;
    }

    return 0;
}
