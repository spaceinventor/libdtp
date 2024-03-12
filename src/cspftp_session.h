#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * CSPFTP, Internal header for session related types and functions
     */

#include <stdint.h>
#include "cspftp/cspftp.h"
#include "cspftp_protocol.h"

    /* CSP connection  type forward declaration*/
    typedef struct csp_packet_s csp_packet_t;
    /* CSP connection  type forward declaration*/
    typedef struct csp_conn_s csp_conn_t;
    /* received segment type forward declaration*/
    typedef struct segments_ctx_t segments_ctx_t;
    /**
     * @brief CSPFTP session, internal definition
     */
    typedef struct cspftp_t
    {
        csp_conn_t *conn;
        cspftp_opt_remote_cfg remote_cfg;
        cspftp_opt_session_hooks_cfg hooks;
        uint32_t bytes_received;
        uint32_t total_bytes;
        uint32_t start_ts;
        uint32_t end_ts;
        cspftp_errno_t errno;
        cspftp_meta_req_t request_meta;
        uint8_t timeout;
    } cspftp_t;

#ifdef __cplusplus
}
#endif
