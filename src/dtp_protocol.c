#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <csp/csp.h>
#include <dtp/platform.h>
#include "dtp/dtp_log.h"
#include "dtp/dtp_protocol.h"
#include "dtp/dtp_session.h"

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
        session->dtp_errno = DTP_CONNECTION_FAILED;
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

void compute_transmit_metrics(dtp_meta_req_t *request, uint32_t *round_time_ms, uint32_t *packets_per_round, uint32_t *resulting_throughput) {

    uint32_t mtu = request->mtu - (2 * sizeof(uint32_t)); // MTU minus the two 32-bit words for the sequence number and session ID
    uint32_t throughput = request->throughput;
    uint32_t packets_sec = throughput / mtu;
    uint32_t secs_packet = mtu / throughput;
    dbg_log("Throughput: %"PRIu32" [bytes/s] using MTU: %"PRIu16" [bytes]", throughput, mtu);
    dbg_log("Packets/second: %" PRIu32, packets_sec);
    dbg_log("Seconds/packet: %" PRIu32, secs_packet);
    
    /* Calculate the round time and the packets per round */
    if (secs_packet > packets_sec) {
        (*round_time_ms) = secs_packet * 1000;
        (*packets_per_round) = 1;
    } else {
        (*round_time_ms) = 1000;
        (*packets_per_round) = packets_sec;
    }
    
    /* Can we optimize the round time */
    uint32_t factor = 1;
    float error;
    do {
        factor *= 10.0;
        uint32_t __throughput = (((*packets_per_round) / factor) * mtu) / ((*round_time_ms) / factor) * 1000UL;
        error = fabs(((float)throughput - (float)__throughput)/(float)throughput)*100.0;
        if ((*round_time_ms) / factor < 10) {
            dbg_log("Round time too short (<10ms)\n");
            break;
        }
    } while(error < 5.0);
    
    /* Roll back one factor of 10 */
    factor = factor / 10;
    (*round_time_ms) /= factor;
    (*packets_per_round) /= factor;
    float bytes_per_ms = ((float)(*packets_per_round) * mtu) / (float)(*round_time_ms);
    (*resulting_throughput) = (uint32_t)(bytes_per_ms * 1000.0);

    dbg_log("Round time: %" PRIu32 " [ms]", *round_time_ms);
    dbg_log("Packets per round: %" PRIu32, *packets_per_round);
    dbg_log("Resulting throughput: %f [bytes/s]", bytes_per_ms * 1000.0);
}
