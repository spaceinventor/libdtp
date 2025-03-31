#include <csp/csp.h>
#include <csp/arch/csp_time.h>
#include "dtp/dtp.h"
#include "dtp_internal_api.h"
#include "dtp/dtp_protocol.h"
#include "dtp/dtp_log.h"
#include "dtp/dtp_os_hal.h"
#include "dtp/dtp_async_api.h"

static void dtp_server_run(bool *keep_running)
{
    static csp_socket_t sock = {0};
    sock.opts = CSP_O_RDP;
    csp_bind(&sock, 7);

    /* Create a backlog of 1 connection */
    csp_listen(&sock, 1);

    /* Wait for connections and then process packets on the connection */
    dbg_log("Waiting for connection...");
    while (*keep_running)
    {
        /* Wait for a new connection, 10000 mS timeout */
        csp_conn_t *conn;
        if ((conn = csp_accept(&sock, 10000)) == NULL)
        {
            /* timeout */
            continue;
        }
        dbg_log("Incoming connection");
        /* Read packets on connection, timout is 100 mS */
        csp_packet_t *packet = csp_read(conn, 10000);
        if (NULL == packet) {
            continue;
        }
        dbg_log("Got meta data request");
        packet = setup_server_transfer(&server_transfer_ctx, csp_conn_src(conn), packet);
        server_transfer_ctx.keep_running = keep_running;
        if(packet) {
            csp_send(conn, packet);
            start_sending_data(&server_transfer_ctx);
        }
        csp_close(conn);
        dbg_log("Transfer done");
    }
    dbg_log("Bye");
}

static uint32_t compute_transfer_size(dtp_server_transfer_ctx_t *ctx) {
    uint32_t size = 0;
    interval_t *cur_int;
    uint32_t chunk_size = ctx->request.mtu - sizeof(uint32_t);
    for (uint8_t i=0; i < ctx->request.nof_intervals; i++) {
        cur_int = &ctx->request.intervals[i];
        if(cur_int->end == UINT32_MAX) {
            /* This means the whole shebang */
            size += ctx->payload_meta.size - (cur_int->start * chunk_size);
        } else {
            size += (cur_int->end * chunk_size) - (cur_int->start * chunk_size);
        }
    }
    return size;
}

typedef struct dtp_payload_vmem_transfer_s {
    dtp_msg_t *msg;
    dtp_async_api_t *api;
} dtp_payload_vmem_transfer_t;

static uint32_t dtp_payload_vmem_read(uint8_t payload_id, uint32_t offset, void *output, uint32_t size, void *context) {

    dtp_payload_vmem_transfer_t *txfr = (dtp_payload_vmem_transfer_t *)context;

    (*txfr->api->read)(txfr->api, output, txfr->msg->vaddr + offset, size);

    return size;
} 

static void dtp_vmem_server_run(bool *keep_running, dtp_async_api_t *api)
{

    dtp_server_transfer_ctx_t server_transfer_ctx;

    dbg_log("Starting DTP VMEM Server task.\n");

    /* Wait for connections and then process packets on the connection */
    while (*keep_running)
    {
        dtp_msg_t msg;

        /* Wait for in coming messages on the queue */
        dbg_log("Waiting for instruction on receive queue...");
        (*api->recv)(api, &msg);

        switch (msg.msg) {
            case 0x01: /* DTP_START_VMEM_TRANSFER */
            {
                dbg_log("Got meta data request thru DTP start VMEM transfer");

                server_transfer_ctx.size_in_bytes = compute_transfer_size(&server_transfer_ctx);

                dtp_payload_vmem_transfer_t transfer_obj = { .msg = &msg, .api = api };
                server_transfer_ctx.payload_meta.context = &transfer_obj;
                server_transfer_ctx.payload_meta.read = dtp_payload_vmem_read;
                server_transfer_ctx.payload_meta.completed = NULL;
                server_transfer_ctx.destination = msg.node;
                server_transfer_ctx.keep_running = keep_running;
                /* Start the actual transmission of data. This will block until done.
                It can be stopped by setting the 'keep_running' flag on the object
                passed on to the process. */
                start_sending_data(&server_transfer_ctx);

                dbg_log("Transfer done");
            }
            break;
        }
    }
    dbg_log("Bye");
}

typedef struct dtp_server_transfer_s {
    bool keep_running;
    uint32_t bytes_sent;
    uint32_t sent_in_interval;
    uint32_t bytes_in_interval;
    uint32_t nof_packets_per_round;
    uint32_t nof_csp_packets;
    dtp_server_transfer_ctx_t *ctx;
} dtp_server_transfer_t;

static bool dtp_server_poll_loop(uint32_t op, void *context) {

    bool carryon = true;
    csp_packet_t *packet;
    dtp_server_transfer_t * transfer = (dtp_server_transfer_t *)context;

    uint32_t pkt_cnt = 0;
    while (transfer->nof_packets_per_round > pkt_cnt) {

        uint32_t data_read;

        packet = csp_buffer_get(0);
        if(NULL == packet) {
            dbg_warn("could not get packet to send");
            break;
        }

        // Calculate the up-coming packet length
        if((transfer->bytes_in_interval - transfer->sent_in_interval) > (transfer->ctx->request.mtu - sizeof(uint32_t))) {
            packet->length = transfer->ctx->request.mtu;
        } else {
            // This is the last packet to send, its size is most likely not == ctx->request.mtu - sizeof(uint32_t)
            packet->length = (transfer->bytes_in_interval - transfer->sent_in_interval) + sizeof(uint32_t);
            // Last packet in the entire transfer
            pkt_cnt = transfer->nof_packets_per_round;
        }

        // Put the amount of bytes sent in the first double word
        memcpy(packet->data, &transfer->bytes_sent, sizeof(uint32_t));

        // Get the packet payload data from the "user"
        data_read = transfer->ctx->payload_meta.read(transfer->ctx->request.payload_id, transfer->bytes_sent, packet->data + sizeof(uint32_t), packet->length - sizeof(uint32_t), transfer->ctx->payload_meta.context);
        if (data_read == 0){
            dbg_warn("could not read data from user");
            break;
        }

        // Update the transmit metrics
        transfer->bytes_sent += data_read;//packet->length - sizeof(uint32_t);
        transfer->sent_in_interval += data_read;//packet->length - sizeof(uint32_t);

        // TODO: The priority parameter might need to be adjusted according to payload meta-data, though it may not have any impact
        // on actual speed transfer at all.
        csp_sendto(CSP_PRIO_NORM, transfer->ctx->destination, 8 /* DTP DATA PORT */, 0, 0, packet);
        transfer->nof_csp_packets++;
        pkt_cnt++;
    }

    /* Decide if we need to keep carrying on with poll'ing */
    carryon = (transfer->sent_in_interval < transfer->bytes_in_interval) && *(transfer->ctx->keep_running);

    return carryon;

}

extern dtp_result start_sending_data(dtp_server_transfer_ctx_t *ctx)
{
    dtp_result result = DTP_OK;
    dtp_server_transfer_t transfer;

    transfer.ctx = ctx;
    transfer.bytes_sent = 0;
    transfer.nof_csp_packets = 0;
    dbg_log("Start sending data, setting max throughput to %u KB/sec", ctx->request.throughput);
    uint32_t max_throughput = ctx->request.throughput * 1024; // In bytes/second
    uint32_t packets_second = (max_throughput / ctx->request.mtu);
    dbg_log("Throughput in packets/sec: %u", packets_second);

    dbg_log("Number of intervals: %u", ctx->request.nof_intervals);
    for(uint8_t i = 0; i < ctx->request.nof_intervals && *(ctx->keep_running); i++)
    {
        uint32_t interval_start = ctx->request.intervals[i].start * (ctx->request.mtu - sizeof(uint32_t));
        uint32_t interval_stop = ctx->request.intervals[i].end * (ctx->request.mtu - sizeof(uint32_t));
        transfer.sent_in_interval = 0;
        transfer.bytes_sent = interval_start;
        if(interval_stop > ctx->payload_meta.size) {
            interval_stop = ctx->payload_meta.size;
            uint32_t fixed_end;
            if (interval_stop % (ctx->request.mtu - sizeof(uint32_t)) != 0) {
                fixed_end = interval_stop / (ctx->request.mtu - sizeof(uint32_t)) + 1;
            } else {
                fixed_end = interval_stop / (ctx->request.mtu - sizeof(uint32_t));
            }
            ctx->request.intervals[i].end = fixed_end;
        }        
        transfer.bytes_in_interval = interval_stop - interval_start;
        dbg_log("interval_start: %u (seq: %u)", interval_start, ctx->request.intervals[i].start);
        dbg_log("interval_stop: %u (seq: %u)", interval_stop, ctx->request.intervals[i].end);
        dbg_log("bytes_in_interval: %u", transfer.bytes_in_interval);

        uint32_t round_time = 100;
        transfer.nof_packets_per_round = (packets_second * round_time) / 1000;
        dbg_log("Sending %d packets every %d ms\n", transfer.nof_packets_per_round, round_time);

        /* Trigger a poll'ing operation for this interval */
        os_hal_start_poll_operation(round_time, 0, dtp_server_poll_loop, &transfer);

        dbg_log("sent_in_interval: %u", transfer.sent_in_interval);
    }
    if(!*(ctx->keep_running)) {
        dbg_warn("Transfer was interrupted");
    }
    dbg_warn("nof_csp_packets= %lu", transfer.nof_csp_packets);
    dbg_warn("Server transfer completed, sent %lu packets", transfer.nof_csp_packets);

    /* Signal completion to the user, if possible */
    if (transfer.ctx->payload_meta.completed) {
        (*transfer.ctx->payload_meta.completed)(transfer.ctx->request.payload_id, transfer.bytes_sent, transfer.ctx->payload_meta.context);
    }

    return result;
}

csp_packet_t *setup_server_transfer(dtp_server_transfer_ctx_t *ctx, uint16_t dst, csp_packet_t *request) {
    csp_packet_t *result = 0;

    memcpy(&(ctx->request), (dtp_meta_req_t *)request->data, sizeof(dtp_meta_req_t));
    ctx->destination = dst;

    /* Get the payload information */
    if (false == get_payload_meta(&ctx->payload_meta, ctx->request.payload_id)) {
        return result;
    }

    ctx->size_in_bytes = compute_transfer_size(ctx);

    csp_buffer_free(request);
    result = csp_buffer_get(0);
    if (result) {
        /* prepare the response */
        result->length = sizeof(dtp_meta_req_t);
        dtp_meta_resp_t *meta_resp = (dtp_meta_resp_t *)result->data;
        meta_resp->total_payload_size = ctx->payload_meta.size;
        meta_resp->size_in_bytes = ctx->size_in_bytes;
    }
    return result;
}

int dtp_server_main(bool *keep_running)
{
    dtp_server_run(keep_running);
    return 0;
}

int dtp_vmem_server_main(bool *keep_running, dtp_async_api_t *api)
{
    dtp_vmem_server_run(keep_running, api);
    return 0;
}
