#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * CSPFTP, Internal header for session related types and functions
     */

#include <stdint.h>
#include <csp/csp.h>
#include "cspftp/cspftp.h"
#include "cspftp_protocol.h"

    /**
     * @brief CSPFTP session, internal definition
     */
    typedef struct cspftp_t
    {
        csp_conn_t *conn;
        cspftp_opt_remote_cfg remote_cfg;
        cspftp_opt_local_cfg local_cfg;
        uint32_t bytes_received;
        cspftp_errno_t errno;
        cspftp_meta_req_t request_meta;
    } cspftp_t;

#ifdef __cplusplus
}
#endif
