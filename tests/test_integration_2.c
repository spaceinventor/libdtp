// #define _GNU_SOURCE // nanosleep
#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 500
#define _GNU_SOURCE
#define __USE_GNU
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <csp/csp.h>
#include "unity.h"
#include "unity_test_utils.h"

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

static pid_t server_pid;

static bool exists(const char fname[])
{
    return access(fname, F_OK | X_OK) != -1;
}

static bool find_in_path(const char name[]) {
	char *fullpath = 0;
    bool found = false;
	char *org_paths = getenv("PATH");
	if(NULL != org_paths) {
		char *paths = strdup(org_paths);
		char *tmp = paths; // to use in free
		const char *item;
		while ((item = strsep(&paths, ":")) != NULL) {
			ssize_t buf_size = snprintf(NULL, 0, "%s/%s", item, name);
			fullpath = malloc(buf_size + 1);
			snprintf(fullpath, buf_size + 1, "%s/%s", item, name);
			if (exists(fullpath)) {
				free(fullpath);
				found = true;
				break;
			} else {
				free(fullpath);
			}
		}
		free(tmp);
	}
    return found;
}

static bool csh_available = false;

void setUp()
{
	/* Detect csh ...*/
	csh_available = find_in_path("csh") && find_in_path("zmqproxy");
	if (csh_available ) {
		server_pid = fork();
		if (server_pid == 0)
		{
			server_pid = fork();
			if (server_pid == 0)
			{
				/* child process*/
				execlp("csh", "csh", "-i/home/jbl/spaceinventor/cspftp/conf/dtp_server.csh", NULL);
			} else if (server_pid > 0) {
				/* child process*/
				execlp("zmqproxy", "zmqproxy", NULL);
			}
			else {
				perror("fork fail");
				exit(1);
			}
		} else if (server_pid < 0) {
			perror("fork fail");
			exit(1);
		}
	}
}

void tearDown()
{
}

REGISTER_TEST(inter_process_transfer)
{
	if (csh_available) {
		extern int dtp_client_main(int argc, char *argv[]);
		char *cmd_line = "csh -i conf/dtp_client.csh";
		TEST_ASSERT(0 == system(cmd_line));
	} else {
		TEST_IGNORE_MESSAGE("csh not found in path");
	}
}

int main(int argc, const char *argv[])
{
	return UNITY_UTIL_MAIN();
}