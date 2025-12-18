#pragma once

#include <dtp/dtp.h>
#include <dtp/platform.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct payload_file_s payload_file_t;
/**
 * @brief Register a file as an DTP available payload
 * @param id to associate the gien file with
 * @param filename to associate with the given id
 * @return pointer to payload_file_t object or NULL if file couldn't be registered (file doesn't exist, no more memory, etc)
 */
extern payload_file_t *dtp_file_payload_add(uint8_t id, const char *filename);

/**
 * @brief Remove file associated with id from DTP available payloads
 * @param id of the file to remove
 */
extern void dtp_file_payload_del(uint8_t id);

/**
 * @brief Print info about registered file payload to stdout
 */
extern void dtp_file_payload_info(void);

/**
 * @brief Get info about a specific file payload
 * 
 * You should call this function from your own implementation of get_payload_meta()
 * 
 * @param meta[out] pointer to a valid object that will be filled in
 * @param payload_id payload identifier
 * @return true if payload is valid and meta data filled in, false otherwise
 */
bool dtp_file_payload_get_meta(dtp_payload_meta_t *meta, uint8_t payload_id);

#ifdef __cplusplus
}
#endif
