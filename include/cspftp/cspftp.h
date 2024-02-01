#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /** Opaque handle to a CSPFTP sesssion */
    typedef struct cspftp_t cspftp_t;

    /** Returned by most of CSPFTP API calls*/
    typedef enum
    {
        CSPFTP_OK //<! Chill, all good
    } cspftp_result;

    /**
     * Represent the error code for the most recent CSPFTP operation when this situation
     * cannot be the return value of an API call (because async for example)
     */
    typedef enum
    {
        CSPFTP_NO_ERR, //<! Chill, all good
        CSPFTP_LAST_ERR
    } cspftp_errno_t;

    /** The current state of a CSPFTP transaction */
    typedef enum {
        NEW,
        IN_PROGRESS,
        PAUSED,
        DONE
    } cspftp_state;

    /**
     *
     */
    cspftp_result cspftp_new_session(cspftp_t **session);

    /**
     *
     */
    cspftp_result cspftp_free_session(cspftp_t *session);

    /**
     *
     */
    cspftp_result cspftp_config(cspftp_t *session, const void *const data, uint32_t len);

    /**
     *
     */
    cspftp_errno_t cspftp_errno(cspftp_t *session);

    /**
     *
     */
    const char* cspftp_strerror(cspftp_errno_t err);

#ifdef __cplusplus
}
#endif
