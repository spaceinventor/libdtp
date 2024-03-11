#include "unity.h"
#include "unity_test_utils.h"
#include "cspftp_internal_api.h"

void setUp() {}
void tearDown() {}

REGISTER_TEST(test_compute_throughput) {
    uint32_t throughput = compute_throughput(1000, 0, 1000);
    TEST_ASSERT(1 == throughput);
}

int main(int argc, const char *argv[]) {
    return UNITY_UTIL_MAIN();
}