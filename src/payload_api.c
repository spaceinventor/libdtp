#include <string.h>
#include <dtp/platform.h>

static uint32_t payload_read(uint8_t payload_id, uint32_t offset, void *output, uint32_t size) {
    uint32_t res = 0;
    switch(payload_id) {
        case 0:
        default:
            memset(output, 0x30 + payload_id, size);
            res = size;
        break;
    }
    return res;
}

__attribute__((weak)) bool get_payload_meta(dftp_payload_meta_t *meta, uint8_t payload_id) {
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