#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 500
#define _GNU_SOURCE
#define __USE_GNU
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <csp/csp.h>
#include "unity.h"
#include "unity_test_utils.h"

static pid_t server_pid;
static pid_t zmqproxy_pid;

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
			/* child process*/
			execlp("csh", "csh", "-i", "conf/dtp_server.csh", NULL);
		} else if(server_pid > 0) {
			zmqproxy_pid = fork();
			if (zmqproxy_pid == 0)
			{
				/* child process*/
				execlp("zmqproxy", "zmqproxy", NULL);
			}
		}
	}
}

void tearDown()
{
}

REGISTER_TEST(inter_process_transfer)
{
	if (csh_available) {
		char *cmd_line = "csh -i conf/dtp_client.csh \"dtp_client -s 3\"";
		int res = system(cmd_line);
		kill(server_pid, 9);
		kill(zmqproxy_pid, 9);
		TEST_ASSERT(0 == res);
	} else {
		TEST_IGNORE_MESSAGE("csh not found in path");
	}
}

int main(int argc, const char *argv[])
{
	return UNITY_UTIL_MAIN();
}