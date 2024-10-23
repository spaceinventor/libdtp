#include <string.h>
#include <dtp/platform.h>

static const char test_data[] = "0123456789ABCDE";
static uint32_t payload_read(uint8_t payload_id, uint32_t offset, void *output, uint32_t size) {
    uint32_t res = 0;
    switch(payload_id) {
        case 0:
        default: {
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
        }
        break;
    }
    return res;
}

__attribute__((weak)) bool get_payload_meta(dtp_payload_meta_t *meta, uint8_t payload_id) {
    bool result = true;
    switch(payload_id) {
        case 0:
        /* deliberate fallthrough for now */
        default:
            /* 128 Mb for testing */
            meta->size = 20 * 1024 * 1024;
            meta->read = payload_read;
        break;
    }
    return result;
}