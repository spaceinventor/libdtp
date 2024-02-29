#include <stdio.h>
#include <apm/apm.h>
#include <vmem/vmem_file.h>
#include <slash/slash.h>
#include <slash/optparse.h>
#include "cspftp/cspftp.h"
#include "segments_utils.h"
#include "cspftp_log.h"
#include "cspftp_session.h"

static void apm_on_start(cspftp_t *session);
static bool apm_on_data_packet(cspftp_t *session, csp_packet_t *p);
static void apm_on_end(cspftp_t *session);
static void apm_on_serialize(cspftp_t *session, vmem_t *output);
static void apm_on_deserialize(cspftp_t *session, vmem_t *input);
static void apm_on_release(cspftp_t *session);

const cspftp_opt_session_hooks_cfg default_session_hooks = {
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
    cur_len = snprintf(line_buf, 128, "{\n\t\"received\": %u,\n", session->bytes_received);
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

int apm_init(void)
{
    return 0;
}

VMEM_DEFINE_FILE(dtp_session, "dtp_session.json", "dtp_session.json", 1024*4);

int dtp_client(struct slash *s)
{
    typedef struct {
        int color;
        uint32_t server;
    } dtp_client_opts_t;
    dtp_client_opts_t opts = { 0 };
    optparse_t * parser = optparse_new("dtp_client", "<name>");
    optparse_add_help(parser);
    optparse_add_set(parser, 'c', "color", 1, &opts.color, "enable color output");
	optparse_add_unsigned(parser, 's', "server", "CSP address", 0, &opts.server, "CSP Address of the DT server to retrieve data from (default = 0))");

    int argi = optparse_parse(parser, s->argc - 1, ((const char **)s->argv) + 1);
    if (argi < 0) {
        optparse_del(parser);
        return SLASH_EINVAL;
    }

    optparse_del(parser);
    cspftp_t *session;
    cspftp_result result = dtp_client_main(opts.server, &session);
    cspftp_serialize_session(session, &vmem_dtp_session);

    if (CSPFTP_ERR == result) {
        switch(cspftp_errno(NULL)) {
            case CSPFTP_EINVAL:
                return SLASH_EINVAL;
            default:
                printf("%s\n", cspftp_strerror(cspftp_errno(NULL)));
                return SLASH_SUCCESS;
        }
    } else {
        cspftp_release_session(session);
    }
    return SLASH_SUCCESS;
}

slash_command(dtp_client, dtp_client, "-s", "DTP client");