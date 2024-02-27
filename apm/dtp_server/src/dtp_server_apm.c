#include <apm/apm.h>
#include <slash/slash.h>
#include <slash/optparse.h>
#include <cspftp/cspftp.h>
#define __USE_GNU
#include <pthread.h>


int apm_init(void) {
    return 0;
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

bool keep_running = true;

void *wrapper(void *p) {
    dtp_server_main(&keep_running);
    return 0;
}

int dtp_server(struct slash *s) {
    optparse_t * parser = optparse_new("dtp_server start", "");
    optparse_add_help(parser);

    int argi = optparse_parse(parser, s->argc - 1, ((const char **)s->argv) + 1);
    if (argi < 0) {
        optparse_del(parser);
        return SLASH_EINVAL;
    }

    optparse_del(parser);
    keep_running = true;
    run_in_thread(wrapper, "DTP server");
    return SLASH_SUCCESS;
}

int dtp_server_stop(struct slash *s) {
    keep_running = false;
    return SLASH_SUCCESS;
}

// slash_command(dtp_server, dtp_server, "", "DTP server");
slash_command_sub(dtp_server, start, dtp_server, "", NULL);
slash_command_sub(dtp_server, stop, dtp_server_stop, "", NULL);
