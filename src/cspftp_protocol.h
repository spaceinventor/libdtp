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

    typedef struct {
        uint32_t start;
        uint32_t end;
    } interval_t;

    typedef struct
    {
        uint8_t nof_intervals;
        interval_t intervals[49];
    } cspftp_meta_req_t;

    typedef struct
    {
        uint32_t size_bytes;        
    } cspftp_meta_resp_t;

    extern cspftp_result send_remote_meta_req(cspftp_t *session);
    extern cspftp_result read_remote_meta_resp(cspftp_t *session);
    extern cspftp_result start_sending_data(uint16_t dest);
    extern cspftp_result start_receiving_data(cspftp_t *session);
#ifdef __cplusplus
}
#endif
