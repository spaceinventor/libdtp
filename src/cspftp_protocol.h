#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * CSPFTP, Internal header for protocol related types and functions
     */

#include <stdint.h>
#include "cspftp/cspftp.h"

    typedef struct
    {
        uint32_t ts;
    } cspftp_meta_req_t;

    typedef struct
    {
        uint32_t status;
    } cspftp_meta_resp_t;

    extern cspftp_result send_remote_meta_req(cspftp_t *session);
    extern cspftp_result read_remote_meta_resp(cspftp_t *session);
#ifdef __cplusplus
}
#endif
