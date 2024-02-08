#include <csp/csp.h>
#include "cspftp_protocol.h"
#include "cspftp_session.h"

cspftp_result send_remote_meta_req(cspftp_t *session) {
    cspftp_result res = CSPFTP_OK;
    csp_packet_t *packet = csp_buffer_get(0);
    if(packet) {
        packet->length = sizeof(cspftp_meta_req_t);
        cspftp_meta_req_t *res = (cspftp_meta_req_t *)packet->data;
        res->ts = 0xff00ff00;
        csp_send(session->conn, packet);
    } 
    return res;
}

cspftp_result read_remote_meta_resp(cspftp_t *session) {
    cspftp_result res = CSPFTP_OK;
    csp_conn_t * conn = session->conn;
    csp_packet_t *packet = csp_read(conn, 100000);
    if(packet) {        
        res = CSPFTP_OK;
        csp_buffer_free(packet);
    }
    return res;
}