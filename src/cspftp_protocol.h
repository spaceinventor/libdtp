#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * CSPFTP, Internal header for protocol related types and functions
     */

#include <stdint.h>
#include <csp/csp.h>
#include "cspftp/cspftp.h"

    /** Transfer inverval definition */
    typedef struct {
        uint32_t start;
        uint32_t end;
    } interval_t;

    /** Transfer request */
    typedef struct
    {
        uint8_t nof_intervals; /** Number of segments to transfer, see the intervals below */
        uint8_t payload_id; /** Payload ID, conceptual identifer for the payload to retrieve, semantic is entirely server-specific */
        interval_t intervals[19]; /** list of start-stop pairs to transfer, number is set by nof_intervals above */
    } cspftp_meta_req_t;

    /**
     * @brief Payload to transfer meta data (size, buffered access interfaces, etc)
     */
    typedef struct {
        uint32_t size;
    } dftp_payload_meta_t;

    typedef struct {
        uint16_t destination; /** Destination port */
        uint32_t size_in_bytes; /** total number of bytes to transfer */
        cspftp_meta_req_t request; /** List of intervals to transfer */
        dftp_payload_meta_t payload_meta; /** Payload info, see dftp_payload_meta_t */
    } cspftp_server_transfer_ctx_t;

    /**
     * @brief CSP response sent from the server
     */
    typedef struct
    {
        uint32_t size_in_bytes; /** Total size of payload data to be sent during this transfer */
        uint32_t total_payload_size; /** Total size of payload, for info (will be >= size_in_bytes) */
    } cspftp_meta_resp_t;

    extern cspftp_server_transfer_ctx_t server_transfer_ctx;

    extern csp_packet_t *setup_server_transfer(cspftp_server_transfer_ctx_t *ctx, uint16_t dst, csp_packet_t *request);
    extern cspftp_result start_sending_data(cspftp_server_transfer_ctx_t *ctx);

    extern cspftp_result send_remote_meta_req(cspftp_t *session);
    extern cspftp_result read_remote_meta_resp(cspftp_t *session);
    extern cspftp_result start_receiving_data(cspftp_t *session);

    /**
     * @brief Compute the number of CSP packets required to send total bytes
     * @param total number of bytes to divide into CSP packets
     * @param effective_payload_size number of payload bytes in one CSP packet
     * @return integer number of CSP packets to use to send total bytes
     */
    extern uint32_t compute_nof_packets(uint32_t total, uint32_t effective_payload_size);
#ifdef __cplusplus
}
#endif
