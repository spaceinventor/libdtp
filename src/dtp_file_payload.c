#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <dtp/platform.h>
#include <dtp/dtp_file_payload.h>

typedef struct payload_file_s {
    payload_file_t *next;
    uint8_t id;
    char *filename;
    FILE *file;
    uint64_t size;
    uint32_t (*read)(uint8_t payload_id, uint32_t offset, void *output, uint32_t size, void *context);
} payload_file_t;

static const char test_data[] = "0123456789ABCDE";
static uint32_t test_memory_payload_read(uint8_t payload_id, uint32_t offset, void *output, uint32_t size, void *context);

static payload_file_t test_payload = {
    .next = NULL,
    .id = 255,
    .filename = ":mem:",
    .file = NULL,
    .size = 1024 * 1024 * 20, /* 20 megabytes */
    .read = test_memory_payload_read
};

static payload_file_t *g_payload_list = &test_payload;


static uint32_t test_memory_payload_read(uint8_t payload_id, uint32_t offset, void *output, uint32_t size, void *context) {
    uint32_t res = 0;
    uint32_t so_far = 0;
    if (so_far % sizeof(test_data) == 0) {
        while(so_far < size) {
            memcpy(output + so_far, test_data, sizeof(test_data));
            so_far += sizeof(test_data);
        }
    } else {
        while(so_far < (size - sizeof(test_data))) {
            memcpy(output + so_far, test_data, sizeof(test_data));
            so_far += sizeof(test_data);
        }
        memcpy(output + so_far, test_data, so_far % sizeof(test_data));
    }
    res = size;
    return res;
}

static payload_file_t *payload_lookup(uint8_t id) {

    payload_file_t *iter = g_payload_list;
    payload_file_t *found = NULL;

    while (iter) {
        if (iter->id == id) {
            found = iter;
            break;
        }
        iter = iter->next;
    }

    return found;
}

static uint32_t payload_read(uint8_t payload_id, uint32_t offset, void *output, uint32_t size, void *context) {
    uint32_t res = 0;

    payload_file_t *payload = payload_lookup(payload_id);
    if (payload) {
        if (payload->size < offset) {
            size = 0;
        } else if (payload->size < (offset + size)) {
            size = offset - payload->size;
        }

        if (size > 0) {
            if (!payload->file) {
                payload->file = fopen(payload->filename, "rb");
            }
            if (payload->file) {
                fseek(payload->file, offset, SEEK_SET);
                size_t bytes_read = fread(output, 1, size, payload->file);
                res = bytes_read;
            }
        }
    }

    return res;
}

payload_file_t *dtp_file_payload_add(uint8_t id, const char *filename) {

    payload_file_t *payload = NULL;

    payload = payload_lookup(id);
    if (!payload) {
        payload = (payload_file_t *)malloc(sizeof(payload_file_t));
    }
    if (payload) {
        if (filename) {
            FILE *file = fopen(filename, "rb");
            if (file) {
                payload = (payload_file_t *)malloc(sizeof(payload_file_t));
                payload->filename = strdup(filename);
                payload->id = id;
                payload->file = NULL;
                payload->read = payload_read;
                fseek(file, 0, SEEK_END);
                payload->size = ftell(file);
                fclose(file);
                /* Insert the payload into the global list */
                payload->next = g_payload_list;
                g_payload_list = payload;
            } else {
                printf("ERROR: Could not open file '%s' - %s\n", filename, strerror(errno));
                payload = NULL;
            }
        }
    }

    return payload;
}

void dtp_file_payload_del(uint8_t id) {

    payload_file_t *payload = NULL;

    payload = payload_lookup(id);
    if (payload) {
        /* Remove it from the list */
        payload_file_t *iter = g_payload_list;
        payload_file_t *prev = NULL;

        while (iter) {
            if (iter == payload) {
                /* Matched it again */
                if (prev) {
                    prev->next = iter->next;
                } else {
                    g_payload_list = iter->next;
                }
                if (iter->file) {
                    fclose(iter->file);
                }
                if (iter->filename) {
                    free(iter->filename);
                }
                free(iter);
                break;
            }

            prev = iter;
            iter = iter->next;
        }
    } else {
        printf("ERROR: Payload id: %d does not exist\n", id);
    }
}

bool dtp_file_payload_get_meta(dtp_payload_meta_t *meta, uint8_t payload_id) {
    bool result = true;

    payload_file_t *payload = payload_lookup(payload_id);
    if (payload) {
        meta->size = payload->size;
        meta->read = payload->read;
    } else {
        meta->size = 0;
        meta->read = NULL;
        result = false;
    }

    return result;
}

void dtp_file_payload_info() {
    printf(" ID |         SIZE | FILENAME\n");
    printf("----+--------------+-----------------------------------------\n");

    /* Parse the list of payloads */
    payload_file_t *iter;
    for (iter = g_payload_list; iter != NULL; iter = iter->next) {
        printf("%3d | %12ld | %s\n", iter->id, iter->size, iter->filename);
    }
}
