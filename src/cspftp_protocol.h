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
        uint8_t nof_intervals;
        interval_t intervals[49];
    } cspftp_meta_req_t;

    typedef struct {
        uint16_t destination; /** Destination port */
        uint32_t size_in_bytes; /** total number of bytes to transfer */
        cspftp_meta_req_t request; /** List of intervals to transfer */
    } cspftp_server_transfer_ctx_t;

    typedef struct
    {
        uint32_t size_in_bytes;
    } cspftp_meta_resp_t;

    extern cspftp_server_transfer_ctx_t server_transfer_ctx;

    extern csp_packet_t *setup_server_transfer(cspftp_server_transfer_ctx_t *ctx, uint16_t dst, csp_packet_t *request);
    extern cspftp_result start_sending_data(cspftp_server_transfer_ctx_t *ctx);

    extern cspftp_result send_remote_meta_req(cspftp_t *session);
    extern cspftp_result read_remote_meta_resp(cspftp_t *session);
    extern cspftp_result start_receiving_data(cspftp_t *session);
#ifdef __cplusplus
}
#endif
