#include "cspftp_log.h"
#include "cspftp/cspftp.h"
#include "cspftp_internal_api.h"
#include <csp/csp.h>
#include <csp/arch/csp_time.h>
#include <vmem/vmem_ram.h>

#include "cspftp/cspftp.h"
#include "cspftp_session.h"
#include "cspftp_log.h"

int dtp_client_main(uint32_t server, uint16_t max_throughput, uint8_t timeout, cspftp_t **out_session) {
    cspftp_t *session = NULL;
    cspftp_result res = CSPFTP_OK;

    session = cspftp_acquire_session();
    if (!session) {
        dbg_warn("%s", cspftp_strerror(cspftp_errno(session)));
        cspftp_set_errno(NULL, CSPFTP_ENOMORE_SESSIONS);
        res = CSPFTP_ERR;
        goto get_out_please;
    } else {
        dbg_log("Session created: %p", session);
    }

    cspftp_params remote_cfg = { .remote_cfg.node = server };
    res = cspftp_set_opt(session, CSPFTP_REMOTE_CFG, &remote_cfg);
    if (CSPFTP_OK != res) {
        goto get_out_please;
    }

    remote_cfg.throughput.value = max_throughput;
    res = cspftp_set_opt(session, CSPFTP_THROUGHPUT_CFG, &remote_cfg);
    if (CSPFTP_OK != res) {
        goto get_out_please;
    }

    remote_cfg.timeout.value = timeout;
    res = cspftp_set_opt(session, CSPFTP_TIMEOUT_CFG, &remote_cfg);
    if (CSPFTP_OK != res) {
        goto get_out_please;
    }

    res = cspftp_start_transfer(session);
get_out_please:
    if (CSPFTP_OK == res && 0 != out_session) {
        /* Give session responsibility to caller, they wants it */
        *out_session = session;
    } else {
        cspftp_release_session(session);
    }
    dbg_log("Bye...");
    return res;
}

cspftp_result cspftp_start_transfer(cspftp_t *session)
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

cspftp_result start_receiving_data(cspftp_t *session)
{
    cspftp_result result = CSPFTP_OK;
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
        const uint32_t payload_s = CSPFTP_PACKET_SIZE - sizeof(uint32_t);
        uint32_t expected_nof_packets = compute_nof_packets(session->total_bytes, payload_s);
        uint32_t nof_csp_packets = 0;

        while ((session->bytes_received < session->total_bytes) && (idle_ms < (session->timeout * 1000)) && packet_seq < expected_nof_packets)
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
            packet_seq = packet->data32[0] / (CSPFTP_PACKET_SIZE - sizeof(uint32_t));
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
        result = CSPFTP_ERR;
    }
    uint32_t duration = now - session->start_ts;
    duration = duration?duration:1;
    dbg_log("Received %lu bytes, expected: %lu, last seq: %lu, status: %d", session->bytes_received, session->total_bytes, packet_seq, result);
    dbg_log("Session duration: %u sec, avg throughput: %u bytes/sec", duration, session->bytes_received / duration);
    return result;
}
