#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <csp/csp.h>
#include "cspftp_log.h"
#include "cspftp_protocol.h"
#include "cspftp_log.h"
#include "cspftp_session.h"

#define PKT_SIZE (190U)
#define NOF_PACKETS (1024 )

cspftp_server_transfer_ctx_t server_transfer_ctx;

csp_packet_t *setup_server_transfer(cspftp_server_transfer_ctx_t *ctx, uint16_t dst, csp_packet_t *request) {
    csp_packet_t *result = 0;
    memcpy(&(ctx->request), (cspftp_meta_req_t *)request->data, sizeof(cspftp_meta_req_t));
    ctx->destination = dst;
    if(ctx->request.nof_intervals == 1 && ctx->request.intervals[0].end == 0xFFFFFFFF) {
        // First request, client is asking for the whole thing, adjust to actual size to transfer
        ctx->request.intervals[0].end = (uint32_t)(NOF_PACKETS * PKT_SIZE);
    }
    csp_buffer_free(request);
    result = csp_buffer_get(0);
    if (result) {
        /* prepare the response */
        result->length = sizeof(cspftp_meta_req_t);
        cspftp_meta_resp_t *meta_resp = (cspftp_meta_resp_t *)result->data;
        /* TODO: get size in bytes of the transfer from somewhere */
        meta_resp->size_in_bytes = (uint32_t)(NOF_PACKETS * PKT_SIZE);
    }
    return result;
}

cspftp_result send_remote_meta_req(cspftp_t *session)
{
    cspftp_result res = CSPFTP_ERR;
    csp_packet_t *packet = csp_buffer_get(0);
    if (packet)
    {
        packet->length = sizeof(cspftp_meta_req_t);
        cspftp_meta_req_t *req = (cspftp_meta_req_t *)packet->data;
        memcpy(req, &(session->request_meta), sizeof(cspftp_meta_req_t));
        csp_send(session->conn, packet);
        res = CSPFTP_OK;
    }
    return res;
}

cspftp_result read_remote_meta_resp(cspftp_t *session)
{
    cspftp_result res = CSPFTP_ERR;
    csp_conn_t *conn = session->conn;
    csp_packet_t *packet = csp_read(conn, 1000);
    if (packet)
    {
        res = CSPFTP_OK;
        cspftp_meta_resp_t *meta_resp = (cspftp_meta_resp_t *)packet->data;
        session->total_bytes = meta_resp->size_in_bytes;
        dbg_log("Setting session total bytes to %u", session->total_bytes);
        csp_buffer_free(packet);
    } else {
        session->errno = CSPFTP_CONNECTION_FAILED;
    }
    return res;
}

extern cspftp_result start_sending_data(cspftp_server_transfer_ctx_t *ctx)
{
    cspftp_result result = CSPFTP_OK;
    csp_packet_t *packet;
    // TODO: get payload data from somewhere
    const char dummy_payload[PKT_SIZE] = { 0x00 };
    uint32_t bytes_sent = 0;
    dbg_log("Start sending data");
    for(uint8_t i = 0; i < ctx->request.nof_intervals; i++)
    {
        uint32_t interval_start = ctx->request.intervals[i].start;
        uint32_t interval_stop = ctx->request.intervals[i].end;
        uint32_t bytes_in_interval = interval_stop - interval_start;
        uint32_t sent_in_interval = 0;
        while(sent_in_interval < bytes_in_interval) {
            packet = csp_buffer_get(0);
            if(NULL == packet) {
                // Count maybe ?
                continue;
            }
            packet->length = PKT_SIZE;
            // dbg_log("Sent %d bytes, seq = %lu", bytes_sent, bytes_sent / PKT_SIZE);
            memcpy(packet->data, dummy_payload, sizeof(dummy_payload));
            memcpy(packet->data, &bytes_sent, sizeof(bytes_sent));
            bytes_sent += packet->length;
            sent_in_interval += packet->length;
            csp_sendto(CSP_PRIO_NORM, ctx->destination, 8, 0, 0, packet);
        }
    }
    dbg_warn("Server transfer completed, sent %d bytes in %lu packets", bytes_sent, bytes_sent / PKT_SIZE);
    return result;
}
