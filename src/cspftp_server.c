#include "cspftp/cspftp.h"
#include <csp/csp.h>

static void cspftp_server_run()
{
    csp_socket_t sock = {0};
    csp_bind(&sock, 24);

    /* Create a backlog of 1 connection */
    csp_listen(&sock, 1);

    /* Wait for connections and then process packets on the connection */
    while (1)
    {
        /* Wait for a new connection, 10000 mS timeout */
        csp_conn_t *conn;
        if ((conn = csp_accept(&sock, 10000)) == NULL)
        {
            /* timeout */
            continue;
        }

        /* Read packets on connection, timout is 100 mS */
        csp_packet_t *packet;
    }
}

int main(int argc, char *argv[])
{
    cspftp_server_run();
    return 0;
}