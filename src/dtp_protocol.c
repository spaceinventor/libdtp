#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <csp/csp.h>
#include <dtp/platform.h>
#include "dtp/dtp_log.h"
#include "dtp/dtp_protocol.h"
#include "dtp/dtp_session.h"

#define PKT_SIZE (200U)

const uint16_t DTP_PACKET_SIZE = PKT_SIZE;

dtp_server_transfer_ctx_t server_transfer_ctx;

dtp_result send_remote_meta_req(dtp_t *session)
{
    dtp_result res = DTP_ERR;
    csp_packet_t *packet = csp_buffer_get(0);
    if (packet)
    {
        packet->length = sizeof(dtp_meta_req_t);
        dtp_meta_req_t *req = (dtp_meta_req_t *)packet->data;
        memcpy(req, &(session->request_meta), sizeof(dtp_meta_req_t));
        csp_send(session->conn, packet);
        res = DTP_OK;
    }
    return res;
}

dtp_result read_remote_meta_resp(dtp_t *session)
{
    dtp_result res = DTP_ERR;
    csp_conn_t *conn = session->conn;
    csp_packet_t *packet = csp_read(conn, 10000);
    if (packet)
    {
        res = DTP_OK;
        dtp_meta_resp_t *meta_resp = (dtp_meta_resp_t *)packet->data;
        session->payload_size = meta_resp->total_payload_size;
        dbg_log("Setting session total bytes to %u", meta_resp->size_in_bytes);
        csp_buffer_free(packet);
    } else {
        session->errno = DTP_CONNECTION_FAILED;
    }
    return res;
}

uint32_t compute_nof_packets(uint32_t total, uint32_t effective_payload_size) {
    uint32_t expected_nof_packets = total / effective_payload_size;

    /* an extra packet will be needed for the remaining bytes */
    if(total % effective_payload_size) {
        expected_nof_packets++;
    }
    return expected_nof_packets;
}
