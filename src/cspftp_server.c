#include <csp/csp.h>
#include "cspftp/cspftp.h"
#include "cspftp_protocol.h"
#include "cspftp_log.h"

static void cspftp_server_run()
{
    csp_socket_t sock = {0};
    csp_bind(&sock, 7);

    /* Create a backlog of 1 connection */
    csp_listen(&sock, 1);

    /* Wait for connections and then process packets on the connection */
    dbg_log("Waiting for connection...");
    while (1)
    {
        /* Wait for a new connection, 10000 mS timeout */
        csp_conn_t *conn;
        if ((conn = csp_accept(&sock, 10000)) == NULL)
        {
            /* timeout */
            continue;
        }
        dbg_log("Incoming connection");
        /* Read packets on connection, timout is 100 mS */
        csp_packet_t *packet = csp_read(conn, 10000);
        if (NULL == packet) {
            continue;
        }
        dbg_log("Got meta data request");
        packet = setup_server_transfer(&server_transfer_ctx, csp_conn_src(conn), packet);
        if(packet) {
            csp_send(conn, packet);
            start_sending_data(&server_transfer_ctx);
        }
        csp_close(conn);
    }
}

int dtp_server_main(int argc, char *argv[])
{
    cspftp_server_run();
    return 0;
}