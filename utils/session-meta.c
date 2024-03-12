#include <stdio.h>
#include "cspftp_session.h"

void parse_meta_data(FILE *f, cspftp_t *session) {
    uint32_t buffer = 0;
    fread(&buffer, sizeof(CSPFTP_SESSION_VERSION), 1, f);
    fread(&session->remote_cfg.node, sizeof(session->remote_cfg.node), 1, f);
    fread(&buffer, sizeof(CSPFTP_PACKET_SIZE), 1, f);
    fread(&session->request_meta.timeout, sizeof(session->request_meta.timeout), 1, f);
    fread(&session->request_meta.throughput, sizeof(session->request_meta.throughput), 1, f);
    fread(&session->request_meta.payload_id, sizeof(session->request_meta.payload_id), 1, f);
    fread(&session->bytes_received, sizeof(session->bytes_received), 1, f);
    fread(&session->total_bytes, sizeof(session->total_bytes), 1, f);
    // number of segments

    fread(&session->request_meta.nof_intervals, sizeof(session->request_meta.nof_intervals), 1, f);
    for (uint32_t i = 0; i < session->request_meta.nof_intervals; i++) {
        fread(&session->request_meta.intervals[i].start, sizeof(uint32_t), 1, f);
        fread(&session->request_meta.intervals[i].end, sizeof(uint32_t), 1, f);
    }
}

int main(int argc, const char *argv[]) {
    const char *file_name = "dtp_session_meta.bin";
    if (argc >= 2) {
        file_name = argv[1];
    }
    FILE *f = fopen(file_name, "rb");
    if (!f) {
        printf("Usage: session-meta [<dtp_session_meta.bin>]\n");
    } else {
        cspftp_t session = {0};
        parse_meta_data(f, &session);
        printf("Session data from file: %s\n", file_name);
        printf("  remote address: %u\n", session.remote_cfg.node);
        printf("  timeout: %u\n", session.request_meta.timeout);
        printf("  throughput: %u\n", session.request_meta.throughput);
        printf("  payload id: %u\n", session.request_meta.payload_id);
        printf("  bytes_received: %u\n", session.bytes_received);
        printf("  total_bytes: %u\n", session.total_bytes);
        printf("  Missing intervals: %u\n", session.request_meta.nof_intervals);
        for(uint8_t i = 0; i < session.request_meta.nof_intervals; i++) {
            printf("\t\tinterval #%u: start=%u, end=%u\n", i, session.request_meta.intervals[i].start, session.request_meta.intervals[i].end);
        }
        fclose(f);
    }

    return 0;
}