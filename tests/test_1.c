#include "unity.h"
#include "unity_test_utils.h"

#include "cspftp/cspftp.h"

void setUp(void) {
}

void tearDown(void) {
}

REGISTER_TEST(test_one) {
    cspftp_t *session = cspftp_new();
    char data[] = "This is what we want to send";
    printf("\t%s\n", data);
    cspftp_result res = cspftp_config(session, &data[0], sizeof(data));
    TEST_ASSERT(res == CSPFTP_OK);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_one);
    return UNITY_END();
}