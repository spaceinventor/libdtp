#include "cspftp_log.h"
#include "cspftp/cspftp.h"
#include "cspftp_internal_api.h"
#include <csp/csp.h>
#include <vmem/vmem_ram.h>

#include "cspftp/cspftp.h"
#include "cspftp_log.h"

int dtp_client_main(uint32_t server) {
    cspftp_t *session = NULL;
    cspftp_result res = CSPFTP_OK;

    session = cspftp_acquire_session();
    if (!session) {
        dbg_warn("%s", cspftp_strerror(cspftp_errno(session)));
        cspftp_set_errno(NULL, CSPFTP_ENOMORE_SESSIONS);
        res = CSPFTP_ERR;
        goto get_out_please;
    } else {
        dbg_log("Session created: %p", session);
    }

    cspftp_params remote_cfg = { .remote_cfg.node = server };
    res = cspftp_set_opt(session, CSPFTP_REMOTE_CFG, &remote_cfg);
    if (CSPFTP_OK != res) {
        goto get_out_please;
    }

    cspftp_params local_cfg = { .local_cfg.vmem = 0 };
    res = cspftp_set_opt(session, CSPFTP_LOCAL_CFG, &local_cfg);
    if (CSPFTP_OK != res) {
        goto get_out_please;

    }
    res = cspftp_start_transfer(session);
    cspftp_serialize_session_to_file(session, "dtp_session.json");
    if (CSPFTP_OK != res) {
        goto get_out_please;
    }
get_out_please:
    cspftp_release_session(session);
    dbg_log("Bye...");
    return res;
}
