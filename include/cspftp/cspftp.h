#pragma once

#include <stdint.h>
#include <vmem/vmem.h>
#include <csp/csp.h>

#ifdef __cplusplus
extern "C"
{
#endif
/**
 * CSP FTP Public Interface.
 */

    /** Opaque handle to a CSPFTP sesssion */
    typedef struct cspftp_t cspftp_t;

    /** Returned by most of CSPFTP API calls*/
    typedef enum
    {
        CSPFTP_OK, //!< Chill, all good
        CSPFTP_ERR //!< Hmm, check errno...
    } cspftp_result;

    /**
     * Represent the error code for the most recent CSPFTP operation when the error situation
     * cannot be the return value of an API call (because async for example)
     */
    typedef enum
    {
        CSPFTP_NO_ERR, //!< Chill, all good
        CSPFTP_EINVAL, //!< Invalid argument
        CSPFTP_ENOMORE_SESSIONS, //!< All static sessions in use
        CSPFTP_ESESSION_NOT_FOUND, //!< Given session not found
        CSPFTP_CONNECTION_FAILED, //!< Connecting to the server failed, for some rease
        CSPFTP_MALLOC_FAILED, //!< Could not allocate a mandatory piece of memory
        CSPFTP_NOT_IMPLEMENTED, //!< Still under construction, sorry
        CSPFTP_LAST_ERR
    } cspftp_errno_t;

    /** The current state of a CSPFTP transaction */
    typedef enum {
        NEW,
        IN_PROGRESS,
        PAUSED,
        DONE
    } cspftp_state;

    /** The options pertaining to a CSPFTP session */
    typedef enum {
        CSPFTP_REMOTE_CFG = 1 << 0,
        CSPFTP_SESSION_HOOKS_CFG = 1 << 1,
        CSPFTP_OPT_3 = 1 << 2,
        CSPFTP_OPT_4 = 1 << 3,
    } cspftp_option;

    /**
     * configuration - DTP server CSP node address
     */
    typedef struct {
        uint16_t node;
    } cspftp_opt_remote_cfg;

    /**
     * configuration - DTP session hooks
     */    
    typedef struct {
        /** 
         * Called when session is about to start
         * @param session the current session
         */
        void (*on_start)(cspftp_t *session);
        /** 
         * Called whenever a data packet is received
         * @param session the current session
         * @param packet the freshly received data packet
         * 
         * @return false -> interrupt the transfer, true -> carry on
         */
        bool (*on_data_packet)(cspftp_t *session, csp_packet_t *packet);
        /** 
         * Called when session is done (for whatever reason, see cspftp_errno())
         * @param session the current session
         */        
        void (*on_end)(cspftp_t *session);

        /** 
         * Called when session is being released (about to be destroyed), allows custom clean up
         * @param session the current session
         */        
        void (*on_release)(cspftp_t *session);

        /** 
         * Called when session is serialized, allows complete customization of serialization.
         * @param session the current session
         * @param output destination for serialization (might be a vmem * IF that's enough?)
         */        
        void (*on_serialize)(cspftp_t *session, vmem_t *output);

        /** 
         * Called when session is deserialized, allows complete customization of de-serialization.
         * @param session the current session
         * @param input source for deserialization (might be a vmem * IF that's enough?)
         */        
        void (*on_deserialize)(cspftp_t *session, vmem_t *input);

        void *hook_ctx;
    } cspftp_opt_session_hooks_cfg;

    
    /** Default session hooks, define this variable if you want your own implementation */
    extern cspftp_opt_session_hooks_cfg default_session_hooks;

    extern const uint16_t CSPFTP_PACKET_SIZE;

    /**
     * All gettable/settable parameter types for a session
     */
    typedef union {
        cspftp_opt_remote_cfg remote_cfg;
        cspftp_opt_session_hooks_cfg hooks;
    } cspftp_params;

/*
#pragma region Public API
*/

    /**
     * Try to get a handle of one of the pre-allocated sessions
     * @return valid pointer to session object or NULL in case of failure, see csftp_errno()
     */
    cspftp_t *cspftp_acquire_session();

    /**
     * Release a previously acquired handle on one of the pre-allocated sessions
     * 
     * @param[in] pointer to a session object previously obtained using cspftp_acquire_session()
     * 
     * @return CSPFTP_OK if release ok, see csftp_errno()
     */
    cspftp_result cspftp_release_session(cspftp_t *session);

    /**
     * @brief Serialize a CSPFTP session using the given destination VMEM object
     * 
     * A serialized CSPFTP session can be read back as a valid session using the counterpart cspftp_deserialize_session() function.
     * 
     * @param[in] session pointer to a valid session object
     * @param[in] dst pointer to a vmem_t object the session will be serialized to
     * 
     * @return CSPFTP_OK in serialization completed, else CSPFTP_ERR, see csftp_errno()
     *
     */
    cspftp_result cspftp_serialize_session(cspftp_t *session, vmem_t *dst);

    /**
     * Deserialize a CSPFTP session using the given source VMEM
     * 
     * @param[in] session pointer to a valid session object
     * @param[in] src pointer to a vmem_t object the session will be deserialized from
     * 
     * @return CSPFTP_OK in serialization completed, else CSPFTP_ERR, see csftp_errno(). If sucessful, the object will contained data as it was when the session was serialized.
     *
     */
    cspftp_result cspftp_deserialize_session(cspftp_t *session, vmem_t *src);

    /**
     * Get the errno value for the given session
     * @param[in] session pointer to a valid cspftp session object
     * 
     * @return errno value of the last operation on the given session
     */
    cspftp_errno_t cspftp_errno(cspftp_t *session);

    /**
     * @return printable string describing the error represented by the given cspftp_errno value
     */
    const char* cspftp_strerror(cspftp_errno_t err);

    /**
     * Set a session option.
     * 
     * @param[in] session pointer to a valid cspftp session object
     * 
     * @return CSPFTP_OK
     */
    cspftp_result cspftp_set_opt(cspftp_t *session, cspftp_option option, cspftp_params *param);

    /**
     * Get a session option.

     * @param[in] session pointer to a valid cspftp session object

     * @return CSPFTP_OK
     */
    cspftp_result cspftp_get_opt(cspftp_t *session, cspftp_option option, cspftp_params *param);

    /**
     * Start (or resume) a CSP FTP session
     * 
     * @param[in] session pointer to a valid cspftp session object

     * @note a succesfully started CSP FTP transfer can be interrupted at any point, see cspftp_stop_transfer()
     * 
     * @return CSPFTP_OK if session was started sucessfully, or CSPFTP_ERR in case of failure, see csftp_errno()
     */
    cspftp_result cspftp_start_transfer(cspftp_t *session);

    /**
     * Interrupt a CSP FTP session.
     * 
     * @param[in] session pointer to a valid cspftp session object

     * @note a succesfully interrupted CSP FTP transfer can be resumed later, see cspftp_start_transfer()
     * 
     * @return CSPFTP_OK if session was stopped sucessfully, or CSPFTP_ERR in case of failure, see csftp_errno()
     */
    cspftp_result cspftp_stop_transfer(cspftp_t *session);

/*
#pragma endregion
 */

/*
#pragma region Simplified Public API
 */
    extern int dtp_server_main(bool *keep_running);
    extern int dtp_client_main(uint32_t server, cspftp_t **session);

/*
#pragma endregion
 */

#ifdef __cplusplus
}
#endif
