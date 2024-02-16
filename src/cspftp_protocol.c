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
    struct timespec sleep_for = {
        .tv_sec = 0,
        .tv_nsec = 1000
    };
    for(uint8_t i = 0; i < ctx->request.nof_intervals; i++)
    {
        uint32_t interval_start = ctx->request.intervals[i].start;
        uint32_t interval_stop = ctx->request.intervals[i].end;
        uint32_t bytes_in_interval = interval_stop - interval_start;
        uint32_t sent_in_interval = 0;
        while(sent_in_interval < bytes_in_interval) {
            nanosleep(&sleep_for, NULL);
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

extern cspftp_result cspftp_start_transfer(cspftp_t *session)
{
    cspftp_result res = CSPFTP_ERR;
    csp_conn_t *conn = csp_connect(CSP_PRIO_HIGH, session->remote_cfg.node, 7, 5, CSP_O_RDP);
    if (NULL == conn) {
        session->errno = CSPFTP_CONNECTION_FAILED;
        goto get_out;
    }
    session->conn = conn;
    res = send_remote_meta_req(session);    
    if (CSPFTP_OK != res)
    {
        goto get_out;
    }
    res = read_remote_meta_resp(session);
    if (CSPFTP_OK != res)
    {
        goto get_out;
    }
    res = start_receiving_data(session);
get_out:
    return res;
}

extern cspftp_result start_receiving_data(cspftp_t *session)
{
    cspftp_result result = CSPFTP_OK;
    csp_packet_t *packet;
    uint32_t current_seq = 0;
    uint32_t packet_seq = 0;
    uint32_t idle_ms = 0;
    csp_socket_t *socket = malloc(sizeof(csp_socket_t));
    socket->opts = CSP_SO_CONN_LESS;

    dbg_log("Start receiving data");
    if(CSP_ERR_NONE == csp_bind(socket, 8)) {
        csp_listen(socket, 1);
        while ((session->bytes_received < session->total_bytes) && (idle_ms < 6000))
        {
            packet = csp_recvfrom(socket, 1000);
            if(NULL == packet) {
                idle_ms += 1;
                if (idle_ms % 1000 == 0)
                dbg_warn("No data received for %u ms", idle_ms);
                // Count maybe ?
                continue;
            }
            idle_ms = 0;
            session->bytes_received += packet->length;
            packet_seq = packet->data32[0] / packet->length;
            if (current_seq != packet_seq) {
                // dbg_warn("Break here, current_seq=%d, received_seq=%d", current_seq, (packet->data32[0] / PKT_SIZE));
            } else {                
                // dbg_log("Received %d bytes, seq = %lu", session->bytes_received, (packet->data32[0]) / PKT_SIZE);
            }
            current_seq++;            
            csp_buffer_free(packet);
            if (packet_seq == 1023) {
                break;
            }
        }
        if (idle_ms > 6000) {
            dbg_warn("No data received for %u ms, bailing out", idle_ms);
        }
        csp_socket_close(socket);
        free(socket);
    } else {
        result = CSPFTP_ERR;
    }
    dbg_log("Received %d bytes, status: %d", session->bytes_received, result);
    return result;
}
