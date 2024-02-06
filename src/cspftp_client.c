#include "cspftp_log.h"
#include "cspftp/cspftp.h"
#include <csp/csp.h>
#include <vmem/vmem_ram.h>

static uint8_t vmem_ram[256] = {0};
VMEM_DEFINE_STATIC_RAM_ADDR(session_serialize_test, "session_serialize_test", sizeof(vmem_ram), &vmem_ram[0]);

int main(int argc, char *argv[]) {
    csp_conn_t *conn;
    cspftp_t *session;
    session = cspftp_acquire_session();
    if (!session) {
        dbg_log("%s", cspftp_strerror(cspftp_errno(session)));
    } else {
        dbg_warn("Session created: %p\n", session);
    }
    csp_init();
    conn = csp_connect(CSP_PRIO_HIGH, 0, 24, 0, 0);
    dbg_log("Connection: %p", conn);
    dbg_enable_colors(false);
    dbg_log("Bye...");
    return 0;
}
