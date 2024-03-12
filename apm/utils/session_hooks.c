#include <stdio.h>
#include "vmem/vmem_mmap.h"
#include "cspftp/cspftp.h"
#include "cspftp_log.h"
#include "cspftp_session.h"
#include "segments_utils.h"

VMEM_DEFINE_MMAP(dtp_session, "dtp_session.json", "dtp_session.json", 1024);
VMEM_DEFINE_MMAP(dtp_session_meta, "dtp_session_meta.bin", "dtp_session_meta.bin", 1024);
VMEM_DEFINE_MMAP(dtp_data, "dtp_data.bin", "dtp_data.bin", 1024);

static void apm_on_start(cspftp_t *session);
static bool apm_on_data_packet(cspftp_t *session, csp_packet_t *p);
static void apm_on_end(cspftp_t *session);
static void apm_on_serialize(cspftp_t *session, vmem_t *output);
static void apm_on_deserialize(cspftp_t *session, vmem_t *input);
static void apm_on_release(cspftp_t *session);

const cspftp_opt_session_hooks_cfg apm_session_hooks = {
    .on_start = apm_on_start,
    .on_data_packet = apm_on_data_packet,
    .on_end = apm_on_end,
    .on_serialize = apm_on_serialize,
    .on_deserialize = apm_on_deserialize,
    .on_release = apm_on_release
};

static void apm_on_start(cspftp_t *session) {
    segments_ctx_t *segments = init_segments_ctx();
    session->hooks.hook_ctx = segments;
    uint32_t dummy = 0;
    /* Grow file to expected session size */
    VMEM_MMAP_VAR(dtp_data).write(&VMEM_MMAP_VAR(dtp_data), session->total_bytes - sizeof(dummy), &dummy, sizeof(dummy));
}

static bool apm_on_data_packet(cspftp_t *session, csp_packet_t *packet) {
    segments_ctx_t *segments = (segments_ctx_t *)session->hooks.hook_ctx;
    uint32_t packet_seq = packet->data32[0] / (CSPFTP_PACKET_SIZE - sizeof(uint32_t));
    VMEM_MMAP_VAR(dtp_data).write(&VMEM_MMAP_VAR(dtp_data), packet_seq * (CSPFTP_PACKET_SIZE - sizeof(uint32_t)), &packet->data32[1], (packet->length - sizeof(uint32_t)));
    return update_segments(segments, packet_seq);
}

typedef struct {
    vmem_t *output;
    uint32_t *offset;
} _anon;


static char line_buf[128] = { 0 };
static void write_segment_to_json(uint32_t idx, uint32_t start, uint32_t end, void *ctx) {
    uint32_t cur_len;
    _anon *out = (_anon *)ctx;
    cur_len = snprintf(line_buf, 128, "\n\t\t{ \"start\": %u, \"end\": %u },", start, end);
    out->output->write(out->output, *(out->offset), line_buf, cur_len);
    *(out->offset) += cur_len;
}

static void apm_on_end(cspftp_t *session) {
    segments_ctx_t *segments = (segments_ctx_t *)session->hooks.hook_ctx;
    close_segments(segments);
    dbg_log("Received segments:");
    print_segments(segments);
    segments_ctx_t *complements = get_complement_segment(segments);
    dbg_log("Missing segments:");
    print_segments(complements);
    dbg_log("Done");
    free_segments(complements);
}

static void apm_on_release(cspftp_t *session) {
    segments_ctx_t *segments = (segments_ctx_t *)session->hooks.hook_ctx;
    free_segments(segments);
}

static void segment_counter(uint32_t _1, uint32_t _2, uint32_t _3, void *counter) {    
    *(uint32_t *)counter = *(uint32_t *)counter + 1;
}

static void write_segment_to_file(uint32_t _1, uint32_t start, uint32_t end, void *output) {
    fwrite(&start, sizeof(start), 1, output);
    fwrite(&end, sizeof(end), 1, output);
}

static void apm_on_serialize(cspftp_t *session, vmem_t *output) {
    uint32_t offset = 0;
    uint32_t cur_len;
    FILE *f = fopen("dtp_session_meta.bin", "wb");
    if (!f) {
    } else {
        segments_ctx_t *segments = (segments_ctx_t *)session->hooks.hook_ctx;
        // For future development, stamp the version as the first 32bits in the file
        fwrite(&CSPFTP_SESSION_VERSION, sizeof(CSPFTP_SESSION_VERSION), 1, f);
        // Write size of transmission unit, to compute size from sequence number
        fwrite(&CSPFTP_PACKET_SIZE, sizeof(CSPFTP_PACKET_SIZE), 1, f);
        fwrite(&session->request_meta.timeout, sizeof(session->request_meta.timeout), 1, f);
        fwrite(&session->request_meta.throughput, sizeof(session->request_meta.throughput), 1, f);
        fwrite(&session->request_meta.payload_id, sizeof(session->request_meta.payload_id), 1, f);
        fwrite(&session->bytes_received, sizeof(session->bytes_received), 1, f);
        fwrite(&session->total_bytes, sizeof(session->total_bytes), 1, f);
        // number of missing segments
        uint8_t nof_segments = 0;
        segments_ctx_t *missing_segments = get_complement_segment(segments);
        for_each_segment(missing_segments, segment_counter, &nof_segments);
        fwrite(&nof_segments, sizeof(nof_segments), 1, f);
        for_each_segment(missing_segments, write_segment_to_file, f);
        free_segments(missing_segments);
        fclose(f);

        // cur_len = snprintf(line_buf, 128, "{\n\t\"payload_id\": %u,\n", session->request_meta.payload_id);
        // output->write(output, offset, line_buf, cur_len);
        // offset += cur_len;

        // cur_len = snprintf(line_buf, 128, "\t\"received\": %u,\n", session->bytes_received);
        // output->write(output, offset, line_buf, cur_len);
        // offset += cur_len;

        // cur_len = snprintf(line_buf, 128, "\t\"total\": %u,\n", session->total_bytes);
        // output->write(output, offset, line_buf, cur_len);
        // offset += cur_len;
        // cur_len = snprintf(line_buf, 128, "\t\"segments_received\": [ ");
        // output->write(output, offset, line_buf, cur_len);
        // offset += cur_len;

        // _anon ctx = {
        //     .output = output,
        //     .offset = &offset
        // };
        // for_each_segment(segments, write_segment_to_json, &ctx);
        // offset--; /* backtrack to the last comma */
        // cur_len = snprintf(line_buf, 128, "\n\t],\n\t\"segments_missing\": [ ");
        // output->write(output, offset, line_buf, cur_len);
        // offset += cur_len;
        // segments_ctx_t *missing = get_complement_segment(segments);
        // for_each_segment(missing, write_segment_to_json, &ctx);
        // free_segments(missing);
        // offset--; /* backtrack to the last comma */
        // cur_len = snprintf(line_buf, 128, "\n\t]\n}\n");
        // output->write(output, offset, line_buf, cur_len);
    }
}

static void apm_on_deserialize(cspftp_t *session, vmem_t *input) {
    FILE *f = fopen("dtp_session_meta.bin", "rb");
    if (f) {
        uint32_t buffer = 0;
        fread(&buffer, sizeof(CSPFTP_SESSION_VERSION), 1, f);
        if(buffer != CSPFTP_SESSION_VERSION) {
            dbg_warn("Session was serialized with a different DTP version (read: %u, current version: %u)!", CSPFTP_SESSION_VERSION, buffer);
        } else {
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
        fclose(f);
    }
    (void)session;
    (void)input;
     dbg_warn("apm_on_deserialize: TODO");
}
