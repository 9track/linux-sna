#ifndef _anameapi_h
#define _anameapi_h

#define ANAME_ENTRY extern void
#define ANAME_PTR   *

/**********************************************************************
 * ANAME Maximum Field Sizes
 **********************************************************************/

#define ANAME_UN_SIZE       (64)     /* user name                     */
#define ANAME_FQ_SIZE       (17)     /* fully qualified LU name       */
#define ANAME_GN_SIZE       (64)     /* group name                    */
#define ANAME_TP_SIZE       (64)     /* transaction program name      */
#define ANAME_DEST_SIZE     (17)     /* destination name              */
#define ANAME_HND_SIZE       (8)     /* handle                        */

/**********************************************************************
 * ANAME Type Definitions
 **********************************************************************/

typedef unsigned long       ANAME_LENGTH_TYPE;

typedef unsigned char       ANAME_HANDLE_TYPE[ ANAME_HND_SIZE ];

typedef unsigned long       ANAME_RETURN_CODE_TYPE;

typedef unsigned long       ANAME_DETAIL_LEVEL_TYPE;

typedef unsigned long       ANAME_TRACE_LEVEL_TYPE;

typedef unsigned long       ANAME_DATA_RECEIVED_TYPE;

typedef unsigned long       ANAME_DUP_FLAG_TYPE;

/**********************************************************************
 *  ANAME Constant Definitions
 **********************************************************************/

/*  Return Codes */

#define ANAME_RC_OK                      ((ANAME_RETURN_CODE_TYPE)0)
#define ANAME_RC_COMM_FAIL_NO_RETRY      ((ANAME_RETURN_CODE_TYPE)1)
#define ANAME_RC_COMM_FAIL_RETRY         ((ANAME_RETURN_CODE_TYPE)2)
#define ANAME_RC_COMM_CONFIG_LOCAL       ((ANAME_RETURN_CODE_TYPE)3)
#define ANAME_RC_COMM_CONFIG_REMOTE      ((ANAME_RETURN_CODE_TYPE)4)
#define ANAME_RC_SECURITY_NOT_VALID      ((ANAME_RETURN_CODE_TYPE)5)
#define ANAME_RC_FAIL_INPUT_ERROR        ((ANAME_RETURN_CODE_TYPE)6)
#define ANAME_RC_FAIL_RETRY              ((ANAME_RETURN_CODE_TYPE)7)
#define ANAME_RC_FAIL_NO_RETRY           ((ANAME_RETURN_CODE_TYPE)8)
#define ANAME_RC_FAIL_FATAL              ((ANAME_RETURN_CODE_TYPE)9)
#define ANAME_RC_PROGRAM_INTERNAL_ERROR  ((ANAME_RETURN_CODE_TYPE)10)
#define ANAME_RC_PARAMETER_CHECK         ((ANAME_RETURN_CODE_TYPE)11)
#define ANAME_RC_HANDLE_NOT_VALID        ((ANAME_RETURN_CODE_TYPE)12)
#define ANAME_RC_STATE_CHECK             ((ANAME_RETURN_CODE_TYPE)13)
#define ANAME_RC_BUFFER_TOO_SMALL        ((ANAME_RETURN_CODE_TYPE)14)
#define ANAME_RC_RECORD_ALREADY_OWNED    ((ANAME_RETURN_CODE_TYPE)100)

/* Data Received Values */

#define  ANAME_DR_DATA_RECEIVED_OK       (ANAME_DATA_RECEIVED_TYPE)(0)
#define  ANAME_DR_NO_MORE_DATA           (ANAME_DATA_RECEIVED_TYPE)(1)

/* Duplicate Flag Values */

#define  ANAME_DISALLOW_DUPLICATES       (ANAME_DUP_FLAG_TYPE)(0)
#define  ANAME_ALLOW_DUPLICATES          (ANAME_DUP_FLAG_TYPE)(1)

/* Format Error Detail Levels */

#define ANAME_DETAIL_RC                  (ANAME_DETAIL_LEVEL_TYPE)(1)
#define ANAME_DETAIL_SECOND              (ANAME_DETAIL_LEVEL_TYPE)(2)
#define ANAME_DETAIL_LOG                 (ANAME_DETAIL_LEVEL_TYPE)(4)
#define ANAME_DETAIL_INFO                (ANAME_DETAIL_LEVEL_TYPE)(8)
#define ANAME_DETAIL_ALL                 (ANAME_DETAIL_LEVEL_TYPE)(15)

/* Trace Levels */

#define ANAME_LVL_NO_TRACING            ((ANAME_TRACE_LEVEL_TYPE)   (0))
#define ANAME_LVL_FAILURES              ((ANAME_TRACE_LEVEL_TYPE)  (10))
#define ANAME_LVL_API                   ((ANAME_TRACE_LEVEL_TYPE)  (20))
#define ANAME_LVL_MODULE                ((ANAME_TRACE_LEVEL_TYPE)  (30))
#define ANAME_LVL_FUNCTION              ((ANAME_TRACE_LEVEL_TYPE)  (40))
#define ANAME_LVL_IO_OPEN_CLOSE         ((ANAME_TRACE_LEVEL_TYPE)  (50))
#define ANAME_LVL_LINE_FLOW             ((ANAME_TRACE_LEVEL_TYPE) (100))
#define ANAME_LVL_IO_READ_WRITE         ((ANAME_TRACE_LEVEL_TYPE) (110))
#define ANAME_LVL_STATUS_DUMP           ((ANAME_TRACE_LEVEL_TYPE) (120))
#define ANAME_LVL_VARIABLE              ((ANAME_TRACE_LEVEL_TYPE) (130))
#define ANAME_LVL_LOCATION              ((ANAME_TRACE_LEVEL_TYPE) (140))
#define ANAME_LVL_DATA_TRACE            ((ANAME_TRACE_LEVEL_TYPE) (170))
#define ANAME_LVL_LOOPLOC               ((ANAME_TRACE_LEVEL_TYPE) (200))

#define ANAME_MAX_TRACE_LVL              ANAME_LVL_LOOPLOC

/**********************************************************************
 *  ANAME Long to Short Function Name Mapping
 **********************************************************************/

#define aname_create                 ANCRT
#define aname_delete                 ANDEL
#define aname_destroy                ANDEST
#define aname_extract_fqlu_name      ANEFQ
#define aname_extract_group_name     ANEGN
#define aname_extract_tp_name        ANETP
#define aname_extract_user_name      ANEUN
#define aname_query                  ANQRY
#define aname_receive                ANRCV
#define aname_register               ANREG
#define aname_set_duplicate_register ANSDR
#define aname_set_fqlu_name          ANSFQ
#define aname_set_group_name         ANSGN
#define aname_set_destination        ANSDEST
#define aname_set_tp_name            ANSTP
#define aname_set_trace_filename     ANSTF
#define aname_set_trace_level        ANSTL
#define aname_set_user_name          ANSUN
#define aname_format_error           ANFERR

#define ancrt                        ANCRT
#define andel                        ANDEL
#define andest                       ANDEST
#define anefq                        ANEFQ
#define anegn                        ANEGN
#define anetp                        ANETP
#define aneun                        ANEUN
#define anqry                        ANQRY
#define anrcv                        ANRCV
#define anreg                        ANREG
#define ansdr                        ANSDR
#define anferr                       ANFERR
#define ansfq                        ANSFQ
#define ansgn                        ANSGN
#define ansdest                      ANSDEST
#define anstf                        ANSTF
#define anstl                        ANSTL
#define anstp                        ANSTP
#define ansun                        ANSUN

/**********************************************************************
 *  ANAME Function Prototypes
 **********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

ANAME_ENTRY
aname_create(
    unsigned char ANAME_PTR              handle,
    ANAME_RETURN_CODE_TYPE ANAME_PTR     return_code);

ANAME_ENTRY
aname_delete(
    unsigned char ANAME_PTR              handle,
    ANAME_RETURN_CODE_TYPE ANAME_PTR     return_code);

ANAME_ENTRY
aname_destroy(
    unsigned char ANAME_PTR              handle,
    ANAME_RETURN_CODE_TYPE ANAME_PTR     return_code);

ANAME_ENTRY
aname_extract_fqlu_name(
    unsigned char ANAME_PTR              handle,
    unsigned char ANAME_PTR              fqlu_name,
    ANAME_LENGTH_TYPE                    buffer_size,
    ANAME_LENGTH_TYPE ANAME_PTR          fqlu_name_length,
    ANAME_RETURN_CODE_TYPE ANAME_PTR     return_code);

ANAME_ENTRY
aname_extract_group_name(
    unsigned char ANAME_PTR              handle,
    unsigned char ANAME_PTR              group_name,
    ANAME_LENGTH_TYPE                    buffer_size,
    ANAME_LENGTH_TYPE ANAME_PTR          group_name_length,
    ANAME_RETURN_CODE_TYPE ANAME_PTR     return_code);

ANAME_ENTRY
aname_extract_tp_name(
    unsigned char ANAME_PTR              handle,
    unsigned char ANAME_PTR              tp_name,
    ANAME_LENGTH_TYPE                    buffer_size,
    ANAME_LENGTH_TYPE ANAME_PTR          tp_name_length,
    ANAME_RETURN_CODE_TYPE ANAME_PTR     return_code);

ANAME_ENTRY
aname_extract_user_name(
    unsigned char ANAME_PTR              handle,
    unsigned char ANAME_PTR              user_name,
    ANAME_LENGTH_TYPE                    buffer_size,
    ANAME_LENGTH_TYPE ANAME_PTR          user_name_length,
    ANAME_RETURN_CODE_TYPE ANAME_PTR     return_code);

ANAME_ENTRY
aname_query(
    unsigned char ANAME_PTR              handle,
    ANAME_RETURN_CODE_TYPE ANAME_PTR     return_code);

ANAME_ENTRY
aname_receive(
    unsigned char ANAME_PTR              handle,
    ANAME_DATA_RECEIVED_TYPE ANAME_PTR   pdata_received,
    ANAME_RETURN_CODE_TYPE ANAME_PTR     return_code);

ANAME_ENTRY
aname_register(
    unsigned char ANAME_PTR              handle,
    ANAME_RETURN_CODE_TYPE ANAME_PTR     return_code);

ANAME_ENTRY
aname_set_duplicate_register(
    unsigned char ANAME_PTR              handle,
    ANAME_DUP_FLAG_TYPE                  dup_flag,
    ANAME_RETURN_CODE_TYPE ANAME_PTR     return_code);

ANAME_ENTRY
aname_set_fqlu_name(
    unsigned char ANAME_PTR              handle,
    unsigned char ANAME_PTR              name,
    ANAME_LENGTH_TYPE                    name_length,
    ANAME_RETURN_CODE_TYPE ANAME_PTR     return_code);

ANAME_ENTRY
aname_set_group_name(
    unsigned char ANAME_PTR              handle,
    unsigned char ANAME_PTR              group,
    ANAME_LENGTH_TYPE                    group_length,
    ANAME_RETURN_CODE_TYPE ANAME_PTR     return_code);

ANAME_ENTRY
aname_set_destination(
    unsigned char ANAME_PTR              handle,
    unsigned char ANAME_PTR              destination,
    ANAME_LENGTH_TYPE                    destination_length,
    ANAME_RETURN_CODE_TYPE ANAME_PTR     return_code);

ANAME_ENTRY
aname_set_tp_name(
    unsigned char ANAME_PTR              handle,
    unsigned char ANAME_PTR              tp_name,
    ANAME_LENGTH_TYPE                    tp_name_length,
    ANAME_RETURN_CODE_TYPE ANAME_PTR     return_code);

ANAME_ENTRY
aname_set_trace_filename(
    unsigned char ANAME_PTR              trace_filename,
    ANAME_RETURN_CODE_TYPE ANAME_PTR     return_code);

ANAME_ENTRY
aname_set_trace_level(
    ANAME_TRACE_LEVEL_TYPE               ns_trace_level,
    ANAME_RETURN_CODE_TYPE ANAME_PTR     return_code);

ANAME_ENTRY
aname_set_user_name(
    unsigned char ANAME_PTR              handle,
    unsigned char ANAME_PTR              alias,
    ANAME_LENGTH_TYPE                    alias_length,
    ANAME_RETURN_CODE_TYPE ANAME_PTR     return_code);

ANAME_ENTRY
aname_format_error(
    unsigned char ANAME_PTR              handle,
    ANAME_DETAIL_LEVEL_TYPE              detail_level,
    unsigned char ANAME_PTR              error_str,
    ANAME_LENGTH_TYPE                    error_str_size,
    ANAME_LENGTH_TYPE ANAME_PTR          returned_length,
    ANAME_RETURN_CODE_TYPE ANAME_PTR     return_code);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
