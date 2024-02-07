#pragma once

#include <stdint.h>
#include <vmem/vmem.h>

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
        CSPFTP_OK, //<! Chill, all good
        CSPFTP_ERR //<! Hmm, check errno...
    } cspftp_result;

    /**
     * Represent the error code for the most recent CSPFTP operation when the error situation
     * cannot be the return value of an API call (because async for example)
     */
    typedef enum
    {
        CSPFTP_NO_ERR, //<! Chill, all good
        CSPFTP_EINVAL, //<! Invalid argument
        CSPFTP_ENOMORE_SESSIONS, //<! All static sessions in use
        CSPFTP_ESESSION_NOT_FOUND, //<! Given session not found
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
        CSPFTP_REMOTE_INFO = 1 << 0,
        CSPFTP_OPT_2 = 1 << 1,
        CSPFTP_OPT_3 = 1 << 2,
        CSPFTP_OPT_4 = 1 << 3,
    } cspftp_option;

    /**
     * gettable/settable parameters for option 1
     */
    typedef struct {
        uint16_t remote;
    } cspftp_opt_remote_info;

    /**
     * gettable/settable parameters for option 2
     */
    typedef struct {
    } cspftp_opt_2;

    /**
     * gettable/settable parameters for option 3
     */
    typedef struct {
    } cspftp_opt_3;

    /**
     * gettable/settable parameters for option 4
     */
    typedef struct {
    } cspftp_opt_4;


    /**
     * All gettable/settable parameter types for a session
     */
    typedef union {
        cspftp_opt_remote_info remote_info;
        cspftp_opt_2 opt_2;
        cspftp_opt_3 opt_3;
        cspftp_opt_4 opt_4;
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
     * Serialize a CSPFTP session using the given destination VMEM object
     * 
     * A serialized CSPFTP session can be read back as a valid session using the counterpart cspftp_deserialize_session() function.
     * 
     * @param[in] session pointer to a valid session object
     * @param[in] dst to a vmem_t object the session will be serialized to
     * 
     * @return CSPFTP_OK in serialization completed, else CSPFTP_ERR, see csftp_errno()
     *
     */
    cspftp_result cspftp_serialize_session(cspftp_t *session, vmem_t *dst);

    /**
     * Deserialize a CSPFTP session using the given source VMEM
     * 
     * @param[in] session pointer to a fread-like function that will be called to read serialized session data
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

    /**
     * @brief 
     * @param session 
     * @param src 
     * @return 
     */
    cspftp_result cspftp_set_source(cspftp_t *session, vmem_t *src);

    /**
     * @brief 
     * @param session 
     * @param dst 
     * @return 
     */
    cspftp_result cspftp_set_destination(cspftp_t *session, vmem_t *dst);

/*
#pragma endregion
 */

/*
#pragma region Simplified Public API - TBD
 */
/*
#pragma endregion
 */

#ifdef __cplusplus
}
#endif
