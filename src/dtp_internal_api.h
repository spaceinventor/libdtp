#pragma once

#include "dtp/dtp.h"

#ifdef __cplusplus
extern "C"
{
#endif
/**
 * Internal Application Interface, for things like last_error and such
 */

extern void dtp_set_errno(dtp_t *session, dtp_errno_t errno);

extern uint32_t compute_throughput(uint32_t now, uint32_t last_ts, uint32_t bytes_sent);

#ifdef __cplusplus
}
#endif
