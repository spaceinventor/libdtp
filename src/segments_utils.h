#pragma once
#include <stdbool.h>
#include "cspftp/cspftp.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Maintain a (linked) list of received segments
 * @param received the data packet id (it's in the first uint32_t in the payload)
 * @return true if all good, false if we could not allocate a segment, in which case it's a good idea to cancel the transaction
 */
extern bool update_segments(uint32_t received);

/**
 * Call this at the end of a transfer to close the currently open segment if any
 * @param received 
 */
extern void close_segments(uint32_t expected);

/**
 * Get the number of received segments
 * @return number of received segments
 */
extern uint32_t get_nof_segments();

/**
 * Dumps current segments to stdout
 */
extern void print_segments();

#ifdef __cplusplus
}
#endif