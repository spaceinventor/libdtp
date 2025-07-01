#include <stdlib.h>
#include "dtp/dtp_log.h"
#include "dtp/dtp.h"
#include "dtp_internal_api.h"
#include <csp/csp.h>
#include <csp/arch/csp_time.h>

#include "dtp/dtp.h"
#include "dtp/dtp_session.h"
#include "dtp/dtp_log.h"

dtp_t *dtp_prepare_session(uint32_t server, uint32_t session_id, uint32_t max_throughput, uint8_t timeout, uint8_t payload_id, void *ctx, uint16_t mtu, bool resume) {
    dtp_t *session = NULL;
    dtp_result res = DTP_OK;

    /* Grab a new session object */
    session = dtp_acquire_session();
    if (!session) {
        dbg_warn("%s", dtp_strerror(dtp_errno(session)));
        dtp_set_errno(NULL, DTP_ENOMORE_SESSIONS);
        res = DTP_ERR;
        goto get_out_please;
    } else {
        dbg_log("Session created: %p", session);
    }

    dtp_session_set_user_ctx(session, ctx);

    if (resume) {
        /* Deserializing a session will override parameters */
        res = dtp_deserialize_session(session, ctx);
        if (DTP_OK != res) {
            dbg_warn("Deserialization failed: %s", dtp_strerror(dtp_errno(session)));
            goto get_out_please;
        }
        if (session->bytes_received == session->payload_size || session->request_meta.nof_intervals == 0) {
            dbg_log("No more data to fetch.");
            session->dtp_errno = DTP_SESSION_EXHAUSTED;
            goto get_out_please;
        }
        dtp_params remote_cfg = { .throughput.value = max_throughput };
        dtp_set_opt(session, DTP_THROUGHPUT_CFG, &remote_cfg);
    } else {
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
        
        remote_cfg.payload_id.value = payload_id;
        res = dtp_set_opt(session, DTP_PAYLOAD_ID_CFG, &remote_cfg);
        if (DTP_OK != res) {
            goto get_out_please;
        }
        
        remote_cfg.mtu.value = mtu;
        res = dtp_set_opt(session, DTP_MTU_CFG, &remote_cfg);
        if (DTP_OK != res) {
            goto get_out_please;
        }

        remote_cfg.session_id.value = session_id;
        res = dtp_set_opt(session, DTP_SESSION_ID_CFG, &remote_cfg);
        if (DTP_OK != res) {
            goto get_out_please;
        }
    }

    return session;

get_out_please:
    dtp_release_session(session);
    return NULL;
}

int dtp_vmem_client_main(dtp_t *session) {
    dtp_result res = DTP_OK;

    /* Start receiving DTP frames */
    res = start_receiving_data(session);

    return res;
}

int dtp_client_main(uint32_t server, uint32_t max_throughput, uint8_t timeout, uint8_t payload_id, uint16_t mtu, bool resume, dtp_t **out_session, void *ctx) {
    dtp_t *session = NULL;
    dtp_result res = DTP_OK;

    /* Prepare it with the parameters from the caller */
    session = dtp_prepare_session(server, 0x900dface, max_throughput, timeout, payload_id, ctx, mtu, resume);
    if (session == NULL) {
        dbg_warn("%s", dtp_strerror(dtp_errno(session)));
        res = DTP_ERR;
        goto get_out_please;
    }

    /* Initiate the transfer (the original DTP way) */
    res = dtp_start_transfer(session);

get_out_please:
    if ((DTP_OK == res || DTP_CANCELLED ) && 0 != out_session) {
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
        session->dtp_errno = DTP_CONNECTION_FAILED;
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
    uint32_t packet_seq = 0;
    uint32_t idle_ms = 0;
    csp_socket_t sock = { .opts = CSP_SO_CONN_LESS };
    csp_socket_t *socket = &sock;
    session->start_ts = csp_get_ms();
    uint32_t now_ts_ms = session->start_ts;
    socket->opts = CSP_SO_CONN_LESS;

    if(session->hooks.on_start) {
        session->hooks.on_start(session);
    }

    session->active = true;

    dbg_log("Start receiving data");
    csp_listen(socket, 1);
    if(CSP_ERR_NONE != csp_bind(socket, 8)) {
        dbg_warn("Could not bind to port 8");
        return DTP_ERR;
    }

    dtp_metrics_t metric;
    compute_dtp_metrics(&session->request_meta, session->payload_size, &metric);
    
    uint32_t nof_packets = 0;
    now_ts_ms = csp_get_ms();
    uint32_t expected_eot_ts_ms = now_ts_ms + metric.total_duration_ms + (metric.round_time_ms * 2);
    uint32_t start_ts_ms = now_ts_ms;

    /* Enter the receiver loop until we have either received all packets or the duration has expired */
    while ((now_ts_ms + idle_ms) < expected_eot_ts_ms && nof_packets < metric.nof_packets)
    {
        /* Break the receiver loop if signalled by the user setting the transfer to inactive */
        if (!dtp_is_active(session)) {
            dbg_log("DTP Operation cancelled.");
            result = DTP_CANCELLED;
            break;
        }

        /* Expect reception within at least 2 rounds of transmitting */
        packet = csp_recvfrom(socket, metric.round_time_ms * 10);
        if (NULL == packet) {
            idle_ms += (metric.round_time_ms * 10);
            dbg_warn("No data received for %" PRIu32 " [ms], last packet_seq=%" PRIu32 "", idle_ms, packet_seq);
            continue;
        }

        /* Reset the idle time and fetch current time */
        idle_ms = 0;
        now_ts_ms = csp_get_ms();

        nof_packets++;
        session->bytes_received += packet->length - (2 * sizeof(uint32_t));
        packet_seq = packet->data32[0] / (session->request_meta.mtu - (2 * sizeof(uint32_t)));

        /* Calculate the current throughput */
        uint32_t bytes_sent = nof_packets * (session->request_meta.mtu - (2 * sizeof(uint32_t)));
        uint32_t ts_diff = now_ts_ms - start_ts_ms;
        if (ts_diff > 0) {
            uint32_t curr_throughput;
            curr_throughput = bytes_sent / ts_diff;
            if (curr_throughput > 0) {
                uint32_t adj_eot_ts_ms;
                adj_eot_ts_ms = ((session->request_meta.mtu - (2 * sizeof(uint32_t))) * metric.nof_packets) / curr_throughput;
                int32_t adjust = (start_ts_ms + adj_eot_ts_ms) - expected_eot_ts_ms;
                if (adjust) {
                    expected_eot_ts_ms += adjust;
                }
            }
        }

        if(session->hooks.on_data_packet) {
            if (false == session->hooks.on_data_packet(session, packet)) {
                dbg_warn("on_data_packet() hook return false, (TBD: aborting)"); 
            }
        }
        csp_buffer_free(packet);

        /* In case we have received the last packet in a sequence, it does not make sense to sit
         * around waiting for extra ones arriving. */
        if (packet_seq >= metric.last_packet) {
            dbg_log("Received last packet (%"PRIu32") in transfer, bailing out.", metric.last_packet);
            result = DTP_OK;
            break;
        }
    }

    csp_socket_close(socket);
    if(session->hooks.on_end) {
        session->hooks.on_end(session);
    }

    uint32_t duration = now_ts_ms - session->start_ts;
    duration = duration?duration:1;
    dbg_log("Received %" PRIu32 " [bytes], last seq: %" PRIu32 ", status: %d", session->bytes_received, packet_seq, result);
    dbg_log("Session duration: %" PRIu32 ".%" PRIu32 " [s], avg throughput: %f [KB/s]", (duration/1000),(duration - ((duration/1000)*1000)), (float)((session->bytes_received / duration) * 1000 ) / 1024.0);

    return result;
}
