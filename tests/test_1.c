#include "unity.h"

#include "cspftp/cspftp.h"

void setUp(void) {
}

void tearDown(void) {
}

void test_one() {
    cspftp *session = cspftp_new();
    char data[] = "This is what we want to send";
    cspftp_result res = cspftp_config(session, &data[0], sizeof(data));
    TEST_ASSERT(res == CSPFTP_OK);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_one);
    return UNITY_END();
}