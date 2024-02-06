#include "unity.h"
#include "unity_test_utils.h"

#include "cspftp/cspftp.h"
#include "vmem/vmem_ram.h"

static uint8_t vmem_ram[256] = {0};
VMEM_DEFINE_STATIC_RAM_ADDR(session_serialize_test, "session_serialize_test", sizeof(uint32_t), &vmem_ram[0]);

void setUp(void) {
}

void tearDown(void) {
}

REGISTER_TEST(test_create_destroy) {
    cspftp_t *session;
    cspftp_result res = cspftp_new_session(&session);
    TEST_ASSERT(res == CSPFTP_OK);
}

REGISTER_TEST(test_serialize_session) {
    cspftp_t *session;
    cspftp_result res = cspftp_new_session(&session);
    TEST_ASSERT(res == CSPFTP_OK);
    res = cspftp_serialize_session(session, &vmem_session_serialize_test);
    TEST_ASSERT(res == CSPFTP_OK);
}


REGISTER_TEST(test_options) {
    cspftp_t *session = cspftp_acquire_session();    
    cspftp_params r_info = {
        .remote_info.remote = 0
    };
    cspftp_result res = cspftp_set_opt(session, CSPFTP_REMOTE_INFO, &r_info);
    TEST_ASSERT(res == CSPFTP_OK);
    r_info.remote_info.remote = 255;
    res = cspftp_get_opt(session, CSPFTP_REMOTE_INFO, &r_info);
    TEST_ASSERT(res == CSPFTP_OK);
    TEST_ASSERT(0 == r_info.remote_info.remote);
}

int main(int argc, const char *argv[]) {
    return UNITY_UTIL_MAIN();
}