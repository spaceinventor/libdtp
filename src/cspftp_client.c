#include "cspftp_log.h"
#include "cspftp/cspftp.h"
#include <csp/csp.h>
#include <vmem/vmem_ram.h>

static uint8_t vmem_ram[256] = {0};
VMEM_DEFINE_STATIC_RAM_ADDR(session_serialize_test, "session_serialize_test", sizeof(vmem_ram), &vmem_ram[0]);

int main(int argc, char *argv[]) {
    csp_conn_t *conn;
    cspftp_t *session;
    dbg_enable_colors(true);
    session = cspftp_acquire_session();
    if (!session) {
        dbg_log("%s", cspftp_strerror(cspftp_errno(session)));
    } else {
        dbg_warn("Session created: %p\n", session);
    }

    cspftp_params remote_cfg = { .remote_cfg.node = 0 };
    cspftp_result res = cspftp_set_opt(session, CSPFTP_REMOTE_CFG, &remote_cfg);
    if (CSPFTP_OK != res) {
        goto get_out_please;
    
    }

    cspftp_params local_cfg = { .local_cfg.vmem = 0 };
    res = cspftp_set_opt(session, CSPFTP_LOCAL_CFG, &local_cfg);
    if (CSPFTP_OK != res) {
        goto get_out_please;
    
    }
    res = cspftp_start_transfer(session);
    if (CSPFTP_OK != res) {
        goto get_out_please;
    
    }

    csp_init();
    conn = csp_connect(CSP_PRIO_HIGH, 0, 24, 0, 0);
    dbg_log("Connection: %p", conn);
    dbg_enable_colors(false);
get_out_please:    
    dbg_log("Bye...");
    return 0;
}
