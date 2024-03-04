#include <stdio.h>
#include "vmem/vmem_mmap.h"
#include "cspftp/cspftp.h"
#include "cspftp_log.h"
#include "cspftp_session.h"
#include "segments_utils.h"

VMEM_DEFINE_MMAP(dtp_session, "dtp_session.json", "dtp_session.json", 1024);
VMEM_DEFINE_MMAP(dtp_data, "dtp_data.bin", "dtp_data.bin", 1024 * 1024 * 129);
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

}

static bool apm_on_data_packet(cspftp_t *session, csp_packet_t *packet) {
    segments_ctx_t *segments = (segments_ctx_t *)session->hooks.hook_ctx;
    uint32_t packet_seq = packet->data32[0] / (CSPFTP_PACKET_SIZE - sizeof(uint32_t));
    VMEM_MMAP_VAR(dtp_data).write(&VMEM_MMAP_VAR(dtp_data), packet_seq * (CSPFTP_PACKET_SIZE - sizeof(uint32_t)), &packet->data32[1], (packet->length - 1));
    return update_segments(segments, packet_seq);
}

typedef struct {
    vmem_t *output;
    uint32_t *offset;
} _anon;


static char line_buf[128] = { 0 };
static void write_segment_to_file(uint32_t idx, uint32_t start, uint32_t end, void *ctx) {
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
    // dbg_log("Serializing session...");
    // cspftp_serialize_session_to_file(session, "dtp_session.json");
    dbg_log("Done");
    free_segments(complements);
}

static void apm_on_release(cspftp_t *session) {
    segments_ctx_t *segments = (segments_ctx_t *)session->hooks.hook_ctx;
    free_segments(segments);
}

static void apm_on_serialize(cspftp_t *session, vmem_t *output) {
    uint32_t offset = 0;
    uint32_t cur_len;
    cur_len = snprintf(line_buf, 128, "{\n\t\"payload_id\": %u,\n", session->request_meta.payload_id);
    output->write(output, offset, line_buf, cur_len);
    offset += cur_len;

    cur_len = snprintf(line_buf, 128, "\t\"received\": %u,\n", session->bytes_received);
    output->write(output, offset, line_buf, cur_len);
    offset += cur_len;

    cur_len = snprintf(line_buf, 128, "\t\"total\": %u,\n", session->total_bytes);
    output->write(output, offset, line_buf, cur_len);
    offset += cur_len;
    segments_ctx_t *segments = (segments_ctx_t *)session->hooks.hook_ctx;
    cur_len = snprintf(line_buf, 128, "\t\"segments_received\": [ ");
    output->write(output, offset, line_buf, cur_len);
    offset += cur_len;

    _anon ctx = {
        .output = output,
        .offset = &offset
    };
    for_each_segment(segments, write_segment_to_file, &ctx);
    offset--; /* backtrack to the last comma */
    cur_len = snprintf(line_buf, 128, "\n\t],\n\t\"segments_missing\": [ ");
    output->write(output, offset, line_buf, cur_len);
    offset += cur_len;
    segments_ctx_t *missing = get_complement_segment(segments);
    for_each_segment(missing, write_segment_to_file, &ctx);
    free_segments(missing);
    offset--; /* backtrack to the last comma */
    cur_len = snprintf(line_buf, 128, "\n\t]\n}\n");
    output->write(output, offset, line_buf, cur_len);
}

static void apm_on_deserialize(cspftp_t *session, vmem_t *input) {
    (void)session;
    (void)input;
    dbg_warn("apm_on_deserialize: TODO");
}
