#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "cspftp_log.h"
#include "segments_utils.h"

typedef struct segment_t segment_t;
typedef struct segment_t {
    uint32_t start;
    uint32_t end;
    struct segment_t *next;
} segment_t;
typedef segment_t segments_t;

/* These will eventually end up making a context */
static segments_t *segments = 0;
static segment_t *cur_segment = 0;
static uint32_t nof_segments = 0;
static bool started = false;

static void append_segment(segment_t *s) {
    segment_t *runner = segments;
    nof_segments++;
    if(!runner) {
        segments = cur_segment;
    } else {
        while(runner->next) {
            runner = runner->next;
        }
        runner->next = cur_segment;
    }
}

bool update_segments(uint32_t received) {
    if (false == started) {
        /* First received packet, may or may not be the actual first one sent */
        nof_segments = 0;
        started = true;
        cur_segment = malloc(sizeof(segment_t));
        if (0 == cur_segment) {
            return false;
        }
        cur_segment->start = received;
        cur_segment->end = received;
        cur_segment->next = 0;
    } else {
        if((cur_segment->end + 1) == received) {
            /* Consecutive chunk, good */
            cur_segment->end = received;
        } else {
            /* Missed some chunks, close current segment and create a new one */
            append_segment(cur_segment);
            cur_segment = malloc(sizeof(segment_t));
            if (0 == cur_segment) {
                return false;
            }
            cur_segment->start = received;
            cur_segment->end = received;
            cur_segment->next = 0;
        }
    }
    return true;
}

void close_segments(uint32_t received) {
    if(cur_segment) {
        append_segment(cur_segment);
    }
}

void print_segments() {
    segment_t *runner  = segments;
    uint32_t i = 0;
    while(runner) {
        dbg_log("Segment #%d, start=%d, end=%d", i++, runner->start, runner->end);
        runner = runner->next;
    }
}

uint32_t get_nof_segments(uint32_t expected) {
    return nof_segments;
}

void free_segments() {
    segment_t *runner  = segments;
    segment_t *tmp;
    while(runner) {
        tmp = runner;
        runner = runner->next;
        free(tmp);
    }
}

void for_each_segment(void (*cb)(uint32_t idx, uint32_t start, uint32_t end, void *cb_ctx), void *cb_ctx) {
    segment_t *runner  = segments;
    uint32_t i = 0;
    while(runner) {
        cb(i, runner->start, runner->end, cb_ctx);
        runner = runner->next;
    }
}