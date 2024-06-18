#include <assert.h>
#include <stdbool.h>
#include <csp/csp.h>

#include "dtp/dtp_protocol.h"
#include "dtp/dtp_session.h"
#include "dtp/dtp_log.h"
#include "dtp/dtp.h"

/**
 * @brief Type for static sessions
 */
typedef struct
{
    dtp_t session;
    bool in_use;
} dtp_static_session_t;

/**
 * @brief global DTP error
 */
static dtp_errno_t last_error = DTP_NO_ERR;

/**
 * @brief error code -> description table
 */
static const char *const _error_strs[] = {
    "No error",
    "Invalid argument",
    "No more static sessions available",
    "Not implemented", /* DTP_NOT_IMPLEMENTED */
    "Could not connect", /* DTP_CONNECTION_FAILED */
    "Could not allocate a mandatory piece of memory", /* DTP_MALLOC_FAILED */
    "Given session not found"
};

static_assert(DTP_LAST_ERR <= sizeof(_error_strs) / sizeof(_error_strs[0]), "_error_strs does not have entries for all error codes");

#define DTP_NOF_STATIC_SESSIONS 5
static const uint8_t dtp_nof_static_sessions = DTP_NOF_STATIC_SESSIONS;
static dtp_static_session_t static_sessions[DTP_NOF_STATIC_SESSIONS] = {0};


/**
 * Default session hooks
 */

static void default_on_session_start(dtp_t *session);
static bool default_on_data_packet(dtp_t *session, csp_packet_t *p);
static void default_on_session_end(dtp_t *session);
static void default_on_serialize(dtp_t *session, vmem_t *output);
static void default_on_deserialize(dtp_t *session, vmem_t *input);
static void default_on_session_release(dtp_t *session);

dtp_opt_session_hooks_cfg default_session_hooks __attribute__((weak)) = {
    .on_start = default_on_session_start,
    .on_data_packet = default_on_data_packet,
    .on_end = default_on_session_end,
    .on_serialize = default_on_serialize,
    .on_deserialize = default_on_deserialize,
    .on_release = default_on_session_release,
    .hook_ctx = 0
};


const uint32_t DTP_SESSION_VERSION = 1;

static void default_on_session_start(dtp_t *session) {
    dbg_log("Default on_start hook");
    (void)session;
}
static bool default_on_data_packet(dtp_t *session, csp_packet_t *packet) {
    dbg_log("Default on_data_packet hook");
    (void)session;
    (void)packet;
    return true;
}
static void default_on_session_end(dtp_t *session) {
    dbg_log("Default on_end hook");
    (void)session;
}

static void default_on_serialize(dtp_t *session, vmem_t *output) {
    dbg_log("Default on_serialize hook");
    (void)session;
    (void)output;
}

static void default_on_deserialize(dtp_t *session, vmem_t *input) {
    dbg_log("Default on_deserialize hook");
    (void)session;
    (void)input;
}
static void default_on_session_release(dtp_t *session) {
    dbg_log("Default on_release hook");
    (void)session;
}

dtp_errno_t dtp_errno(dtp_t *session)
{
    if (0 != session)
    {
        return session->errno;
    }
    return last_error;
}

const char *dtp_strerror(dtp_errno_t err)
{
    return _error_strs[err];
}

dtp_t *dtp_acquire_session()
{
    for (uint8_t i = 0; i < dtp_nof_static_sessions; i++)
    {
        if (false == static_sessions[i].in_use)
        {
            static_sessions[i].in_use = true;
            dtp_t *session = &static_sessions[i].session;
            session->bytes_received = 0;
            session->request_meta.nof_intervals = 1;
            session->request_meta.intervals[0].start = 0;
            session->request_meta.intervals[0].end = 0xffffffff; /* interval 0-0xFFFFFFFF means the whole thing */
            dtp_params default_params = {
                .hooks = default_session_hooks
            };
            dtp_set_opt(session, DTP_SESSION_HOOKS_CFG, &default_params);
            last_error = DTP_NO_ERR;
            return session;
        }
    }
    last_error = DTP_ENOMORE_SESSIONS;
    return 0;
}

dtp_result dtp_release_session(dtp_t *session)
{
    for (uint8_t i = 0; i < dtp_nof_static_sessions; i++)
    {
        if (session == &(static_sessions[i].session))
        {
            if(session->hooks.on_release) {
                session->hooks.on_release(session);
            }
            csp_close(static_sessions[i].session.conn);
            /* Save session last error in case somebody asks after releasing */
            last_error = static_sessions[i].session.errno;
            static_sessions[i].in_use = false;
            static_sessions[i].session.errno = DTP_NO_ERR;
            return DTP_OK;
        }
    }
    last_error = DTP_ESESSION_NOT_FOUND;
    return DTP_ERR;
}

dtp_result dtp_set_opt(dtp_t *session, dtp_option option, dtp_params *param)
{
    switch (option)
    {
    case DTP_REMOTE_CFG:
    {
        session->remote_cfg.node = param->remote_cfg.node;
        break;
    }
    case DTP_SESSION_HOOKS_CFG:
    {
        session->hooks.on_start = param->hooks.on_start;
        session->hooks.on_data_packet = param->hooks.on_data_packet;
        session->hooks.on_end = param->hooks.on_end;
        session->hooks.on_release = param->hooks.on_release;
        session->hooks.on_serialize = param->hooks.on_serialize;
        session->hooks.on_deserialize = param->hooks.on_deserialize;
        break;
    }
    case DTP_THROUGHPUT_CFG:
        session->request_meta.throughput = param->throughput.value;
    break;
    case DTP_TIMEOUT_CFG:
        session->timeout = param->timeout.value;
    break;
    case DTP_PAYLOAD_ID_CFG:
        session->request_meta.payload_id = param->payload_id.value;
    break;
    default:
        break;
    }
    return DTP_OK;
}

dtp_result dtp_get_opt(dtp_t *session, dtp_option option, dtp_params *param)
{
    switch (option)
    {
    case DTP_REMOTE_CFG:
    {
        param->remote_cfg.node = session->remote_cfg.node;
        break;
    }
    case DTP_SESSION_HOOKS_CFG:
    {
        param->hooks.on_start = session->hooks.on_start;
        param->hooks.on_data_packet = session->hooks.on_data_packet;
        param->hooks.on_end = session->hooks.on_end;
        param->hooks.on_release = session->hooks.on_release;
        param->hooks.on_serialize = session->hooks.on_serialize;
        param->hooks.on_deserialize = session->hooks.on_deserialize;
        break;
    }
    default:
        break;
    }
    return DTP_OK;
}

dtp_result dtp_stop_transfer(dtp_t *session)
{
    return DTP_OK;
}

dtp_result dtp_serialize_session(dtp_t *session, vmem_t *dest)
{
    if(session->hooks.on_serialize) {
        session->hooks.on_serialize(session, dest);
    }
    return DTP_OK;
}

dtp_result dtp_deserialize_session(dtp_t *session, vmem_t *src)
{
    if(session->hooks.on_deserialize) {
        session->hooks.on_deserialize(session, src);
    }
    return DTP_OK;
}

/* Internal functions */
void dtp_set_errno(dtp_t *session, dtp_errno_t errno)
{
    if (0 != session)
    {
        session->errno = errno;
    } else {
        last_error = errno;
    }
}

uint32_t compute_throughput(uint32_t now, uint32_t last_ts, uint32_t bytes_sent) {
    if((now != last_ts)) {
        return bytes_sent / (now - last_ts);
    }
    return 0;
}
