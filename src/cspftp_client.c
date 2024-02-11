#include "cspftp_log.h"
#include "cspftp/cspftp.h"
#include <csp/csp.h>
#include <vmem/vmem_ram.h>
#include <slash/optparse.h>

static uint8_t vmem_ram[256] = {0};
VMEM_DEFINE_STATIC_RAM_ADDR(session_serialize_test, "session_serialize_test", sizeof(vmem_ram), &vmem_ram[0]);

typedef struct {
    int color;
    uint32_t server;
} dtp_client_opts_t;

int dtp_client_main(int argc, char *argv[]) {
    cspftp_t *session;
    dtp_client_opts_t opts;
    optparse_t * parser = optparse_new("dt", "<name>");
    optparse_add_help(parser);
    optparse_add_set(parser, 'c', "color", 1, &opts.color, "enable color output");
	optparse_add_unsigned(parser, 's', "server", "CSP address", 0, &opts.server, "CSP Address of the DT server to retrieve data from (default = 0))");

    int argi = optparse_parse(parser, argc - 1, (const char **) argv + 1);
    if (argi < 0) {
        optparse_del(parser);
	    return CSPFTP_EINVAL;
    }

    /* Check "argi" if positional args are needed*/

    dbg_enable_colors(opts.color);

    session = cspftp_acquire_session();
    if (!session) {
        dbg_warn("%s", cspftp_strerror(cspftp_errno(session)));
    } else {
        dbg_log("Session created: %p", session);
    }

    cspftp_params remote_cfg = { .remote_cfg.node = opts.server };
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
get_out_please:    
    dbg_log("Bye...");
    return 0;
}
