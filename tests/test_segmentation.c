#include <stdlib.h>
#include "unity.h"
#include "segments_utils.h"
#include "unity_test_utils.h"

void setUp()
{
}

void tearDown()
{
}

REGISTER_TEST(segmentation)
{
    uint16_t missing_packets[] = {
        256,
        257,
        258,
        259,
        692,
        1023,
    };
    uint16_t nof_missing_packets = sizeof(missing_packets) / sizeof(missing_packets[0]);
    uint16_t cur_missing_packet = 0;

    uint16_t i = 0;
    for (; i < 1024; i++) {
        if (cur_missing_packet < nof_missing_packets) {
            if (i == missing_packets[cur_missing_packet]) {
                cur_missing_packet++;
            } else {
                update_segments(i);
            }
        } else {
            update_segments(i);
        }
    }
    close_segments(i);
    TEST_ASSERT(get_nof_segments() == 3);
}

int main(int argc, const char *argv[])
{
	return UNITY_UTIL_MAIN();
}
