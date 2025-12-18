#include <stdio.h>
#include <stdlib.h>
#include <csp/csp.h>
#include <csp/arch/csp_time.h>
#include "dtp/dtp.h"
#include "dtp/dtp_log.h"
#include "dtp/dtp_session.h"
#include <dtp/dtp_client_posix_segments_utils.h>

static void apm_on_start(dtp_t *session);
static bool apm_on_data_packet(dtp_t *session, csp_packet_t *p);
static void apm_on_end(dtp_t *session);
static void apm_on_serialize(dtp_t *session, void *ctx);
static void apm_on_deserialize(dtp_t *session, void *ctx);
static void apm_on_release(dtp_t *session);
static void apm_on_dump_info(dtp_t *session, uint32_t info, void *context);

const dtp_opt_session_hooks_cfg apm_session_hooks = {
    .on_start = apm_on_start,
    .on_data_packet = apm_on_data_packet,
    .on_end = apm_on_end,
    .on_serialize = apm_on_serialize,
    .on_deserialize = apm_on_deserialize,
    .on_release = apm_on_release,
    .on_dump_info = apm_on_dump_info,
    .hook_ctx = 0
};

typedef struct {
    uint32_t last_packet_ts;
    segments_ctx_t *segments;
    segments_ctx_t *received_segments;
    FILE *output;
} hook_ctx_t;

static void apm_on_start(dtp_t *session) {
    hook_ctx_t *ctx;
    if (!session->hooks.hook_ctx) {
        /* Allocate memory for a hook context object */
        ctx = malloc(sizeof(hook_ctx_t));
        /* Allocate a segments context object to hold the segments being received */
        segments_ctx_t *segments = init_segments_ctx();
        ctx->segments = segments;
        /* Reset the list of segments de-serialized from the meta file */
        ctx->received_segments = NULL;
        ctx->last_packet_ts = 0;
        ctx->output = NULL;
        session->hooks.hook_ctx = ctx;
    } else {
        ctx = session->hooks.hook_ctx;
    }

    char *filename = dtp_session_get_user_ctx(session);
    char dtp_file_name[1024];
    sprintf(dtp_file_name, "%s", filename);
    if (session->bytes_received > 0) {
        ctx->output = fopen(dtp_file_name, "r+b");
    } else {
        ctx->output = fopen(dtp_file_name, "wb");
    }
}

static bool apm_on_data_packet(dtp_t *session, csp_packet_t *packet) {
    segments_ctx_t *segments = ((hook_ctx_t *)session->hooks.hook_ctx)->segments;
    uint32_t last_ts = ((hook_ctx_t *)session->hooks.hook_ctx)->last_packet_ts;
    uint32_t now = csp_get_ms();
    uint32_t packet_seq = packet->data32[0] / (session->request_meta.mtu - (2 * sizeof(uint32_t)));

    if((now - last_ts) > 150) {
        printf("\33[2K\r");
        printf("%"PRIu32"/%"PRIu32, session->bytes_received, session->payload_size);
        fflush(stdout);
        ((hook_ctx_t *)session->hooks.hook_ctx)->last_packet_ts = now;
    }
    hook_ctx_t *ctx = (hook_ctx_t *)session->hooks.hook_ctx;
    fseek(ctx->output, packet_seq * (session->request_meta.mtu - (2 * sizeof(uint32_t))), SEEK_SET);
    fwrite(&packet->data32[2], (packet->length - (2 * sizeof(uint32_t))), 1, ctx->output);
    return update_segments(segments, packet_seq);
}

static void apm_on_end(dtp_t *session) {
    hook_ctx_t *ctx = (hook_ctx_t *)session->hooks.hook_ctx;
    if(ctx->output) {
        fclose(ctx->output);
    }
    close_segments(ctx->segments);
    uint32_t nof_segments = get_nof_segments(ctx->segments);
    dbg_log("\nReceived segments (%" PRIu32 "):", nof_segments);
    print_segments(ctx->segments);

    segments_ctx_t *received_segments = ((hook_ctx_t *)session->hooks.hook_ctx)->received_segments;
    if (ctx->segments && received_segments) {
        /* Merge the two segments together */
        segments_ctx_t *merged = merge_segments(ctx->segments, received_segments);
        if (merged) {
            free_segments(ctx->segments);
            ctx->segments = merged;
        }
    }

    if(ctx->segments) {
        uint32_t count = session->payload_size / (session->request_meta.mtu - (2 * sizeof(uint32_t)));
        if (session->payload_size % (session->request_meta.mtu - (2 * sizeof(uint32_t))) != 0) {
            count++;
        }
                
        segments_ctx_t *complements = get_complement_segment(ctx->segments, 0, count - 1);
        nof_segments = get_nof_segments(complements);
        dbg_log("Missing segments (%" PRIu32 "):", nof_segments);
        print_segments(complements);
        free_segments(complements);
    }

    dbg_log("Done");
}

static void apm_on_release(dtp_t *session) {
    if(session->hooks.hook_ctx != NULL) {
        segments_ctx_t *segments = ((hook_ctx_t *)session->hooks.hook_ctx)->segments;
        free_segments(segments);
        ((hook_ctx_t *)session->hooks.hook_ctx)->segments = NULL;
        segments = ((hook_ctx_t *)session->hooks.hook_ctx)->received_segments;
        free_segments(segments);
        ((hook_ctx_t *)session->hooks.hook_ctx)->received_segments = NULL;
        free(session->hooks.hook_ctx);
    }
    session->hooks.hook_ctx = 0;
}

static bool write_segment_to_file(uint32_t _1, uint32_t start, uint32_t end, void *output) {
    fwrite(&start, sizeof(start), 1, output);
    fwrite(&end, sizeof(end), 1, output);
    return true;
}

static void apm_on_serialize(dtp_t *session, void *ctx) {
    FILE *f;
    {
        char *filename = dtp_session_get_user_ctx(session);
        char dtp_file_name[1024];
        sprintf(dtp_file_name, "%s.meta", filename);
        f = fopen(dtp_file_name, "wb");
    }
    if (f) {
        // For future development, stamp the version as the first 32bits in the file
        fwrite(&DTP_SESSION_VERSION, sizeof(DTP_SESSION_VERSION), 1, f);
        fwrite(&session->remote_cfg.node, sizeof(session->remote_cfg.node), 1, f);
        // Write size of transmission unit, to compute size from sequence number
        fwrite(&session->request_meta.mtu, sizeof(session->request_meta.mtu), 1, f);
        fwrite(&session->timeout, sizeof(session->timeout), 1, f);
        fwrite(&session->request_meta.throughput, sizeof(session->request_meta.throughput), 1, f);
        fwrite(&session->request_meta.payload_id, sizeof(session->request_meta.payload_id), 1, f);
        fwrite(&session->request_meta.session_id, sizeof(session->request_meta.session_id), 1, f);
        fwrite(&session->bytes_received, sizeof(session->bytes_received), 1, f);
        fwrite(&session->payload_size, sizeof(session->payload_size), 1, f);
        segments_ctx_t *segments = ((hook_ctx_t *)session->hooks.hook_ctx)->segments;
        if(segments) {
            // number of received segments
            uint32_t nof_segments = get_nof_segments(segments);
            fwrite(&nof_segments, sizeof(nof_segments), 1, f);
            for_each_segment(segments, write_segment_to_file, f);
        }
        fclose(f);
    } else {
        dbg_warn("Serialization: could not open dtp_session_meta.bin!");
    }

}

static bool create_meta_data(uint32_t idx, uint32_t start, uint32_t end, void *output) {

    bool ret = true;
    dtp_meta_req_t *meta = (dtp_meta_req_t *)output;
    const uint8_t max_nof_intervals = sizeof(meta->intervals) / sizeof(meta->intervals[0]);
    if (idx < max_nof_intervals) {
        meta->intervals[idx].start = start;
        meta->intervals[idx].end = end;
        meta->nof_intervals = (idx + 1);
    } else {
        dbg_warn("Too many segments, only %u intervals supported!", max_nof_intervals);
        ret = false;
    }
    return ret;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
static void apm_on_deserialize(dtp_t *session, void *ctx) {
    FILE *f = NULL;
    char *filename = dtp_session_get_user_ctx(session);
    if (filename == NULL) {
        dbg_warn("No filename provided for deserialization!");
        return;
    }

    char dtp_file_name[256];
    sprintf(dtp_file_name, "%s.meta", filename);
    f = fopen(dtp_file_name, "rb");
    if (f == NULL) {
        dbg_warn("Deserialization: could not open '%s'!", dtp_file_name);
        session->dtp_errno = DTP_EINVAL;
        return;
    }

    if (f) {
        uint32_t buffer = 0;
        fread(&buffer, sizeof(DTP_SESSION_VERSION), 1, f);
        if(buffer != DTP_SESSION_VERSION) {
            dbg_warn("Session was serialized with a different DTP version (read: %u, current version: %u)!", DTP_SESSION_VERSION, buffer);
        } else {
            fread(&session->remote_cfg.node, sizeof(session->remote_cfg.node), 1, f);
            fread(&session->request_meta.mtu, sizeof(session->request_meta.mtu), 1, f);
            fread(&session->timeout, sizeof(session->timeout), 1, f);
            fread(&session->request_meta.throughput, sizeof(session->request_meta.throughput), 1, f);
            fread(&session->request_meta.payload_id, sizeof(session->request_meta.payload_id), 1, f);
            fread(&session->request_meta.session_id, sizeof(session->request_meta.session_id), 1, f);
            fread(&session->bytes_received, sizeof(session->bytes_received), 1, f);
            fread(&session->payload_size, sizeof(session->payload_size), 1, f);

            segments_ctx_t *received_segments = init_segments_ctx();
            hook_ctx_t *ctx = malloc(sizeof(hook_ctx_t));

            uint32_t nof_segments = 0;
            fread(&nof_segments, sizeof(nof_segments), 1, f);
            for (uint32_t i = 0; i < nof_segments; i++) {
                uint32_t start = 0;
                uint32_t end = 0;
                fread(&start, sizeof(start), 1, f);
                fread(&end, sizeof(end), 1, f);
                add_segment(received_segments, start, end);
            }

            /* Create the meta request from the received segments in the meta file */
            segments_ctx_t *segments = get_complement_segment(received_segments, 0, session->payload_size / (session->request_meta.mtu - (2 * sizeof(uint32_t))));
            nof_segments = get_nof_segments(segments);
            if (nof_segments > sizeof(session->request_meta.intervals)/sizeof(session->request_meta.intervals[0])) {
                nof_segments = sizeof(session->request_meta.intervals)/sizeof(session->request_meta.intervals[0]);
            }
            if(nof_segments) {
                for_each_segment(segments, create_meta_data, &session->request_meta);
            } else {
                session->request_meta.nof_intervals = 0;
            }
            free_segments(segments);

            ctx->received_segments = received_segments;
            ctx->last_packet_ts = 0;
            ctx->segments = init_segments_ctx();
            ctx->output = NULL;

            session->hooks.hook_ctx = ctx;
        }
        fclose(f);
    }
    dbg_log("apm_on_deserialize: done");
}
#pragma GCC diagnostic pop

static bool segment_dumper(uint32_t idx, uint32_t start, uint32_t end, void *context) {
    if (end == UINT32_MAX) {
        printf("\t\tinterval #%"PRIu32": start=%"PRIu32", end=Rest of payload\n", idx, start);
    } else {
        printf("\t\tinterval #%"PRIu32": start=%"PRIu32", end=%"PRIu32"\n", idx, start, end);
    }
    return true;
}

static void apm_on_dump_info(dtp_t *session, uint32_t info, void *context) {

    switch (info) {
        case 0:
        {
            printf("  remote node address: %u\n", session->remote_cfg.node);
            printf("  timeout: %u [s]\n", session->timeout);
            printf("  throughput: %"PRIu32" KByte/s\n", session->request_meta.throughput);
            printf("  MTU: %u Bytes\n", session->request_meta.mtu);
            printf("  payload id: %u\n", session->request_meta.payload_id);
            printf("  session id: %"PRIu32"\n", session->request_meta.session_id);
            printf("  bytes_received: %"PRIu32" Bytes\n", session->bytes_received);
            printf("  payload size: %"PRIu32" Bytes\n", session->payload_size);
            printf("  Missing: %"PRIu32" Bytes\n", session->payload_size - session->bytes_received);
            hook_ctx_t *ctx = (hook_ctx_t *)session->hooks.hook_ctx;
            if (ctx == NULL) {
                printf("  No segments received\n");
                return;
            }
            segments_ctx_t *segments = ctx->received_segments;
            uint32_t nof_segments = get_nof_segments(segments);
            printf("  Received Intervals: %"PRIu32"\n", nof_segments);
            for_each_segment(segments, segment_dumper, NULL);
        }
        break;
        default:
            printf("Unknown info type\n");
            return;        
    }
}