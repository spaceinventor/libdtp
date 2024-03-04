#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * DTP, Header for payload handling. Defines function to be implemented by server process per platform.
     * 
     */

#include <stdint.h>
#include <csp/csp.h>
#include "cspftp/cspftp.h"

    /**
     * @brief Payload to transfer meta data (size, buffered access interfaces, etc)
     */
    typedef struct
    {
        uint32_t size;
        uint32_t (*read)(uint8_t payload_id, uint32_t offset, void *output, uint32_t size);
    } dftp_payload_meta_t;

    extern bool get_payload_meta(dftp_payload_meta_t *meta, uint8_t payload_id);

#ifdef __cplusplus
}
#endif
