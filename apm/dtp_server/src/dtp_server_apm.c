#include <apm/apm.h>
#include <slash/slash.h>


int apm_init(void) {
    return 0;
}

int dtp_server(struct slash *s) {
    extern int dtp_server_main(int argc, char *argv[]);
    return dtp_server_main(s->argc, s->argv);
}

slash_command(dtp_server, dtp_server, "asdawd", "DTP server");