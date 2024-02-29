#include <time.h>
#include <unistd.h>
#define __USE_GNU
#include <pthread.h>
#include <csp/csp.h>
#include <cspftp/cspftp.h>
#include "segments_utils.h"
#include "cspftp_session.h"
#include "cspftp_log.h"
#include <vmem/vmem_file.h>
#include "unity.h"
#include "unity_test_utils.h"


void client(void);

static void *server()
{
    bool keep_running = true;
	dtp_server_main(&keep_running);
	return NULL;
}

static void *router(void *param)
{

	/* Here there be routing */
	while (1)
	{
		csp_route_work();
	}

	return NULL;
}

static int run_in_thread(void *(*routine)(void *), const char *name)
{

	pthread_attr_t attributes;
	pthread_t handle;
	int ret;

	if (pthread_attr_init(&attributes) != 0)
	{
		return -1;
	}
	/* no need to join with thread to free its resources */
	pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);

	ret = pthread_create(&handle, &attributes, routine, NULL);
	pthread_attr_destroy(&attributes);

	if (ret != 0)
	{
		return ret;
	}
	pthread_setname_np(handle, name);
	return 0;
}

void setUp() {
	csp_init();
	run_in_thread(router, "router");
	run_in_thread(server, "server");
	sleep(1);
}

void tearDown() {

}

VMEM_DEFINE_FILE(dtp_test_session, "dtp_test_session.json", "dtp_test_session.json", 1024*4);
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
    // .on_serialize = apm_on_serialize,
    .on_deserialize = apm_on_deserialize,
    .on_release = apm_on_release
};

static void apm_on_start(cspftp_t *session) {
    segments_ctx_t *segments = init_segments_ctx();
    session->hooks.hook_ctx = segments;
}

static bool apm_on_data_packet(cspftp_t *session, csp_packet_t *packet) {
    segments_ctx_t *segments = (segments_ctx_t *)session->hooks.hook_ctx;
    uint32_t packet_seq = packet->data32[0] / packet->length;
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
    // dbg_log("Received segments:");
    // print_segments(segments);
    // segments_ctx_t *complements = get_complement_segment(segments);
    // dbg_log("Missing segments:");
    // print_segments(complements);
    // dbg_log("Done");
    // free_segments(complements);
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
}

REGISTER_TEST(in_process_transfer) {
	cspftp_t *session;
	TEST_ASSERT(0 == dtp_client_main(0, &session));
	cspftp_serialize_session(session, &vmem_dtp_test_session);
}

int main(int argc, const char *argv[])
{
	return UNITY_UTIL_MAIN();
}