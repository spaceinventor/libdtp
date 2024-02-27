#include <time.h>
#include <unistd.h>
#define __USE_GNU
#include <pthread.h>
#include <csp/csp.h>
#include <cspftp/cspftp.h>
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

REGISTER_TEST(in_process_transfer) {
	cspftp_t *session;
	TEST_ASSERT(0 == dtp_client_main(0, &session));
}

int main(int argc, const char *argv[])
{
	return UNITY_UTIL_MAIN();
}