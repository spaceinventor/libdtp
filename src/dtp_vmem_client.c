#include <stdint.h>
#include <stdio.h>
#include <endian.h>

#include "csp/arch/csp_time.h"
#include "csp/csp.h"
//#include "vmem/vmem_server.h"

#include "dtp/dtp_vmem_client.h"

int vmem_dtp_download(int node, int timeout, uint64_t address, uint32_t length, char * dataout, int version, int use_rdp)
{
    uint32_t time_begin = csp_get_ms();
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

    uint16_t packet_len = sizeof(dtp_vmem_request_t) + (sizeof(interval_v2_t) * intervals);
    csp_packet_t * packet = csp_buffer_get(packet_len);
    if (packet == NULL) {
        return -1;
    }

    dtp_vmem_request_t * request = (void *) packet->data;
    /* VMEM request header */
    request->hdr.type = VMEM_SERVER_START_DTP_TRANSFER;
    request->hdr.version = VMEM_VERSION;
    
    /* DTP specifics */
    request->meta.throughput = 1024; /* 1 MB/s */
    request->meta.mtu = VMEM_SERVER_MTU; /* MTU size (size of the *useful* payload DTP will use to split the payload) in BYTES */
    request->meta.nof_intervals = intervals; /* Only one interval, since this is the initial one */
    request->meta.intervals[0].start = htobe64(address);
    request->meta.intervals[0].length = htobe32(length);

    /* Set the CSP packet length */
    packet->length = packet_len;

    /* Send the request */
    csp_send(conn, packet);

    /* Now we expect the VMEM server at the other end to start DTP transmission */
    uint32_t count = 0;
    int dotcount = 0;

    while(1) { 

    }


    csp_close(conn);

    uint32_t time_total = csp_get_ms() - time_begin;

    printf("  Downloaded %u bytes in %.03f s at %u Bps\n", (unsigned int) count, time_total / 1000.0, (unsigned int) (count / ((float)time_total / 1000.0)) );

    return count;

}