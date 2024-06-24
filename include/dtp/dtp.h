#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif
/**
 * DTP Public Interface.
 */

    /* CSP connection  type forward declaration*/
    typedef struct csp_packet_s csp_packet_t;
     /** VMEM type forward declaration */
    typedef struct vmem_s vmem_t;
    /** Opaque handle to a DTP sesssion */
    typedef struct dtp_t dtp_t;

    /** Returned by most of DTP API calls*/
    typedef enum
    {
        DTP_OK, //!< Chill, all good
        DTP_ERR //!< Hmm, check errno...
    } dtp_result;

    /**
     * Represent the error code for the most recent DTP operation when the error situation
     * cannot be the return value of an API call (because async for example)
     */
    typedef enum
    {
        DTP_NO_ERR, //!< Chill, all good
        DTP_EINVAL, //!< Invalid argument
        DTP_ENOMORE_SESSIONS, //!< All static sessions in use
        DTP_ESESSION_NOT_FOUND, //!< Given session not found
        DTP_CONNECTION_FAILED, //!< Connecting to the server failed, for some rease
        DTP_MALLOC_FAILED, //!< Could not allocate a mandatory piece of memory
        DTP_NOT_IMPLEMENTED, //!< Still under construction, sorry
        DTP_LAST_ERR
    } dtp_errno_t;

    /** The current state of a DTP transaction */
    typedef enum {
        NEW,
        IN_PROGRESS,
        PAUSED,
        DONE
    } dtp_state;

    /** The options pertaining to a DTP session */
    typedef enum {
        DTP_REMOTE_CFG = 1 << 0,
        DTP_SESSION_HOOKS_CFG = 1 << 1,
        DTP_THROUGHPUT_CFG = 1 << 2,
        DTP_TIMEOUT_CFG = 1 << 3,
        DTP_PAYLOAD_ID_CFG = 1 << 4,
        DTP_MTU_CFG = 1 << 5,
    } dtp_option;

    /**
     * configuration - DTP server CSP node address
     */
    typedef struct {
        uint16_t node;
    } dtp_opt_remote_cfg;

    typedef struct {
        uint32_t value; /// MAX Throughout in KB/s
    } dtp_opt_throughput_cfg;

    typedef struct {
        uint8_t value; /// Idle timeout that will stop the session
    } dtp_opt_timeout_cfg;

    typedef struct {
        uint8_t value; /// Payload ID to retrieve
    } dtp_opt_payload_id_cfg;

    typedef struct {
        uint16_t value; /// MTU size to use
    } dtp_opt_mtu_cfg;

    /**
     * configuration - DTP session hooks
     */    
    typedef struct {
        /** 
         * Called when session is about to start
         * @param session the current session
         */
        void (*on_start)(dtp_t *session);
        /** 
         * Called whenever a data packet is received
         * @param session the current session
         * @param packet the freshly received data packet
         * 
         * @return false -> interrupt the transfer, true -> carry on
         */
        bool (*on_data_packet)(dtp_t *session, csp_packet_t *packet);
        /** 
         * Called when session is done (for whatever reason, see dtp_errno())
         * @param session the current session
         */        
        void (*on_end)(dtp_t *session);

        /** 
         * Called when session is being released (about to be destroyed), allows custom clean up
         * @param session the current session
         */        
        void (*on_release)(dtp_t *session);

        /** 
         * Called when session is serialized, allows complete customization of serialization.
         * @param session the current session
         * @param output destination for serialization (might be a vmem * IF that's enough?)
         */        
        void (*on_serialize)(dtp_t *session, vmem_t *output);

        /** 
         * Called when session is deserialized, allows complete customization of de-serialization.
         * @param session the current session
         * @param input source for deserialization (might be a vmem * IF that's enough?)
         */        
        void (*on_deserialize)(dtp_t *session, vmem_t *input);

        void *hook_ctx;
    } dtp_opt_session_hooks_cfg;

    
    /** Default session hooks, define this variable if you want your own implementation */
    extern dtp_opt_session_hooks_cfg default_session_hooks;
    extern const uint32_t DTP_SESSION_VERSION;

    /**
     * All gettable/settable parameter types for a session
     */
    typedef union {
        dtp_opt_remote_cfg remote_cfg;
        dtp_opt_session_hooks_cfg hooks;
        dtp_opt_throughput_cfg throughput;
        dtp_opt_timeout_cfg timeout;
        dtp_opt_payload_id_cfg payload_id;
        dtp_opt_mtu_cfg mtu;
    } dtp_params;

/*
#pragma region Public API
*/

    /**
     * Try to get a handle of one of the pre-allocated sessions
     * @return valid pointer to session object or NULL in case of failure, see csftp_errno()
     */
    dtp_t *dtp_acquire_session();

    /**
     * Release a previously acquired handle on one of the pre-allocated sessions
     * 
     * @param[in] pointer to a session object previously obtained using dtp_acquire_session()
     * 
     * @return DTP_OK if release ok, see csftp_errno()
     */
    dtp_result dtp_release_session(dtp_t *session);

    /**
     * @brief Serialize a DTP session using the given destination VMEM object
     * 
     * A serialized DTP session can be read back as a valid session using the counterpart dtp_deserialize_session() function.
     * 
     * @param[in] session pointer to a valid session object
     * @param[in] dst pointer to a vmem_t object the session will be serialized to
     * 
     * @return DTP_OK in serialization completed, else DTP_ERR, see csftp_errno()
     *
     */
    dtp_result dtp_serialize_session(dtp_t *session, vmem_t *dst);

    /**
     * Deserialize a DTP session using the given source VMEM
     * 
     * @param[in] session pointer to a valid session object
     * @param[in] src pointer to a vmem_t object the session will be deserialized from
     * 
     * @return DTP_OK in serialization completed, else DTP_ERR, see csftp_errno(). If sucessful, the object will contained data as it was when the session was serialized.
     *
     */
    dtp_result dtp_deserialize_session(dtp_t *session, vmem_t *src);

    /**
     * @brief Save a pointer to user data in a DTP session object
     * @param session pointer to a valid DTP session object
     * @param user_context pointer to user data
     * @note this is by default set to NULL
     */
    void dtp_session_set_user_ctx(dtp_t *session, void *user_context);

    /**
     * @brief Retrieve a DTP session user context previously saved using dtp_deserialize_session()
     * @param session pointer to a valid DTP session object
     * @return pointer to the user context
     */
    void *dtp_session_get_user_ctx(dtp_t *session);

    /**
     * Get the errno value for the given session
     * @param[in] session pointer to a valid dtp session object
     * 
     * @return errno value of the last operation on the given session
     */
    dtp_errno_t dtp_errno(dtp_t *session);

    /**
     * @return printable string describing the error represented by the given dtp_errno value
     */
    const char* dtp_strerror(dtp_errno_t err);

    /**
     * Set a session option.
     * 
     * @param[in] session pointer to a valid dtp session object
     * 
     * @return DTP_OK
     */
    dtp_result dtp_set_opt(dtp_t *session, dtp_option option, dtp_params *param);

    /**
     * Get a session option.

     * @param[in] session pointer to a valid dtp session object

     * @return DTP_OK
     */
    dtp_result dtp_get_opt(dtp_t *session, dtp_option option, dtp_params *param);

    /**
     * Start (or resume) a DTP session
     * 
     * @param[in] session pointer to a valid dtp session object

     * @note a succesfully started DTP transfer can be interrupted at any point, see dtp_stop_transfer()
     * 
     * @return DTP_OK if session was started sucessfully, or DTP_ERR in case of failure, see csftp_errno()
     */
    dtp_result dtp_start_transfer(dtp_t *session);

    /**
     * Interrupt a DTP session.
     * 
     * @param[in] session pointer to a valid dtp session object

     * @note a succesfully interrupted DTP transfer can be resumed later, see dtp_start_transfer()
     * 
     * @return DTP_OK if session was stopped sucessfully, or DTP_ERR in case of failure, see csftp_errno()
     */
    dtp_result dtp_stop_transfer(dtp_t *session);

/*
#pragma endregion
 */

/*
#pragma region Simplified Public API
 */
    extern int dtp_server_main(bool *keep_running);
    extern int dtp_client_main(uint32_t server, uint16_t max_throughput, uint8_t timeout, uint8_t payload_id, uint16_t mtu, bool resume, dtp_t **session);

/*
#pragma endregion
 */

#ifdef __cplusplus
}
#endif
