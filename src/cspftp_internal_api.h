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

#ifdef __cplusplus
}
#endif
