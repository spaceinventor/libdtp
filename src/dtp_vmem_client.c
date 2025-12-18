#include <stdint.h>
#include <stdio.h>
#include <endian.h>

#include "csp/arch/csp_time.h"
#include "csp/csp.h"
#include "vmem/vmem_server.h"
#include "dtp/dtp_vmem_client.h"
#include "dtp/dtp_vmem_server.h"
#include "dtp/dtp_session.h"
#include "dtp_internal_api.h"

int vmem_request_dtp_start_download(dtp_t *session, int node, uint32_t session_id, int timeout, int version, int use_rdp, uint64_t vaddr, uint32_t size)
{
    /* Establish RDP connection */
    uint32_t opts = CSP_O_CRC32;
    if (use_rdp) {
        opts |= CSP_O_RDP;
    }
    csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, node, VMEM_PORT_SERVER, timeout, opts);
    if (conn == NULL) {
        return -1;
    }

    uint16_t packet_len = sizeof(dtp_vmem_request_t) + sizeof(dtp_start_req_t);
    csp_packet_t * packet = csp_buffer_get(packet_len);
    if (packet == NULL) {
        return -1;
    }

    /* VMEM request header */
    dtp_vmem_request_t *vmem_request = (dtp_vmem_request_t *)&packet->data[0];
    vmem_request->hdr.type = VMEM_SERVER_DTP_REQUEST;
    vmem_request->hdr.version = version;

    /* DTP request */
    vmem_request->type = DTP_REQUEST_START_TRANSFER;
    dtp_start_req_t *request = (dtp_start_req_t *)&vmem_request->body[0];
    request->vaddr = htobe64(vaddr);
    request->size = htobe32(size);

    /* DTP specifics */
    request->meta.throughput = htobe32(session->request_meta.throughput); /* Throughput in bytes/second */
    request->meta.mtu = htobe16(session->request_meta.mtu); /* MTU size (size of the *useful* payload DTP will use to split the payload) in BYTES */
    request->meta.session_id = htobe32(session_id); /* The session ID, representing this particular transfer */
    request->meta.keep_alive_interval = htobe32(session->request_meta.keep_alive_interval); /* Keep alive interval, 0 means no keep alive */
    request->meta.nof_intervals = session->request_meta.nof_intervals; /* Number of intervals */
    if (request->meta.nof_intervals > (sizeof(request->meta.intervals) / sizeof(request->meta.intervals[0]))) {
        request->meta.nof_intervals = (sizeof(request->meta.intervals) / sizeof(request->meta.intervals[0]));
    }
    for (uint8_t i = 0; i < request->meta.nof_intervals; i++) {
        request->meta.intervals[i].start = htobe32(session->request_meta.intervals[i].start);
        request->meta.intervals[i].end = htobe32(session->request_meta.intervals[i].end);
    }
    /* Set the CSP packet length */
    packet->length = packet_len;

    /* Send the request */
    csp_send(conn, packet);

    /* Close connection */
    csp_close(conn);

    return 0;

}

int vmem_request_dtp_stop_download(int node, uint32_t session_id, int timeout, int version, int use_rdp) {

    /* Establish RDP connection */
    uint32_t opts = CSP_O_CRC32;
    if (use_rdp) {
        opts |= CSP_O_RDP;
    }
    csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, node, VMEM_PORT_SERVER, timeout, opts);
    if (conn == NULL) {
        return -1;
    }

    uint16_t packet_len = sizeof(dtp_vmem_request_t) + sizeof(dtp_stop_req_t);
    csp_packet_t * packet = csp_buffer_get(packet_len);
    if (packet == NULL) {
        return -1;
    }

    /* VMEM request header */
    dtp_vmem_request_t *vmem_request = (dtp_vmem_request_t *)&packet->data[0];
    vmem_request->hdr.type = VMEM_SERVER_DTP_REQUEST;
    vmem_request->hdr.version = version;

    /* DTP request */
    vmem_request->type = DTP_REQUEST_STOP_TRANSFER;
    dtp_stop_req_t *request = (dtp_stop_req_t *)&vmem_request->body[0];
    request->session_id = htobe32(session_id);

    /* Set the CSP packet length */
    packet->length = packet_len;

    /* Send the request */
    csp_send(conn, packet);

    /* Close connection */
    csp_close(conn);

    return 0;
}

int vmem_request_dtp_status(int node, int timeout, int version, int use_rdp) {

    int result = -1;

    /* Establish RDP connection */
    uint32_t opts = CSP_O_CRC32;
    if (use_rdp) {
        opts |= CSP_O_RDP;
    }
    csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, node, VMEM_PORT_SERVER, timeout, opts);
    if (conn == NULL) {
        return -1;
    }

    uint16_t packet_len = sizeof(dtp_vmem_request_t);
    csp_packet_t * packet = csp_buffer_get(packet_len);
    if (packet == NULL) {
        return -1;
    }

    /* VMEM request header */
    dtp_vmem_request_t *vmem_request = (dtp_vmem_request_t *)&packet->data[0];
    vmem_request->hdr.type = VMEM_SERVER_DTP_REQUEST;
    vmem_request->hdr.version = version;

    /* DTP request */
    vmem_request->type = DTP_REQUEST_TRANSFER_STATUS;

    /* Set the CSP packet length */
    packet->length = packet_len;

    /* Send the request */
    csp_send(conn, packet);

    /* Wait for the response */
    csp_packet_t *reponse = csp_read(conn, timeout);
    if (reponse) {
        /* Check the response */
        if (reponse->length >= sizeof(dtp_status_resp_t)) {
            dtp_status_resp_t *response = (dtp_status_resp_t *)&reponse->data[0];
            uint32_t session_id = be32toh(response->session_id);
            uint32_t status = be32toh(response->status);
            printf("Received DTP VMEM status response.\n");
            printf("\tSession ID: %"PRIu32"\n", session_id);
            printf("\tStatus: %"PRIu32"\n", status);
            result = (int)status;
        } else {
            printf("Invalid response length\n");
        }
        csp_buffer_free(reponse);
    }

    /* Close connection */
    csp_close(conn);

    return result;
}

int vmem_request_dtp_send_alive(int node) {

    int result = -1;

    /* Establish RDP connection */
    uint32_t opts = CSP_O_CRC32;
    opts |= CSP_O_RDP;
    csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, node, VMEM_PORT_SERVER, 100000, opts);
    if (conn == NULL) {
        return -1;
    }

    uint16_t packet_len = sizeof(dtp_vmem_request_t);
    csp_packet_t * packet = csp_buffer_get(packet_len);
    if (packet == NULL) {
        return -1;
    }

    /* VMEM request header */
    dtp_vmem_request_t *vmem_request = (dtp_vmem_request_t *)&packet->data[0];
    vmem_request->hdr.type = VMEM_SERVER_DTP_REQUEST;
    vmem_request->hdr.version = 0;

    /* DTP request */
    vmem_request->type = DTP_REQUEST_TRANSFER_ALIVE;

    /* Set the CSP packet length */
    packet->length = packet_len;

    /* Send the request */
    csp_send(conn, packet);

    /* Close connection */
    csp_close(conn);

    return result;
}