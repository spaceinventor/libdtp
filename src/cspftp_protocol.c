#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <csp/csp.h>
#include <cspftp/platform.h>
#include "cspftp_log.h"
#include "cspftp_protocol.h"
#include "cspftp_log.h"
#include "cspftp_session.h"

#define PKT_SIZE (200U)

const uint16_t CSPFTP_PACKET_SIZE = PKT_SIZE;

cspftp_server_transfer_ctx_t server_transfer_ctx;

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

uint32_t compute_nof_packets(uint32_t total, uint32_t effective_payload_size) {
    uint32_t expected_nof_packets = total / effective_payload_size;

    /* an extra packet will be needed for the remaining bytes */
    if(total % effective_payload_size) {
        expected_nof_packets++;
    }
    return expected_nof_packets;
}
