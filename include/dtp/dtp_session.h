#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * DTP, Internal header for session related types and functions
     */

#include <stdint.h>
#include "dtp/dtp.h"
#include "dtp/dtp_protocol.h"

    /* CSP connection  type forward declaration*/
    typedef struct csp_packet_s csp_packet_t;
    /* CSP connection  type forward declaration*/
    typedef struct csp_conn_s csp_conn_t;
    /* received segment type forward declaration*/
    typedef struct segments_ctx_t segments_ctx_t;
    /**
     * @brief DTP session, internal definition
     */
    typedef struct dtp_t
    {
        bool active;
        csp_conn_t *conn;
        void *user_context;
        dtp_opt_remote_cfg remote_cfg;
        dtp_opt_session_hooks_cfg hooks;
        uint32_t bytes_received;
        uint32_t payload_size;
        uint32_t start_ts;
        uint32_t end_ts;
        dtp_errno_t dtp_errno;
        dtp_meta_req_t request_meta;
        uint8_t timeout;
    } dtp_t;

#ifdef __cplusplus
}
#endif
