#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#include "csp/csp.h"

#include "dtp/dtp_async_api.h"
#include "dtp/dtp_vmem_server.h"

#include "ossi/message_queue.h"

#include "vmem/vmem.h"
#include "vmem/vmem_server.h"

typedef struct dtp_vmem_context_s {
    bool carryon;
    message_queue_t queue;
    uint8_t queue_storage[10 * sizeof(dtp_msg_t)];
    vmem_handler_obj_t vmem_handler;
} dtp_vmem_context_t;

static int vmem_dtp_request_handler(csp_conn_t *conn, csp_packet_t *packet, void *context) {

    /* This method gets called when the VMEM server receives a request on its port
    with the request type of 0x80. We need to unpack the DTP meta data and setup
    the actual transfer. When the DTP transfer has been started, it will send packets
    to port 8 at the initiating node. */

    dtp_vmem_request_t *vmem_request = (void *)packet->data;
    dtp_msg_t msg;
    dtp_vmem_context_t *vmem_context = (dtp_vmem_context_t *)context;

    switch (vmem_request->type) {
        case  DTP_REQUEST_START_TRANSFER:
        {
            dtp_start_req_t *request = (dtp_start_req_t *)&vmem_request->body[0];
            uint32_t chunk_size = be16toh(request->meta.mtu) - sizeof(uint32_t);

            printf("Received DTP VMEM start transfer request.\n");
            printf("\tSession ID: %"PRIu32"\n", be32toh(request->session_id));
            printf("\tMTU: %"PRIu16"\n", be16toh(request->meta.mtu));
            msg.meta.mtu = be16toh(request->meta.mtu);
            msg.meta.throughput = be32toh(request->meta.throughput);
            msg.meta.payload_id = request->meta.payload_id;
            msg.meta.nof_intervals = request->meta.nof_intervals;
            printf("\tStart address: 0x%016"PRIX64"\n", be64toh(request->vaddr));
            printf("\tSize: %"PRIu32"\n", be32toh(request->size));
            printf("\tChunk size: %"PRIu32"\n", chunk_size);
            printf("\tnof_intervals: %"PRIu8"\n", request->meta.nof_intervals);
            for (uint8_t i=0;i<request->meta.nof_intervals;i++) {
                interval_t interval;
                memcpy(&interval, &request->meta.intervals[i], sizeof(interval_t));
                interval.start = be32toh(interval.start);
                interval.end = be32toh(interval.end);
                msg.meta.intervals[i] = interval;
                printf("\tinterval[%d]: %"PRIu32" - %"PRIu32"\n", i, interval.start, interval.end);
            }

            msg.msg = 0x01; /* DTP_START_VMEM_TRANSFER */
            msg.node = csp_conn_src(conn);
            msg.vaddr = be64toh(request->vaddr);
            msg.size = be32toh(request->size);

            message_queue_send(&vmem_context->queue, &msg);
        }
        break;

        case DTP_REQUEST_STOP_TRANSFER:
        {
            dtp_stop_req_t *request = (dtp_stop_req_t *)&vmem_request->body[0];
            printf("Received DTP VMEM stop transfer request.\n");
            printf("\tSession ID: %"PRIu32"\n", be32toh(request->session_id));

            /* Stop the transfer */
            printf("Stopping the DTP transfer...\n");
            vmem_context->carryon = false;
        }
        break;
    }

    csp_buffer_free(packet);

    return 0;

}

static void async_recv(dtp_async_api_t *me, dtp_msg_t *msg) {

    message_queue_t *queue = (message_queue_t *)me->context;
    message_queue_receive(queue, msg);
}

static uint32_t async_vmem_read(dtp_async_api_t *me, uint8_t *to_addr, uint64_t from_vaddr, uint32_t size) {

    vmem_read(to_addr, from_vaddr, size);

    return size;
}

void dtp_vmem_server_task(void * param) {

    static dtp_vmem_context_t dtp_vmem_context;
    static dtp_async_api_t dtp_api = {
        .recv = async_recv,
        .send = NULL,
        .read = async_vmem_read,
        .context = &dtp_vmem_context.queue,
    };

    /* Set the task to be carrying on by default */
    dtp_vmem_context.carryon = true;

    /* Create the message queue to be used for async task communication */
    message_queue_create(&dtp_vmem_context.queue, sizeof(dtp_msg_t), 10, &dtp_vmem_context.queue_storage[0]);

    /* Hook up the VMEM request type for the DTP server */
    vmem_server_bind_type(0x80 /* VMEM_SERVER_DTP_REQUEST */, vmem_dtp_request_handler, &dtp_vmem_context.vmem_handler, &dtp_vmem_context);

    /* Start the DTP VMEM server main task - it will newer return */
    dtp_vmem_server_main(&dtp_vmem_context.carryon, &dtp_api);

}