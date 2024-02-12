#define _GNU_SOURCE // nanosleep
#include <time.h>
#include <string.h>
#include <csp/csp.h>
#include "cspftp_log.h"
#include "cspftp_protocol.h"
#include "cspftp_log.h"
#include "cspftp_session.h"

cspftp_result send_remote_meta_req(cspftp_t *session)
{
    cspftp_result res = CSPFTP_ERR;
    csp_packet_t *packet = csp_buffer_get(0);
    if (packet)
    {
        packet->length = sizeof(cspftp_meta_req_t);
        cspftp_meta_req_t *req = (cspftp_meta_req_t *)packet->data;
        req->ts = 0xff00ff00;
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
        csp_buffer_free(packet);
    }
    return res;
}

#define PKT_SIZE (CSP_BUFFER_SIZE)

extern cspftp_result start_sending_data(uint16_t dest)
{
    cspftp_result result = CSPFTP_OK;
    csp_packet_t *packet;
    dbg_log("Start sending data");
    // TODO: get payload data from somewhere
    const char dummy_payload[PKT_SIZE] = { 0x00 };
    uint32_t bytes_sent = 0;
    struct timespec sleep_for = {
        .tv_sec = 0,
        .tv_nsec = 100000
    };
    while (bytes_sent < 1024 * PKT_SIZE)
    {
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
        csp_sendto(CSP_PRIO_NORM, dest, 8, 0, 0, packet);
        bytes_sent += packet->length;
    }
    result = CSPFTP_ERR;
    return result;
}

extern cspftp_result start_receiving_data(cspftp_t *session)
{
    cspftp_result result = CSPFTP_OK;
    csp_packet_t *packet;
    csp_socket_t socket = { .opts = CSP_SO_CONN_LESS };
    uint32_t current_seq = 0;
    dbg_log("Start receiving data");
    if(CSP_ERR_NONE == csp_bind(&socket, 8)) {
        csp_listen(&socket, 1);
        while (session->bytes_received < 1024 * PKT_SIZE)
        {
            packet = csp_recvfrom(&socket, 2000000);
            if(NULL == packet) {
                // Count maybe ?
                continue;
            }
            session->bytes_received += packet->length;
            if (current_seq != (packet->data32[0] / PKT_SIZE)) {
                dbg_warn("Break here, current_seq=%d, received_seq=%d", current_seq, (packet->data32[0] / PKT_SIZE));
            } else {
                dbg_log("Received %d bytes, seq = %lu", session->bytes_received, (packet->data32[0]) / PKT_SIZE);
            }
            current_seq++;
            csp_buffer_free(packet);
            if ((packet->data32[0]) / PKT_SIZE == 1023) {
                break;
            }
        }
    } else {
        result = CSPFTP_ERR;
    }
    return result;
}
