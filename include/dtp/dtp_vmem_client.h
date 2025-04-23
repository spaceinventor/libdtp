#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "csp/csp.h"
#include "dtp/dtp_protocol.h"

typedef bool vmem_dtp_on_data_t(dtp_t *session, csp_packet_t *packet);

extern int vmem_request_dtp_start_download(dtp_t *session, int node, uint32_t session_id, int timeout, int version, int use_rdp, uint64_t vaddr, uint32_t size);
extern int vmem_request_dtp_stop_download(int node, uint32_t session_id, int timeout, int version, int use_rdp);
