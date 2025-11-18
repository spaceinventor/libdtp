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

static segments_ctx_t *generate_test_segments() {
    segments_ctx_t *ctx = init_segments_ctx();
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
                update_segments(ctx, i);
            }
        } else {
            update_segments(ctx, i);
        }
    }
    close_segments(ctx);
    return ctx;
}

REGISTER_TEST(segmentation)
{
    segments_ctx_t *ctx = generate_test_segments();
    print_segments(ctx);
    TEST_ASSERT(get_nof_segments(ctx) == 3);
    free_segments(ctx);

}

REGISTER_TEST(compute_complement_segments) {
    segments_ctx_t *ctx = generate_test_segments();
    segments_ctx_t *complements = get_complement_segment(ctx, 0, 1);
    TEST_ASSERT(get_nof_segments(ctx) == 3);
    print_segments(complements);
    free_segments(ctx);
    free_segments(complements);
}

int main(int argc, const char *argv[])
{
	return UNITY_UTIL_MAIN();
}
