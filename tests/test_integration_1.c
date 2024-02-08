#include <unistd.h>
#define __USE_GNU
#include <pthread.h>
#include <csp/csp.h>

extern int dtp_server_main(int argc, char *argv[]);
void client(void);

static void *server() {
    dtp_server_main(0, NULL);
    return NULL;
}

static void * router(void * param) {

	/* Here there be routing */
	while (1) {
		csp_route_work();
	}

	return NULL;
}

static int run_in_thread(void * (*routine)(void *), const char *name) {

	pthread_attr_t attributes;
	pthread_t handle;
	int ret;

	if (pthread_attr_init(&attributes) != 0) {
		return -1;
	}
	/* no need to join with thread to free its resources */
	pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);

	ret = pthread_create(&handle, &attributes, routine, NULL);    
	pthread_attr_destroy(&attributes);

	if (ret != 0) {
		return ret;
	}
    pthread_setname_np(handle, name);
	return 0;
}

extern int dtp_client_main(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    csp_init();
    run_in_thread(router, "router");
    run_in_thread(server, "server");
	sleep(1);
    return dtp_client_main(argc, argv);
}