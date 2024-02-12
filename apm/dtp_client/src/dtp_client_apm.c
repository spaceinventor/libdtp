#include <apm/apm.h>
#include <slash/slash.h>


int apm_init(void) {
    return 0;
}

int dtp_client(struct slash *s) {
    extern int dtp_client_main(int argc, char *argv[]);
    return dtp_client_main(s->argc, s->argv);
}

slash_command(dtp_client, dtp_client, "asdawd", "DTP client");