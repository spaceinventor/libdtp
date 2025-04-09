#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "csp/csp.h"
#include "dtp/dtp_protocol.h"

typedef bool vmem_dtp_on_data_t(dtp_t *session, csp_packet_t *packet);

extern int vmem_dtp_download(int node, int timeout, uint64_t address, uint32_t length, vmem_dtp_on_data_t *on_data, int version, int use_rdp, uint32_t thrughput);
extern int vmem_dtp_stop_download(int node, int timeout, int version, int use_rdp);
