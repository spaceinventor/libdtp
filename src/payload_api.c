#include <cspftp/platform.h>

bool get_payload_meta(dftp_payload_meta_t *meta, uint8_t payload_id) {
    bool result = true;
    switch(payload_id) {
        case 0:
        /* deliberate fallthrough for now */
        default:
            /* 128 Mb for testing */
            meta->size = 128 * 1024 * 1024;
        break;
    }
    return result;
}