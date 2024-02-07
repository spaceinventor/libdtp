#include "unity.h"
#include "unity_test_utils.h"

#include "cspftp/cspftp.h"

void setUp(void) {
}

void tearDown(void) {
}

REGISTER_TEST(test_one) {
    cspftp_t *session;
    cspftp_result res = cspftp_new_session(&session);
    char data[] = "This is what we want to send";
    printf("\t%s\n", data);
    TEST_ASSERT(res == CSPFTP_OK);
}

REGISTER_TEST(test_two) {
    TEST_ASSERT(1);
}

int main(int argc, const char *argv[]) {
    return UNITY_UTIL_MAIN();
}