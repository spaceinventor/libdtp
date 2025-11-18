#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include "dtp/dtp_log.h"
#include "segments_utils.h"

typedef struct segment_t segment_t;
typedef struct segment_t {
    uint32_t start;
    uint32_t end;
    struct segment_t *next;
} segment_t;
typedef segment_t segments_t;

typedef struct segments_ctx_t {
    segments_t *segments;
    segment_t *cur_segment;
    uint32_t nof_segments;
    bool started;
} segments_ctx_t;

static void append_segment(segments_ctx_t *ctx) {
    segment_t *runner = ctx->segments;
    if(!runner) {
        ctx->segments = ctx->cur_segment;
        ctx->nof_segments++;
    } else {
        while(runner) {
            if(ctx->cur_segment->start >= runner->start && ctx->cur_segment->start <= runner->end) {
                if(ctx->cur_segment->end >= runner->end) {
                    // Current segment overlaps parially or completely with a previously dowloaded segment,
                    // update the existing segment and discard "cur_segment"
                    runner->end = ctx->cur_segment->end;
                    free(ctx->cur_segment);
                    ctx->cur_segment = 0;
                    break;
                }
            } else if(ctx->cur_segment->start <= runner->start && ctx->cur_segment->end >= runner->start) {
                runner->start = ctx->cur_segment->start;
                free(ctx->cur_segment);
                ctx->cur_segment = 0;
                break;
            }
            if(!runner->next) {
                runner->next = ctx->cur_segment;
                ctx->nof_segments++;
                break;
            }
            runner = runner->next;
        }
    }
}

extern segments_ctx_t *init_segments_ctx(void) {
    segments_ctx_t *res = calloc(1, sizeof(segments_ctx_t));
    return res;
}

bool update_segments_range(segments_ctx_t *ctx, uint32_t start, uint32_t end) {
    if (false == ctx->started) {
        /* First received packet, may or may not be the actual first one sent */
        ctx->nof_segments = 0;
        ctx->started = true;
        ctx->cur_segment = malloc(sizeof(segment_t));
        if (0 == ctx->cur_segment) {
            return false;
        }
        ctx->cur_segment->start = start;
        ctx->cur_segment->end = end;
        ctx->cur_segment->next = 0;
    } else {
        if((ctx->cur_segment->end + 1) >= start) {
            if (ctx->cur_segment->end >= end) {
                /* Already received this segment, ignore */
                return true;
            }
            /* Consecutive or overlapping chunk, good */
            ctx->cur_segment->end = end;
        } else {
            /* Missed some chunks, close current segment and create a new one */
            append_segment(ctx);
            ctx->cur_segment = malloc(sizeof(segment_t));
            if (0 == ctx->cur_segment) {
                return false;
            }
            ctx->cur_segment->start = start;
            ctx->cur_segment->end = end;
            ctx->cur_segment->next = 0;
        }
    }
    return true;
}

bool update_segments(segments_ctx_t *ctx, uint32_t received) {
    return update_segments_range(ctx, received, received);
}

void close_segments(segments_ctx_t *ctx) {
    if(ctx->cur_segment) {
        append_segment(ctx);
    }
}

segments_ctx_t *get_complement_segment(segments_ctx_t *ctx, uint32_t start, uint32_t end) {
    segments_ctx_t *complements = init_segments_ctx();
    uint32_t seg_start = 0;
    uint32_t seg_end = 0;
    if(0 != ctx->segments) {
        segment_t *runner  = ctx->segments;
        while(runner) {
            if(runner->start == start) {
                seg_start = runner->end + 1;
                runner = runner->next;
                continue;
            } else if(seg_start <= (runner->start - 1)) {
                seg_end = runner->start - 1;
                complements->cur_segment = malloc(sizeof(segment_t));
                complements->cur_segment->start = seg_start;
                complements->cur_segment->end = seg_end;
                complements->cur_segment->next = 0;
                append_segment(complements);
                seg_start = runner->end + 1;
            }
            runner = runner->next;
        }
    }
    if(seg_start < end) {
        if (seg_end < seg_start) {
            complements->cur_segment = malloc(sizeof(segment_t));
            complements->cur_segment->start = seg_start;
            complements->cur_segment->end = end;
            complements->cur_segment->next = 0;
            append_segment(complements);
        }
    }
    // else {
    //     complements->cur_segment = malloc(sizeof(segment_t));
    //     complements->cur_segment->start = end;
    //     complements->cur_segment->end = 0xffffffff;
    //     complements->cur_segment->next = 0;
    //     append_segment(complements);
    // }
    return complements;
}

void print_segments(segments_ctx_t *ctx) {
    if(0 != ctx->segments) {
        segment_t *runner  = ctx->segments;
        uint32_t i = 0;
        while(runner) {
            printf("Segment #%"PRIu32", start=%"PRIu32", end=%"PRIu32"\n", i++, runner->start, runner->end);
            runner = runner->next;
        }
    }
}

uint32_t get_nof_segments(segments_ctx_t *ctx) {
    return ctx->nof_segments;
}

void free_segments(segments_ctx_t *ctx) {
    if(0 != ctx && 0 != ctx->segments) {
        segment_t *runner  = ctx->segments;
        segment_t *tmp;
        while(runner) {
            tmp = runner;
            runner = runner->next;
            free(tmp);
        }
    }
    free(ctx);
}

void for_each_segment(segments_ctx_t *ctx, bool (*cb)(uint32_t idx, uint32_t start, uint32_t end, void *cb_ctx), void *cb_ctx) {
    if(0 != ctx->segments) {
        segment_t *runner  = ctx->segments;
        uint32_t idx = 0;
        while(runner) {
            if (!cb(idx, runner->start, runner->end, cb_ctx)) {
                break;
            }
            runner = runner->next;
            idx++;
        }
    }
}

void add_segment(segments_ctx_t *ctx, uint32_t start, uint32_t end) {
    segment_t *s = malloc(sizeof(segment_t));
    s->start = start;
    s->end = end;
    s->next = NULL;
    if (0 == ctx->segments) {
        ctx->segments = s;
    } else {
        segment_t *runner = ctx->segments;
        while (runner) {
            if (!runner->next) {
                runner->next = s;
                break;
            }
            runner = runner->next;
        }
    }
    ctx->nof_segments++;
}

segments_ctx_t *merge_segments(segments_ctx_t *a, segments_ctx_t *b) {
    segments_ctx_t *res = init_segments_ctx();
    segment_t *iter_a = a->segments;
    segment_t *iter_b = b->segments;

    uint32_t start = 0;
    uint32_t end = 0;
    while (iter_a || iter_b) {
        if (iter_a && iter_b) {
            if (iter_a->start < iter_b->start) {
                start = iter_a->start;
                end = iter_a->end;
                iter_a = iter_a->next;
            } else {
                start = iter_b->start;
                end = iter_b->end;
                iter_b = iter_b->next;
            }
        } else if (iter_a) {
            start = iter_a->start;
            end = iter_a->end;
            iter_a = iter_a->next;
        } else {
            start = iter_b->start;
            end = iter_b->end;
            iter_b = iter_b->next;
        }
        update_segments_range(res, start, end);
    }
    close_segments(res);

    return res;
}
