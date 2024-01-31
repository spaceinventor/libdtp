#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct cspftp cspftp;

    typedef enum
    {
        CSPFTP_OK
    } cspftp_result;

    /**
     *
     */
    cspftp *cspftp_new(void);

    /**
     *
     */
    cspftp_result cspftp_config(cspftp *session, const void *const data, uint32_t len);

#ifdef __cplusplus
}
#endif
