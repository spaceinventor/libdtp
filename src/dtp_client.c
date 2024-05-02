#include <stdlib.h>
#include "dtp/dtp_log.h"
#include "dtp/dtp.h"
#include "dtp_internal_api.h"
#include <csp/csp.h>
#include <csp/arch/csp_time.h>

#include "dtp/dtp.h"
#include "dtp/dtp_session.h"
#include "dtp/dtp_log.h"

int dtp_client_main(uint32_t server, uint16_t max_throughput, uint8_t timeout, bool resume, dtp_t **out_session) {
    dtp_t *session = NULL;
    dtp_result res = DTP_OK;

    session = dtp_acquire_session();
    if (!session) {
        dbg_warn("%s", dtp_strerror(dtp_errno(session)));
        dtp_set_errno(NULL, DTP_ENOMORE_SESSIONS);
        res = DTP_ERR;
        goto get_out_please;
    } else {
        dbg_log("Session created: %p", session);
    }

    if (resume) {
        res = dtp_deserialize_session(session, 0);
        if (DTP_OK != res) {
            goto get_out_please;
        }
        if (session->request_meta.nof_intervals == 0) {
            dbg_log("No more data to fetch.");
            goto get_out_please;
        }
    }
    dtp_params remote_cfg = { .remote_cfg.node = server };
    res = dtp_set_opt(session, DTP_REMOTE_CFG, &remote_cfg);
    if (DTP_OK != res) {
        goto get_out_please;
    }

    remote_cfg.throughput.value = max_throughput;
    res = dtp_set_opt(session, DTP_THROUGHPUT_CFG, &remote_cfg);
    if (DTP_OK != res) {
        goto get_out_please;
    }

    remote_cfg.timeout.value = timeout;
    res = dtp_set_opt(session, DTP_TIMEOUT_CFG, &remote_cfg);
    if (DTP_OK != res) {
        goto get_out_please;
    }

    res = dtp_start_transfer(session);
get_out_please:
    if (DTP_OK == res && 0 != out_session) {
        /* Give session responsibility to caller, they wants it */
        *out_session = session;
    } else {
        dtp_release_session(session);
    }
    dbg_log("Bye...");
    return res;
}

dtp_result dtp_start_transfer(dtp_t *session)
{
    dtp_result res = DTP_ERR;
    csp_conn_t *conn = csp_connect(CSP_PRIO_HIGH, session->remote_cfg.node, 7, 5, CSP_O_RDP);
    if (NULL == conn) {
        session->errno = DTP_CONNECTION_FAILED;
        goto get_out;
    }
    session->conn = conn;
    res = send_remote_meta_req(session);
    if (DTP_OK != res)
    {
        goto get_out;
    }
    res = read_remote_meta_resp(session);
    if (DTP_OK != res)
    {
        goto get_out;
    }
    res = start_receiving_data(session);
get_out:
    return res;
}

dtp_result start_receiving_data(dtp_t *session)
{
    dtp_result result = DTP_OK;
    csp_packet_t *packet;
    uint32_t current_seq = 0;
    uint32_t packet_seq = 0;
    uint32_t idle_ms = 0;
    csp_socket_t sock = { .opts = CSP_SO_CONN_LESS };
    csp_socket_t *socket = &sock;
    session->start_ts = csp_get_s();
    uint32_t now = session->start_ts;
    socket->opts = CSP_SO_CONN_LESS;

    if(session->hooks.on_start) {
        session->hooks.on_start(session);
    }
    dbg_log("Start receiving data");
    csp_listen(socket, 1);
    if(CSP_ERR_NONE == csp_bind(socket, 8)) {
        const uint32_t payload_s = DTP_PACKET_SIZE - sizeof(uint32_t);
        uint32_t expected_nof_packets = compute_nof_packets(session->payload_size - session->bytes_received, payload_s);
        uint32_t nof_csp_packets = 0;
        uint32_t received_so_far = 0;
        while ((idle_ms <= (session->timeout * 1000)) && nof_csp_packets < expected_nof_packets)
        {
            packet = csp_recvfrom(socket, 1000);
            if(NULL == packet) {
                idle_ms += 1000;
                dbg_warn("No data received for %u ms, last packet_seq=%u", idle_ms, packet_seq);
                continue;
            }
            nof_csp_packets++;
            now = csp_get_s();
            idle_ms = 0;
            session->bytes_received += packet->length - sizeof(uint32_t);
            received_so_far += packet->length - sizeof(uint32_t);
            packet_seq = packet->data32[0] / (DTP_PACKET_SIZE - sizeof(uint32_t));
            if(session->hooks.on_data_packet) {
                if (false == session->hooks.on_data_packet(session, packet)) {
                   dbg_warn("on_data_packet() hook return false, (TBD: aborting)"); 
                }
            }
            current_seq++;
            csp_buffer_free(packet);
        }
        if (idle_ms >= (session->timeout * 1000)) {
            dbg_warn("No data received for %u ms, bailing out", idle_ms);
        }
        csp_socket_close(socket);
        if(session->hooks.on_end) {
            session->hooks.on_end(session);
        }
    } else {
        result = DTP_ERR;
    }
    uint32_t duration = now - session->start_ts;
    duration = duration?duration:1;
    dbg_log("Received %lu bytes, last seq: %lu, status: %d", session->bytes_received, packet_seq, result);
    dbg_log("Session duration: %u sec, avg throughput: %u Kb/sec", duration, (session->bytes_received / duration) / 1024);
    return result;
}
