#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <csp/csp.h>
#include "cspftp_log.h"
#include "cspftp_protocol.h"
#include "cspftp_log.h"
#include "cspftp_session.h"

#define PKT_SIZE (200U)

const uint16_t CSPFTP_PACKET_SIZE = PKT_SIZE;

cspftp_server_transfer_ctx_t server_transfer_ctx;


static bool get_payload_meta(dftp_payload_meta_t *meta, uint8_t payload_id) {
    bool result = true;
    switch(payload_id) {
        case 0:
        /* deliberate fallthrough for now */
        default:
            /* 128 Mb for testing */
            meta->size = 128 * 1024 * 1024;
        break;
    }
    return result;
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
    csp_packet_t *packet = csp_read(conn, 10000);
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

static char dummy_payload[PKT_SIZE - sizeof(uint32_t)];

extern cspftp_result start_sending_data(cspftp_server_transfer_ctx_t *ctx)
{
    cspftp_result result = CSPFTP_OK;
    csp_packet_t *packet;
    // TODO: get payload data from somewhere
    uint32_t bytes_sent = 0;
    uint32_t nof_csp_packets = 0;
    memset(dummy_payload, 0x33, sizeof(dummy_payload));
    dbg_log("Start sending data");
    for(uint8_t i = 0; i < ctx->request.nof_intervals; i++)
    {
        uint32_t interval_start = ctx->request.intervals[i].start;
        uint32_t interval_stop = ctx->request.intervals[i].end;
        uint32_t bytes_in_interval = interval_stop - interval_start;
        uint32_t sent_in_interval = 0;
        uint32_t throttler = 0;
        while(sent_in_interval < bytes_in_interval) {
            if((throttler % 250) == 0) {
                usleep(1000);
            }
            throttler++;
            packet = csp_buffer_get(0);
            if(NULL == packet) {
                // Count maybe ?
                dbg_warn("could not get packet to send");
                continue;
            }
            if((bytes_in_interval - sent_in_interval) > (CSPFTP_PACKET_SIZE - sizeof(uint32_t))) {
                packet->length = CSPFTP_PACKET_SIZE;
            } else {
                packet->length = (bytes_in_interval - sent_in_interval) + sizeof(uint32_t);
                dbg_warn("last packet->length= %lu", packet->length);
            }
            memcpy(packet->data + sizeof(uint32_t), dummy_payload, sizeof(dummy_payload));
            memcpy(packet->data, &bytes_sent, sizeof(uint32_t));
            bytes_sent += packet->length - sizeof(uint32_t);
            sent_in_interval += packet->length - sizeof(uint32_t);
            csp_sendto(CSP_PRIO_NORM, ctx->destination, 8, 0, 0, packet);
            nof_csp_packets++;
        }
    }
    dbg_warn("Server transfer completed, sent %lu bytes in %lu packets", bytes_sent, nof_csp_packets);
    return result;
}

uint32_t compute_nof_packets(uint32_t total, uint32_t effective_payload_size) {
    uint32_t expected_nof_packets = total / effective_payload_size;

    /* an extra packet will be needed for the remaining bytes */
    if(total % effective_payload_size) {
        expected_nof_packets++;
    }
    return expected_nof_packets;
}
