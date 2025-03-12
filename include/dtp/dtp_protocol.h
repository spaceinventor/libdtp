#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * DTP, Internal header for protocol related types and functions
     */

#include <stdint.h>
#include "dtp/dtp.h"
#include "dtp/platform.h"

    /* CSP connection  type forward declaration*/
    typedef struct csp_packet_s csp_packet_t;

    /** Transfer inverval definition */
    typedef struct {
        uint32_t start;
        uint32_t end;
    } interval_t;

    /** Transfer request */
    typedef struct
    {
        uint32_t throughput; /** max server throughput in KB/second */
        // uint8_t timeout; /** number of seconds with no received packets that will stop the session */
        uint8_t nof_intervals; /** Number of segments to transfer, see the intervals below */
        uint8_t payload_id; /** Payload ID, conceptual identifier for the payload to retrieve, semantic is entirely server-specific */
        uint16_t mtu; /** MTU size (size of the *useful* payload DTP will use to split the payload) in BYTES */
        interval_t intervals[19]; /** list of start-stop pairs to transfer, number is set by nof_intervals above */
    } dtp_meta_req_t;

    typedef struct {
        uint16_t destination; /** Destination port */
        uint32_t size_in_bytes; /** total number of bytes to transfer */
        bool *keep_running; /** Flag indicating if the transfer should continue or not */
        dtp_meta_req_t request; /** List of intervals to transfer */
        dtp_payload_meta_t payload_meta; /** Payload info, see dtp_payload_meta_t */
    } dtp_server_transfer_ctx_t;

    /**
     * @brief CSP response sent from the server
     */
    typedef struct
    {
        uint32_t size_in_bytes; /** Total size of payload data to be sent during this transfer */
        uint32_t total_payload_size; /** Total size of payload, for info (will be >= size_in_bytes) */
    } dtp_meta_resp_t;

    extern dtp_server_transfer_ctx_t server_transfer_ctx;

    extern csp_packet_t *setup_server_transfer(dtp_server_transfer_ctx_t *ctx, uint16_t dst, csp_packet_t *request);
    extern dtp_result start_sending_data(dtp_server_transfer_ctx_t *ctx);

    extern dtp_result send_remote_meta_req(dtp_t *session);
    extern dtp_result read_remote_meta_resp(dtp_t *session);
    extern dtp_result start_receiving_data(dtp_t *session);

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
