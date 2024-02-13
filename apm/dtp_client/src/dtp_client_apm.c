#include <stdio.h>
#include <apm/apm.h>
#include <slash/slash.h>
#include "cspftp/cspftp.h"

int apm_init(void)
{
    return 0;
}

int dtp_client(struct slash *s)
{
    extern int dtp_client_main(int argc, char *argv[]);
    cspftp_result result = dtp_client_main(s->argc, s->argv);
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

slash_command(dtp_client, dtp_client, "asdawd", "DTP client");