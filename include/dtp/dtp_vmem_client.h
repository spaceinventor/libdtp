#pragma once

#include <stdint.h>

extern int vmem_dtp_download(int node, int timeout, uint64_t address, uint32_t length, char * dataout, int version, int use_rdp, uint32_t thrughput);
extern int vmem_dtp_stop_download(int node, int timeout, int version, int use_rdp);
