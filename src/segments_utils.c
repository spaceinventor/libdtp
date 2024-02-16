#include <stdint.h>
#include <stdlib.h>
#include "cspftp_log.h"

typedef struct segment_t segment_t;
typedef struct segment_t {
    uint16_t start;
    uint16_t end;
    struct segment_t *next;
} segment_t;
typedef segment_t segments_t;
segments_t *segments = 0;
segment_t *cur_segment = 0;

void update_segments(uint32_t received, uint32_t expected) {
    if (received != expected) {
        if(!cur_segment) {
            cur_segment = malloc(sizeof(segment_t));
            cur_segment->start = received;
            cur_segment->end = expected;
            cur_segment->next = 0;
        } else {
            cur_segment->end = received;
        }
    } else {
        if(cur_segment) {
            cur_segment->end = received;
            segment_t *runner = segments;
            if(!runner) {
                segments = cur_segment;
            } else {
                while(runner->next) {
                    runner = runner->next;
                }
                runner->next = cur_segment;
            }
            cur_segment = 0;
        }
    }
    if(cur_segment) {
        cur_segment->end = received;
    }
}

void close_segments(uint32_t expected) {
    if(cur_segment) {
        cur_segment->end = expected;
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
    uint32_t i = 0;
    segment_t *tmp;
    while(runner) {
        tmp = runner;
        dbg_log("Segment #%d, start=%d, end=%d\n", i++, runner->start, runner->end);
        runner = runner->next;
        free(tmp);
    }
}
