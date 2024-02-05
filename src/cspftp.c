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

cspftp_t *cspftp_acquire_session()
{
    return 0;
}

cspftp_result cspftp_release_session(cspftp_t *session)
{
    return CSPFTP_OK;
}

cspftp_result cspftp_new_session(cspftp_t **cspftp_t)
{
    return CSPFTP_OK;
}

cspftp_result cspftp_free_session(cspftp_t *session)
{
    return CSPFTP_OK;
}

cspftp_result cspftp_set_opt(cspftp_t *session, cspftp_option option, cspftp_params *param)
{
    return CSPFTP_OK;
}

cspftp_result cspftp_get_opt(cspftp_t *session, cspftp_option option, cspftp_params *param)
{
    return CSPFTP_OK;
}

cspftp_result cspftp_start_transfer(cspftp_t *session)
{
    return CSPFTP_OK;
}

cspftp_result cspftp_stop_transfer(cspftp_t *session)
{
    return CSPFTP_OK;
}

cspftp_result cspftp_serialize_session(cspftp_t *session, uint32_t (*write)(void *data, uint32_t length))
{
    return CSPFTP_OK;
}

cspftp_result cspftp_deserialize_session(cspftp_t *session, uint32_t (*read)(void *data, uint32_t length)) {
    return CSPFTP_OK;
}