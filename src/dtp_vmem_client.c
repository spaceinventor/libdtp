#include <stdint.h>
#include <stdio.h>
#include <endian.h>

#include "csp/arch/csp_time.h"
#include "csp/csp.h"
#include "vmem/vmem_server.h"
#include "dtp/dtp_vmem_client.h"
#include "dtp/dtp_vmem_server.h"
#include "dtp/dtp_session.h"

#define STRINGIFY_IMPL(arg) #arg
#define STRINGIFY(arg) STRINGIFY_IMPL(arg)

#define CSI "\x1B["
#define SAVE_CURPOS CSI "s"
#define RESTORE_CURPOS CSI "u"
#define ERASE_LINE CSI "K"
#define SET_CURPOS CSI "%u;%uH"
#define SCROLL_UP CSI "S"
#define SCROLL_DOWN CSI "T"

static bool vmem_dtp_on_data_packet(dtp_t *session, csp_packet_t *packet)
{
    printf(SAVE_CURPOS);
    printf(SET_CURPOS, 0, 0);
    printf(ERASE_LINE);
    printf("Receiving: [%"PRIu32":%"PRIu32"]\n", packet->data32[0], (uint32_t)(packet->data32[0] + packet->length - sizeof(uint32_t) - 1));
    printf(RESTORE_CURPOS);
    return true;
}

int vmem_dtp_download(int node, int timeout, uint64_t address, uint32_t length, char * dataout, int version, int use_rdp, uint32_t thrughput)
{
    uint8_t intervals = 1;

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
    request->session_id = htobe32(0); /* TODO: use the session ID */
    request->vaddr = htobe64(address);
    request->size = htobe32(length);
    
    /* DTP specifics */
    request->meta.throughput = htobe32(thrughput);
    request->meta.mtu = htobe16(VMEM_SERVER_MTU); /* MTU size (size of the *useful* payload DTP will use to split the payload) in BYTES */
    request->meta.nof_intervals = intervals; /* Only one interval, since this is the initial one */
    request->meta.intervals[0].start = htobe32(0);
    request->meta.intervals[0].end = htobe32(UINT32_MAX);

    /* Set the CSP packet length */
    packet->length = packet_len;

    /* Send the request */
    csp_send(conn, packet);

    csp_close(conn);

    dtp_t *session = NULL;
    dtp_result res = DTP_OK;

    session = dtp_acquire_session();
    dtp_params remote_cfg = { .remote_cfg.node = node };
    res = dtp_set_opt(session, DTP_REMOTE_CFG, &remote_cfg);
    if (DTP_OK != res) {
        goto get_out_please;
    }

    remote_cfg.throughput.value = thrughput;
    res = dtp_set_opt(session, DTP_THROUGHPUT_CFG, &remote_cfg);
    if (DTP_OK != res) {
        goto get_out_please;
    }

    remote_cfg.timeout.value = timeout;
    res = dtp_set_opt(session, DTP_TIMEOUT_CFG, &remote_cfg);
    if (DTP_OK != res) {
        goto get_out_please;
    }
    
    remote_cfg.payload_id.value = UINT8_MAX;
    res = dtp_set_opt(session, DTP_PAYLOAD_ID_CFG, &remote_cfg);
    if (DTP_OK != res) {
        goto get_out_please;
    }
    
    remote_cfg.mtu.value = VMEM_SERVER_MTU;
    res = dtp_set_opt(session, DTP_MTU_CFG, &remote_cfg);
    if (DTP_OK != res) {
        goto get_out_please;
    }

    session->payload_size = length;
    session->hooks.on_data_packet = vmem_dtp_on_data_packet;
    res = start_receiving_data(session);
    if (res != DTP_OK) {
        printf("Error receiving data: %d\n", res);
    }

get_out_please:
    dtp_release_session(session);

    return 0;

}

int vmem_dtp_stop_download(int node, int timeout, int version, int use_rdp) {

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
    request->session_id = htobe32(0);; /* TODO: use the session ID */

    /* Set the CSP packet length */
    packet->length = packet_len;

    /* Send the request */
    csp_send(conn, packet);

    csp_close(conn);

    return 0;
}