#pragma once

#include <stdint.h>

#include "dtp/dtp_protocol.h"
#include "vmem/vmem_server.h"

#define VMEM_SERVER_DTP_REQUEST (VMEM_SERVER_USER_TYPES + 0)

#define DTP_REQUEST_START_TRANSFER 0x00
#define DTP_REQUEST_STOP_TRANSFER 0x01
#define DTP_REQUEST_TRANSFER_STATUS 0x02

#define DTP_TRANSFER_STATUS_OK 0UL
#define DTP_TRANSFER_STATUS_BUSY (1UL << 0)

typedef struct {
    uint32_t session_id;
    uint64_t vaddr;
    uint32_t size;
    dtp_meta_req_t meta;
} __attribute__((__packed__)) dtp_start_req_t;

typedef struct {
    uint32_t session_id;
} __attribute__((__packed__)) dtp_stop_req_t;

typedef struct {
    uint32_t session_id;
    uint32_t status;
} __attribute__((__packed__)) dtp_status_resp_t;

typedef struct {
    vmem_request_hdr_t hdr;
    uint8_t type;
    uint8_t body[0];
} __attribute__((__packed__)) dtp_vmem_request_t;

extern void dtp_vmem_server_task(void * param);
