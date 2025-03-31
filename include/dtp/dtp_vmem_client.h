#pragma once

#include <stdint.h>

#include "dtp/dtp_protocol.h"
#include "vmem/vmem_server.h"

#define VMEM_SERVER_START_DTP_TRANSFER (VMEM_SERVER_USER_TYPES + 0)

typedef struct {
    vmem_request_hdr_t hdr;
    uint64_t vaddr;
    uint32_t size;
    dtp_meta_req_t meta;
} __attribute__((packed)) dtp_vmem_request_t;
