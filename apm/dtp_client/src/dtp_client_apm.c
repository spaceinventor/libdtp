#include <stdio.h>
#include <apm/apm.h>
#include <slash/slash.h>
#include <slash/optparse.h>
#include "cspftp/cspftp.h"

int apm_init(void)
{
    return 0;
}

int dtp_client(struct slash *s)
{
    typedef struct {
        int color;
        uint32_t server;
    } dtp_client_opts_t;
    extern int dtp_client_main(uint32_t server);
    dtp_client_opts_t opts = { 0 };
    optparse_t * parser = optparse_new("dtp_client", "<name>");
    optparse_add_help(parser);
    optparse_add_set(parser, 'c', "color", 1, &opts.color, "enable color output");
	optparse_add_unsigned(parser, 's', "server", "CSP address", 0, &opts.server, "CSP Address of the DT server to retrieve data from (default = 0))");

    int argi = optparse_parse(parser, s->argc - 1, ((const char **)s->argv) + 1);
    if (argi < 0) {
        optparse_del(parser);
        return SLASH_EINVAL;
    }

    optparse_del(parser);
    cspftp_result result = dtp_client_main(opts.server);
    if (CSPFTP_ERR == result) {
        switch(cspftp_errno(NULL)) {
            case CSPFTP_EINVAL:
                return SLASH_EINVAL;
            default:
                printf("%s\n", cspftp_strerror(cspftp_errno(NULL)));
                return SLASH_SUCCESS;
        }
    }
    return SLASH_SUCCESS;
}

slash_command(dtp_client, dtp_client, "-s", "DTP client");