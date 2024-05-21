/**
 * @file platform.h
 * DTP, Header for payload handling. Defines function to be implemented by server process per platform.
 * 
 */
#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "dtp/dtp.h"

    /**
     * @brief Payload to transfer meta data (size, buffered access interfaces, etc)
     */
    typedef struct
    {
        uint32_t size;
        uint32_t (*read)(uint8_t payload_id, uint32_t offset, void *output, uint32_t size);
    } dftp_payload_meta_t;

    /**
     * @brief Get info about a specific payload
     * 
     * This function is to be specialized for each platform that can deliver a payload through DTP,
     * if you do not implement it, a test implementation (__weak__) will get linked in.
     * 
     * @param meta[out] pointer to a valid object that will be filled in
     * @param payload_id payload identifier
     * @return true if payload is valid and meta data filled in, false otherwise
     */
    extern bool get_payload_meta(dftp_payload_meta_t *meta, uint8_t payload_id);

#ifdef __cplusplus
}
#endif
