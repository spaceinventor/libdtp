#include <stdio.h>
#include <apm/apm.h>
#include <vmem/vmem_mmap.h>
#include <slash/slash.h>
#include <slash/optparse.h>
#include "cspftp/cspftp.h"
#include "segments_utils.h"
#include "cspftp_log.h"
#include "cspftp_session.h"

extern  vmem_t VMEM_MMAP_VAR(dtp_session);

int apm_init(void)
{
    return 0;
}

extern cspftp_opt_session_hooks_cfg apm_session_hooks;

int dtp_client(struct slash *s)
{
    typedef struct {
        int color;
        int resume;
        uint32_t server;
        unsigned int throughput;
        unsigned int timeout;
    } dtp_client_opts_t;
    dtp_client_opts_t opts = { .color = 0, .server = 0, .throughput = 8, .timeout = 5 };
    optparse_t * parser = optparse_new("dtp_client", "<name>");
    /* This is very important, else the default no-op hooks will be used */
    default_session_hooks = apm_session_hooks;

    optparse_add_help(parser);
    optparse_add_set(parser, 'c', "color", 1, &opts.color, "enable color output");
    optparse_add_set(parser, 'r', "resume", 1, &opts.resume, "resume previous session");
	optparse_add_unsigned(parser, 's', "server", "CSP address", 0, &opts.server, "CSP Address of the DT server to retrieve data from (default = 0))");
    optparse_add_unsigned(parser, 't', "throughput", "Transfer throughput", 8, &opts.throughput, "Max throughput expressed in MBits/sec (default = 8Mbits/sec))");
    optparse_add_unsigned(parser, 'T', "timeout", "Timeout", 5, &opts.timeout, "Idle timeout (default = 5)");

    int argi = optparse_parse(parser, s->argc - 1, ((const char **)s->argv) + 1);
    if (argi < 0) {
        optparse_del(parser);
        return SLASH_EINVAL;
    }

    optparse_del(parser);
    cspftp_t *session;
    cspftp_result result = dtp_client_main(opts.server, opts.throughput, opts.timeout, opts.resume, &session);
    cspftp_serialize_session(session, &VMEM_MMAP_VAR(dtp_session));

    if (CSPFTP_ERR == result) {
        switch(cspftp_errno(NULL)) {
            case CSPFTP_EINVAL:
                return SLASH_EINVAL;
            default:
                printf("%s\n", cspftp_strerror(cspftp_errno(NULL)));
                return SLASH_SUCCESS;
        }
    } else {
        cspftp_release_session(session);
    }
    return SLASH_SUCCESS;
}

slash_command(dtp_client, dtp_client, "-s", "DTP client");