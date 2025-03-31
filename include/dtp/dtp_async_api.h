#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "dtp_protocol.h"

typedef struct dtp_msg_s {
    uint32_t msg;
    uint16_t node;
    uint64_t vaddr;
    uint32_t size;
    dtp_meta_req_t meta;
} dtp_msg_t;

typedef struct dtp_async_api_s dtp_async_api_t;

typedef void dtp_async_recv_message_t(dtp_async_api_t *me, dtp_msg_t *msg);
typedef bool dtp_async_send_message_t(dtp_async_api_t *me, dtp_msg_t *msg);
typedef uint32_t dtp_async_read_data_t(dtp_async_api_t *me, uint8_t *to_addr, uint64_t from_vaddr, uint32_t size);

typedef struct dtp_async_api_s {
    dtp_async_recv_message_t *recv;
    dtp_async_send_message_t *send;
    dtp_async_read_data_t *read;
    void *context;
} dtp_async_api_t;
