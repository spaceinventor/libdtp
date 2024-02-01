#include <assert.h>
#include "cspftp/cspftp.h"

typedef struct cspftp_t
{
    cspftp_errno_t errno;
} cspftp_t;


static cspftp_errno_t last_error = CSPFTP_NO_ERR;

static const char *const _error_strs[] = {
    "No error",
};

static_assert(CSPFTP_LAST_ERR <= sizeof(_error_strs) / sizeof(_error_strs[0]), "_error_strs does not have entries for all error codes");

cspftp_errno_t cspftp_errno(cspftp_t *session)
{
    if (0 != session) {
        return session->errno;
    }
    return last_error;
}

const char *cspftp_strerror(cspftp_errno_t err)
{
    return _error_strs[err];
}

cspftp_result cspftp_new(cspftp_t **cspftp_t)
{
    return 0;
}

cspftp_result cspftp_config(cspftp_t *session, const void *const data, uint32_t len)
{
    return CSPFTP_OK;
}
