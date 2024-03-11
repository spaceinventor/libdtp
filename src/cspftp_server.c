#include <csp/csp.h>
#include <csp/arch/csp_time.h>
#include "cspftp/cspftp.h"
#include "cspftp_internal_api.h"
#include "cspftp_protocol.h"
#include "cspftp_log.h"

static void cspftp_server_run(bool *keep_running)
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
        if(packet) {
            csp_send(conn, packet);
            start_sending_data(&server_transfer_ctx);
        }
        csp_close(conn);
        dbg_log("Transfer done");
    }
    dbg_log("Bye");
}

static uint32_t compute_transfer_size(cspftp_server_transfer_ctx_t *ctx) {
    uint32_t size = 0;
    interval_t *cur_int;
    for (uint8_t i=0; i < ctx->request.nof_intervals; i++) {
        cur_int = &ctx->request.intervals[i];
        if(cur_int->end == 0xFFFFFFFF) {
            /* This means the whole shebang */            
            size = ctx->payload_meta.size;
            cur_int->end = size;
            break;
        } else {
            size += cur_int->end - cur_int->start;
        }
    }
    return size;
}

extern cspftp_result start_sending_data(cspftp_server_transfer_ctx_t *ctx)
{
    cspftp_result result = CSPFTP_OK;
    csp_packet_t *packet;
    uint32_t bytes_sent = 0;
    uint32_t nof_csp_packets = 0;
    dbg_log("Start sending data, setting max throughput to %u MBits/sec", ctx->request.throughput);
    uint32_t first_packet_ts = csp_get_ms();
    uint32_t now;
    uint32_t current_throughput = 0;
    uint32_t max_throughput = ctx->request.throughput * 1000 / 8; // In bytes/second
    uint32_t packets_second = (max_throughput / CSPFTP_PACKET_SIZE);
    dbg_log("Throughput in packets/sec: %u", packets_second);

    bool throttling = false;

    for(uint8_t i = 0; i < ctx->request.nof_intervals; i++)
    {
        uint32_t interval_start = ctx->request.intervals[i].start;
        uint32_t interval_stop = ctx->request.intervals[i].end;
        uint32_t bytes_in_interval = interval_stop - interval_start;
        uint32_t sent_in_interval = 0;
        while(sent_in_interval < bytes_in_interval) {

            now = csp_get_ms();
            if((now - first_packet_ts) < 500) {
                // TODO: Could sleep instead of spinning the CPU for now - first_packet_ts millisenconds,
                // this requires a platform-agnostic sleep mechanism.
                continue;
            }
            if(!throttling) {
                current_throughput = compute_throughput(now, first_packet_ts, bytes_sent) * 1000;
                throttling = current_throughput > max_throughput;
                if (throttling) {
                    // TODO: Could sleep instead of spinning the CPU for current_throughput - max_throughput millisenconds,
                    // this requires a platform-agnostic sleep mechanism.
                    continue;
                }
            } else {
                current_throughput = compute_throughput(now, first_packet_ts, bytes_sent) * 1000;
                throttling = current_throughput > max_throughput;
                if (throttling) {
                    // TODO: Could sleep instead of spinning the CPU for current_throughput - max_throughput millisenconds,
                    // this requires a platform-agnostic sleep mechanism.
                    continue;
                }
            }
            packet = csp_buffer_get(0);
            if(NULL == packet) {
                dbg_warn("could not get packet to send");
                continue;
            }
            nof_csp_packets++;
            if((bytes_in_interval - sent_in_interval) > (CSPFTP_PACKET_SIZE - sizeof(uint32_t))) {
                packet->length = CSPFTP_PACKET_SIZE;
            } else {
                // This is the last packet to send, its size is most likely not == CSPFTP_PACKET_SIZE - sizeof(uint32_t)
                packet->length = (bytes_in_interval - sent_in_interval) + sizeof(uint32_t);
            }
            memcpy(packet->data, &bytes_sent, sizeof(uint32_t));
            ctx->payload_meta.read(ctx->request.payload_id, bytes_sent, packet->data + sizeof(uint32_t), packet->length - sizeof(uint32_t));
            bytes_sent += packet->length - sizeof(uint32_t);
            sent_in_interval += packet->length - sizeof(uint32_t);
            // TODO: The priority parameter might need to be adjusted according to payload meta-data, though it may not any any impact
            // on actual speed transfer at all.
            csp_sendto(CSP_PRIO_NORM, ctx->destination, 8, 0, 0, packet);
        }
    }
    dbg_warn("last packet->length= %lu, nof_csp_packets= %lu", packet->length, nof_csp_packets);
    dbg_warn("Server transfer completed, sent %lu bytes in %lu packets", bytes_sent, nof_csp_packets);
    return result;
}

csp_packet_t *setup_server_transfer(cspftp_server_transfer_ctx_t *ctx, uint16_t dst, csp_packet_t *request) {
    csp_packet_t *result = 0;

    memcpy(&(ctx->request), (cspftp_meta_req_t *)request->data, sizeof(cspftp_meta_req_t));
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
        result->length = sizeof(cspftp_meta_req_t);
        cspftp_meta_resp_t *meta_resp = (cspftp_meta_resp_t *)result->data;
        meta_resp->total_payload_size = ctx->payload_meta.size;
        meta_resp->size_in_bytes = ctx->size_in_bytes;
    }
    return result;
}

int dtp_server_main(bool *keep_running)
{
    cspftp_server_run(keep_running);
    return 0;
}