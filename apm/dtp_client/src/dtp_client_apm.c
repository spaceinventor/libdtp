#include <stdio.h>
#include <apm/apm.h>
#include <vmem/vmem_mmap.h>
#include <slash/slash.h>
#include <slash/optparse.h>
#include "dtp/dtp.h"
#include "segments_utils.h"
#include "dtp_log.h"
#include "dtp_session.h"

extern  vmem_t VMEM_MMAP_VAR(dtp_session);

int apm_init(void)
{
    return 0;
}

extern dtp_opt_session_hooks_cfg apm_session_hooks;

int dtp_client(struct slash *s)
{
    typedef struct {
        int color;
        int resume;
        uint32_t server;
        unsigned int throughput;
        unsigned int timeout;
    } dtp_client_opts_t;
    dtp_client_opts_t opts = { .color = 0, .server = 0, .throughput = 1024, .timeout = 5 };
    optparse_t * parser = optparse_new("dtp_client", "<name>");
    /* This is very important, else the default no-op hooks will be used */
    default_session_hooks = apm_session_hooks;

    optparse_add_help(parser);
    optparse_add_set(parser, 'c', "color", 1, &opts.color, "enable color output");
    optparse_add_set(parser, 'r', "resume", 1, &opts.resume, "resume previous session");
	optparse_add_unsigned(parser, 's', "server", "CSP address", 10, &opts.server, "CSP Address of the DT server to retrieve data from (default = 0))");
    optparse_add_unsigned(parser, 't', "throughput", "Transfer throughput", 10, &opts.throughput, "Max throughput expressed in Kb/s (default = 1024Kb/sec))");
    optparse_add_unsigned(parser, 'T', "timeout", "Timeout", 10, &opts.timeout, "Idle timeout (default = 5)");

    int argi = optparse_parse(parser, s->argc - 1, ((const char **)s->argv) + 1);
    if (argi < 0) {
        optparse_del(parser);
        return SLASH_EINVAL;
    }

    optparse_del(parser);
    dtp_t *session;
    dtp_result result = dtp_client_main(opts.server, opts.throughput, opts.timeout, opts.resume, &session);    

    if (DTP_ERR == result) {
        switch(dtp_errno(NULL)) {
            case DTP_EINVAL:
                return SLASH_EINVAL;
            default:
                printf("%s\n", dtp_strerror(dtp_errno(NULL)));
                return SLASH_SUCCESS;
        }
    } else {
        dtp_serialize_session(session, &VMEM_MMAP_VAR(dtp_session));
        dtp_release_session(session);
    }
    return SLASH_SUCCESS;
}


int dtp_info(struct slash *s)
{
    optparse_t * parser = optparse_new("dtp_info", "<name>");
    optparse_add_help(parser);
    optparse_parse(parser, s->argc - 1, ((const char **)s->argv) + 1);
    optparse_del(parser);
    dtp_t session = { 0 };
    apm_session_hooks.on_deserialize(&session, NULL);
        printf("  remote address: %u\n", session.remote_cfg.node);
        printf("  timeout: %u\n", session.request_meta.timeout);
        printf("  throughput: %u Kb/s\n", session.request_meta.throughput);
        printf("  payload id: %u\n", session.request_meta.payload_id);
        printf("  bytes_received: %u\n", session.bytes_received);
        printf("  payload size: %u\n", session.payload_size);
        printf("  Missing: %u\n", session.payload_size - session.bytes_received);
        printf("  Missing intervals: %u\n", session.request_meta.nof_intervals);
        for(uint8_t i = 0; i < session.request_meta.nof_intervals; i++) {
            printf("\t\tinterval #%u: start=%u, end=%u\n", i, session.request_meta.intervals[i].start, session.request_meta.intervals[i].end);
        }
    return SLASH_SUCCESS;
}

slash_command(dtp_client, dtp_client, "", "DTP client");
slash_command(dtp_info, dtp_info, "", "Show information about a saved DTP session");