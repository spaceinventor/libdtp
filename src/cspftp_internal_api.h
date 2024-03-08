#pragma once

#include "cspftp/cspftp.h"

#ifdef __cplusplus
extern "C"
{
#endif
/**
 * Internal Application Interface, for things like last_error and such
 */

extern void cspftp_set_errno(cspftp_t *session, cspftp_errno_t errno);

extern uint32_t compute_throughput(uint32_t now, uint32_t last_ts, uint32_t bytes_sent);

#ifdef __cplusplus
}
#endif
