#include <stdlib.h>
#include "unity.h"
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
    typedef struct segment_t segment_t;
    typedef struct segment_t {
        uint16_t start;
        uint16_t end;
        struct segment_t *next;
    } segment_t;
    typedef segment_t segments_t;
    segments_t *segments = NULL;
    segment_t *cur_segment = NULL;
    uint16_t i = 0;
    for (; i < 1024; i++) {
        if (cur_missing_packet < nof_missing_packets) {
            if (i == missing_packets[cur_missing_packet]) {
                cur_missing_packet++;
                if(!cur_segment) {
                    cur_segment = malloc(sizeof(segment_t));
                    cur_segment->start = i;
                    cur_segment->end = i;
                    cur_segment->next = NULL;
                } else {
                    cur_segment->end = i;
                }
            } else {
                if(cur_segment) {
                    cur_segment->end = i;
                    segment_t *runner = segments;
                    if(!runner) {
                        segments = cur_segment;
                    } else {
                        while(runner->next) {
                            runner = runner->next;
                        }
                        runner->next = cur_segment;
                    }
                    cur_segment = NULL;
                }
            }
        } else {
            if(cur_segment) {
                cur_segment->end = i;
            }
        }
    }
    if(cur_segment) {
        cur_segment->end = i;
        segment_t *runner = segments;
        if(!runner) {
            segments = cur_segment;
        } else {
            while(runner->next) {
                runner = runner->next;
            }
            runner->next = cur_segment;
        }
    }
    
    segment_t *runner  = segments;
    i = 0;
    segment_t *tmp;
    while(runner) {
        tmp = runner;
        printf("Segment #%d, start=%d, end=%d\n", i++, runner->start, runner->end);
        runner = runner->next;
        free(tmp);
    }
    TEST_ASSERT(cur_segment != NULL);
    TEST_ASSERT(i == 3);
}

int main(int argc, const char *argv[])
{
	return UNITY_UTIL_MAIN();
}
