#include "unity.h"
#include "unity_test_utils.h"

#include "dtp/dtp.h"
#include "vmem/vmem_ram.h"

static uint8_t vmem_ram[256] = {0};
VMEM_DEFINE_STATIC_RAM_ADDR(session_serialize_test, "session_serialize_test", sizeof(uint32_t), &vmem_ram[0]);

void setUp(void) {
}

void tearDown(void) {
}

REGISTER_TEST(test_create_destroy) {
    dtp_t *session = dtp_acquire_session();
    TEST_ASSERT(0 != session);
}

REGISTER_TEST(test_serialize_session) {
    dtp_t *session = dtp_acquire_session();
    TEST_ASSERT(0 != session);
    dtp_result res = dtp_serialize_session(session, &vmem_session_serialize_test);
    TEST_ASSERT(res == DTP_OK);
}


REGISTER_TEST(test_options) {
    dtp_t *session = dtp_acquire_session();    
    dtp_params r_info = {
        .remote_cfg.node = 0
    };
    dtp_result res = dtp_set_opt(session, DTP_REMOTE_CFG, &r_info);
    TEST_ASSERT(res == DTP_OK);
    r_info.remote_cfg.node = 255;
    res = dtp_get_opt(session, DTP_REMOTE_CFG, &r_info);
    TEST_ASSERT(res == DTP_OK);
    TEST_ASSERT(0 == r_info.remote_cfg.node);
}

int main(int argc, const char *argv[]) {
    return UNITY_UTIL_MAIN();
}