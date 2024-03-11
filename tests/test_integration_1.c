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
#include <vmem/vmem_mmap.h>
#include "unity.h"
#include "unity_test_utils.h"

extern cspftp_opt_session_hooks_cfg apm_session_hooks;

cspftp_opt_session_hooks_cfg default_session_hooks;

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
    /* This is very important, else the default no-op hooks will be used */
    default_session_hooks = apm_session_hooks;
	run_in_thread(router, "router");
	run_in_thread(server, "server");
	sleep(1);
}

void tearDown() {

}

VMEM_DEFINE_FILE(dtp_test_session, "dtp_test_session.json", "dtp_test_session.json", 1024*4);


REGISTER_TEST(in_process_transfer) {
	cspftp_t *session;
	TEST_ASSERT(0 == dtp_client_main(0, 1024*16, 2, &session));
	cspftp_serialize_session(session, &vmem_dtp_test_session);
}

int main(int argc, const char *argv[])
{
	return UNITY_UTIL_MAIN();
}