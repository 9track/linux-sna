/* cpic.h: CPI Communications Header.
 *
 * Author:
 * Jay Schulist         <jschlst@samba.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * None of the authors or maintainers or their employers admit
 * liability nor provide warranty for any of this software.
 * This material is provided "as is" and at no charge.
 */

#ifndef _CPIC_H
#define _CPIC_H

#define CM_INT32 signed long int
#define CM_ENTRY extern void
#define CM_PTR *

typedef unsigned char CONVERSATION_ID [8];
typedef unsigned char CONTEXT_ID [32];
typedef unsigned char SECURITY_PASSWORD [10];
typedef unsigned char SECURITY_USER_ID [10];

typedef CM_INT32 CM_AE_QUAL_OR_AP_TITLE_FORMAT;
typedef CM_INT32 CM_ALLOCATE_CONFIRM_TYPE;
typedef CM_INT32 CM_BEGIN_TRANSACTION;
typedef CM_INT32 CM_BUFFER_LENGTH;
typedef CM_INT32 CM_CALL_ID;
typedef CM_INT32 CM_COMPLETED_OP_COUNT;
typedef CM_INT32 CM_CONFIRMATION_URGENCY;
typedef CM_INT32 CM_CONTEXT_ID_LENGTH;
typedef CM_INT32 CM_CONTROL_INFORMATION_RECEIVED;
typedef CM_INT32 CM_CONVERSATION_QUEUE;
typedef CM_INT32 CM_CONVERSATION_RETURN_CODE;
typedef CM_INT32 CM_CONVERSATION_SECURITY_TYPE;
typedef CM_INT32 CM_SECURITY_PASSWORD_LENGTH;
typedef CM_INT32 CM_CONVERSATION_SECURITY_LENGTH;
typedef CM_INT32 CM_SECURITY_USER_ID_LENGTH;
typedef CM_INT32 CM_CONVERSATION_STATE;
typedef CM_INT32 CM_CONVERSATION_TYPE;
typedef CM_INT32 CM_DATA_RECEIVED_TYPE;
typedef CM_INT32 CM_DEALLOCATE_TYPE;
typedef CM_INT32 CM_DIRECTORY_ENCODING;
typedef CM_INT32 CM_DIRECTORY_SYNTAX;
typedef CM_INT32 CM_ERROR_DIRECTION;
typedef CM_INT32 CM_FILL;
typedef CM_INT32 CM_MAXIMUM_BUFFER_SIZE;
typedef CM_INT32 CM_OOID;
typedef CM_INT32 CM_PARTNER_ID_SCOPE;
typedef CM_INT32 CM_PARTNER_ID_TYPE;
typedef CM_INT32 CM_PREPARE_DATA_PERMITTED_TYPE;
typedef CM_INT32 CM_PREPARE_TO_RECEIVE_TYPE;
typedef CM_INT32 CM_PROCESSING_MODE;
typedef CM_INT32 CM_RECEIVE_TYPE;
typedef CM_CONTROL_INFORMATION_RECEIVED CM_REQUEST_TO_SEND_RECEIVED;
typedef CM_INT32 CM_RETURN_CODE;
typedef CM_INT32 CM_RETURN_CONTROL;
typedef CM_INT32 CM_SEND_RECEIVE_MODE;
typedef CM_INT32 CM_SEND_TYPE;
typedef CM_INT32 CM_STATUS_RECEIVED;
typedef CM_INT32 CM_SYNC_LEVEL;
typedef CM_INT32 CM_TIMEOUT;
typedef CM_INT32 CM_TRANSACTION_CONTROL;
typedef CM_INT32 CONVERSATION_TYPE;
typedef CM_INT32 CONVERSATION_SECURITY_TYPE;
typedef CM_INT32 DATA_RECEIVED;
typedef CM_INT32 DEALLOCATE_TYPE;
typedef CM_INT32 ERROR_DIRECTION;
typedef CM_INT32 PREPARE_TO_RECEIVE_TYPE;
typedef CM_INT32 PROCESSING_MODE;
typedef CM_INT32 RECEIVE_TYPE;
typedef CM_INT32 REQUEST_TO_SEND_RECEIVED;
typedef CM_INT32 CM_RETCODE;
typedef CM_INT32 RETURN_CONTROL;
typedef CM_INT32 SEND_TYPE;
typedef CM_INT32 STATUS_RECEIVED;
typedef CM_INT32 SYNC_LEVEL;

#define CM_DN			(CM_AE_QUAL_OR_AP_TITLE_FORMAT) 0
#define CM_OID			(CM_AE_QUAL_OR_AP_TITLE_FORMAT) 1
#define CM_INT_DIGITS		(CM_AE_QUAL_OR_AP_TITLE_FORMAT) 2

/* Allocate_confirm values. */
#define CM_ALLOCATE_NO_CONFIRM	(CM_ALLOCATE_CONFIRM_TYPE) 0
#define CM_ALLOCATE_CONFIRM	(CM_ALLOCATE_CONFIRM_TYPE) 1

/* begin_transaction values. */
#define CM_BEGIN_IMPLICIT	(CM_BEGIN_TRANSACTION) 0
#define CM_BEGIN_EXPLICIT	(CM_BEGIN_TRANSACTION) 1

/* call_ID values. */
#define CM_CMACCI		(CM_CALL_ID) 1
#define CM_CMACCP		(CM_CALL_ID) 2
#define CM_CMALLC		(CM_CALL_ID) 3
#define CM_CMCANC		(CM_CALL_ID) 4
#define CM_CMCFM		(CM_CALL_ID) 5
#define CM_CMCFMD		(CM_CALL_ID) 6
#define CM_CMCNVI		(CM_CALL_ID) 7
#define CM_CMCNVO		(CM_CALL_ID) 8
#define CM_CMDEAL		(CM_CALL_ID) 9
#define CM_CMDFDE		(CM_CALL_ID) 10
#define CM_CMEACN		(CM_CALL_ID) 11
#define CM_CMEAEQ		(CM_CALL_ID) 12
#define CM_CMEAPT		(CM_CALL_ID) 13
#define CM_CMECS		(CM_CALL_ID) 14
#define CM_CMECT		(CM_CALL_ID) 15
#define CM_CMECTX		(CM_CALL_ID) 16
#define CM_CMEID		(CM_CALL_ID) 17
#define CM_CMEMBS		(CM_CALL_ID) 18
#define CM_CMEMN		(CM_CALL_ID) 19
#define CM_CMEPID		(CM_CALL_ID) 20
#define CM_CMEPLN		(CM_CALL_ID) 21
#define CM_CMESI		(CM_CALL_ID) 22
#define CM_CMESL		(CM_CALL_ID) 23
#define CM_CMESRM		(CM_CALL_ID) 24
#define CM_CMESUI		(CM_CALL_ID) 25
#define CM_CMETC		(CM_CALL_ID) 26
#define CM_CMETPN		(CM_CALL_ID) 27
#define CM_CMFLUS		(CM_CALL_ID) 28
#define CM_CMINCL		(CM_CALL_ID) 29
#define CM_CMINIC		(CM_CALL_ID) 30
#define CM_CMINIT		(CM_CALL_ID) 31
#define CM_CMPREP		(CM_CALL_ID) 32
#define CM_CMPTR		(CM_CALL_ID) 33
#define CM_CMRCV		(CM_CALL_ID) 34
#define CM_CMRCVX		(CM_CALL_ID) 35
#define CM_CMRLTP		(CM_CALL_ID) 36
#define CM_CMRTS		(CM_CALL_ID) 37
#define CM_CMSAC		(CM_CALL_ID) 38
#define CM_CMSACN		(CM_CALL_ID) 39
#define CM_CMSAEQ		(CM_CALL_ID) 40
#define CM_CMSAPT		(CM_CALL_ID) 41
#define CM_CMSBT		(CM_CALL_ID) 42
#define CM_CMSCSP		(CM_CALL_ID) 43
#define CM_CMSCST		(CM_CALL_ID) 44
#define CM_CMSCSU		(CM_CALL_ID) 45
#define CM_CMSCT		(CM_CALL_ID) 46
#define CM_CMSCU		(CM_CALL_ID) 47
#define CM_CMSDT		(CM_CALL_ID) 48
#define CM_CMSED		(CM_CALL_ID) 49
#define CM_CMSEND		(CM_CALL_ID) 50
#define CM_CMSERR		(CM_CALL_ID) 51
#define CM_CMSF			(CM_CALL_ID) 52
#define CM_CMSID		(CM_CALL_ID) 53
#define CM_CMSLD		(CM_CALL_ID) 54
#define CM_CMSLTP		(CM_CALL_ID) 55
#define CM_CMSMN		(CM_CALL_ID) 56
#define CM_CMSNDX		(CM_CALL_ID) 57
#define CM_CMSPDP		(CM_CALL_ID) 58
#define CM_CMSPID		(CM_CALL_ID) 59
#define CM_CMSPLN		(CM_CALL_ID) 60
#define CM_CMSPM		(CM_CALL_ID) 61
#define CM_CMSPTR		(CM_CALL_ID) 62
#define CM_CMSQCF		(CM_CALL_ID) 63
#define CM_CMSQPM		(CM_CALL_ID) 64
#define CM_CMSRC		(CM_CALL_ID) 65
#define CM_CMSRT		(CM_CALL_ID) 66
#define CM_CMSSL		(CM_CALL_ID) 67
#define CM_CMSSRM		(CM_CALL_ID) 68
#define CM_CMSST		(CM_CALL_ID) 69
#define CM_CMSTC		(CM_CALL_ID) 70
#define CM_CMSTPN		(CM_CALL_ID) 71
#define CM_CMTRTS		(CM_CALL_ID) 72
#define CM_CMWAIT		(CM_CALL_ID) 73
#define CM_CMWCMP		(CM_CALL_ID) 74

/* confirmation_urgency values. */
#define CM_CONFIRMATION_NOT_URGENT	(CM_CONFIRMATION_URGENCY) 0
#define CM_CONFIRMATION_URGENT		(CM_CONFIRMATION_URGENCY) 1

/* control_information_received, request_to_send_received_values. */
#define CM_NO_CONTROL_INFO_RECEIVED	(CM_CONTROL_INFORMATION_RECEIVED) 0
#define CM_REQ_TO_SEND_NOT_RECEIVED	(CM_CONTROL_INFORMATION_RECEIVED) 0
#define CM_REQ_TO_SEND_RECEIVED		(CM_CONTROL_INFORMATION_RECEIVED) 1
#define CM_ALLOCATE_CONFIRMED		(CM_CONTROL_INFORMATION_RECEIVED) 2
#define CM_ALLOCATE_CONFIRMED_WITH_DATA	(CM_CONTROL_INFORMATION_RECEIVED) 3
#define CM_ALLOCATE_REJECTED_WITH_DATA	(CM_CONTROL_INFORMATION_RECEIVED) 4
#define CM_EXPEDITED_DATA_AVAILABLE	(CM_CONTROL_INFORMATION_RECEIVED) 5
#define CM_RTS_RCVD_AND_EXP_DATA_AVAIL	(CM_CONTROL_INFORMATION_RECEIVED) 6

/* conversation_queue values. */
#define CM_INITIALIZATION_QUEUE		(CM_CONVERSATION_QUEUE) 0
#define CM_SEND_QUEUE			(CM_CONVERSATION_QUEUE) 1
#define CM_RECEIVE_QUEUE		(CM_CONVERSATION_QUEUE) 2
#define CM_SEND_RECEIVE_QUEUE		(CM_CONVERSATION_QUEUE) 3
#define CM_EXPEDITED_SEND_QUEUE		(CM_CONVERSATION_QUEUE) 4
#define CM_EXPEDITED_RECEIVE_QUEUE	(CM_CONVERSATION_QUEUE) 5

/* conversation_state values. */
#define CM_RESET_STATE			(CM_CONVERSATION_STATE) 1
#define CM_INITIALIZE_STATE		(CM_CONVERSATION_STATE) 2
#define CM_SEND_STATE			(CM_CONVERSATION_STATE) 3
#define CM_RECEIVE_STATE		(CM_CONVERSATION_STATE) 4
#define CM_SEND_PENDING_STATE		(CM_CONVERSATION_STATE) 5
#define CM_CONFIRM_STATE		(CM_CONVERSATION_STATE) 6
#define CM_CONFIRM_SEND_STATE		(CM_CONVERSATION_STATE) 7
#define CM_CONFIRM_DEALLOCATE_STATE	(CM_CONVERSATION_STATE) 8
#define CM_DEFER_RECEIVE_STATE		(CM_CONVERSATION_STATE) 9
#define CM_DEFER_DEALLOCATE_STATE	(CM_CONVERSATION_STATE) 10
#define CM_SYNC_POINT_STATE		(CM_CONVERSATION_STATE) 11
#define CM_SYNC_POINT_SEND_STATE	(CM_CONVERSATION_STATE) 12
#define CM_SYNC_POINT_DEALLOCATE_STATE	(CM_CONVERSATION_STATE) 13
#define CM_INITIALIZE_INCOMING_STATE	(CM_CONVERSATION_STATE) 14
#define CM_SEND_ONLY_STATE		(CM_CONVERSATION_STATE) 15
#define CM_RECEIVE_ONLY_STATE		(CM_CONVERSATION_STATE) 16
#define CM_SEND_RECEIVE_STATE		(CM_CONVERSATION_STATE) 17
#define CM_PREPARED_STATE		(CM_CONVERSATION_STATE) 18

/* conversation_type value. */
#define CM_BASIC_CONVERSATION		(CM_CONVERSATION_TYPE) 0
#define CM_MAPPED_CONVERSATION		(CM_CONVERSATION_TYPE) 1

/* data_received values. */
#define CM_NO_DATA_RECEIVED		(CM_DATA_RECEIVED_TYPE) 0
#define CM_DATA_RECEIVED		(CM_DATA_RECEIVED_TYPE) 1
#define CM_COMPLETE_DATA_RECEIVED	(CM_DATA_RECEIVED_TYPE) 2
#define CM_INCOMPLETE_DATA_RECEIVED	(CM_DATA_RECEIVED_TYPE) 3

/* deallocate_type values. */
#define CM_DEALLOCATE_SYNC_LEVEL	(CM_DEALLOCATE_TYPE) 0
#define CM_DEALLOCATE_FLUSH		(CM_DEALLOCATE_TYPE) 1
#define CM_DEALLOCATE_CONFIRM		(CM_DEALLOCATE_TYPE) 2
#define CM_DEALLOCATE_ABEND		(CM_DEALLOCATE_TYPE) 3

/* directory_encoding values. */
#define CM_DEFAULT_ENCODING		(CM_DIRECTORY_ENCODING) 0
#define CM_UNICODE_ENCODING		(CM_DIRECTORY_ENCODING) 1

/* directory_syntax values. */
#define CM_DEFAULT_SYNTAX		(CM_DIRECTORY_SYNTAX) 0
#define CM_DCE_SYNTAX			(CM_DIRECTORY_SYNTAX) 1
#define CM_XDS_SYNTAX			(CM_DIRECTORY_SYNTAX) 2
#define CM_NDS_SYNTAX			(CM_DIRECTORY_SYNTAX) 3

/* error_direction values. */
#define CM_RECEIVE_ERROR		(CM_ERROR_DIRECTION) 0
#define CM_SEND_ERROR			(CM_ERROR_DIRECTION) 1

/* fill values. */
#define CM_FILL_LL			(CM_FILL) 0
#define CM_FILL_BUFFER			(CM_FILL) 1

/* partner_ID_scope values. */
#define CM_EXPLICIT			(CM_PARTNER_ID_SCOPE) 0
#define CM_REFERENCE			(CM_PARTNER_ID_SCOPE) 1

/* partner_ID_type values. */
#define CM_DISTINGUISHED_NAME		(CM_PARTNER_ID_TYPE) 0
#define CM_LOCAL_DISTINGUISHED_NAME	(CM_PARTNER_ID_TYPE) 1
#define CM_PROGRAM_FUNCTION_ID		(CM_PARTNER_ID_TYPE) 2
#define CM_OSI_TPSU_TITLE_OID		(CM_PARTNER_ID_TYPE) 3
#define CM_PROGRAM_BINDING		(CM_PARTNER_ID_TYPE) 4

/* prepare_data_permitted values. */
#define CM_PREPARE_DATA_NOT_PERMITTED	(CM_PREPARE_DATA_PERMITTED_TYPE) 0
#define CM_PREPARE_DATA_PERMITTED	(CM_PREPARE_DATA_PERMITTED_TYPE) 1

/* prepare_to_receive_type values. */
#define CM_PREP_TO_RECEIVE_SYNC_LEVEL	(CM_PREPARE_TO_RECEIVE_TYPE) 0
#define CM_PREP_TO_RECEIVE_FLUSH	(CM_PREPARE_TO_RECEIVE_TYPE) 1
#define CM_PREP_TO_RECEIVE_CONFIRM	(CM_PREPARE_TO_RECEIVE_TYPE) 2

/* processing_mode values. */
#define CM_BLOCKING			(CM_PROCESSING_MODE) 0
#define CM_NON_BLOCKING			(CM_PROCESSING_MODE) 1

/* receive_type values. */
#define CM_RECEIVE_AND_WAIT		(CM_RECEIVE_TYPE) 0
#define CM_RECEIVE_IMMEDIATE		(CM_RECEIVE_TYPE) 1

/* return_code values. */
#define CM_OK				(CM_RETURN_CODE) 0
#define CM_ALLOCATE_FAILURE_NO_RETRY	(CM_RETURN_CODE) 1
#define CM_ALLOCATE_FAILURE_RETRY	(CM_RETURN_CODE) 2
#define CM_CONVERSATION_TYPE_MISMATCH	(CM_RETURN_CODE) 3
#define CM_PIP_NOT_SPECIFIED_CORRECTLY	(CM_RETURN_CODE) 5
#define CM_SECURITY_NOT_VALID		(CM_RETURN_CODE) 6
#define CM_SYNC_LVL_NOT_SUPPORTED_LU	(CM_RETURN_CODE) 7
#define CM_SYNC_LVL_NOT_SUPPORTED_SYS	(CM_RETURN_CODE) 7
#define CM_SYNC_LVL_NOT_SUPPORTED_PGM	(CM_RETURN_CODE) 8
#define CM_TPN_NOT_RECOGNIZED		(CM_RETURN_CODE) 9
#define CM_TP_NOT_AVAILABLE_NO_RETRY	(CM_RETURN_CODE) 10
#define CM_TP_NOT_AVAILABLE_RETRY	(CM_RETURN_CODE) 11
#define CM_DEALLOCATED_ABEND		(CM_RETURN_CODE) 17
#define CM_DEALLOCATED_NORMAL		(CM_RETURN_CODE) 18
#define CM_PARAMETER_ERROR		(CM_RETURN_CODE) 19
#define CM_PRODUCT_SPECIFIC_ERROR	(CM_RETURN_CODE) 20
#define CM_PROGRAM_ERROR_NO_TRUNC	(CM_RETURN_CODE) 21
#define CM_PROGRAM_ERROR_PURGING	(CM_RETURN_CODE) 22
#define CM_PROGRAM_ERROR_TRUNC		(CM_RETURN_CODE) 23
#define CM_PROGRAM_PARAMETER_CHECK	(CM_RETURN_CODE) 24
#define CM_PROGRAM_STATE_CHECK		(CM_RETURN_CODE) 25
#define CM_RESOURCE_FAILURE_NO_RETRY	(CM_RETURN_CODE) 26
#define CM_RESOURCE_FAILURE_RETRY	(CM_RETURN_CODE) 27
#define CM_UNSUCCESSFUL			(CM_RETURN_CODE) 28
#define CM_DEALLOCATED_ABEND_SVC	(CM_RETURN_CODE) 30
#define CM_DEALLOCATED_ABEND_TIMER	(CM_RETURN_CODE) 31
#define CM_SVC_ERROR_NO_TRUNC		(CM_RETURN_CODE) 32
#define CM_SVC_ERROR_PURGING		(CM_RETURN_CODE) 33
#define CM_SVC_ERROR_TRUNC		(CM_RETURN_CODE) 34
#define CM_OPERATION_INCOMPLETE		(CM_RETURN_CODE) 35
#define CM_SYSTEM_EVENT			(CM_RETURN_CODE) 36
#define CM_OPERATION_NOT_ACCEPTED	(CM_RETURN_CODE) 37
#define CM_CONVERSATION_ENDING		(CM_RETURN_CODE) 38
#define CM_SEND_RCV_MODE_NOT_SUPPORTED	(CM_RETURN_CODE) 39
#define CM_BUFFER_TOO_SMALL		(CM_RETURN_CODE) 40
#define CM_EXP_DATA_NOT_SUPPORTED	(CM_RETURN_CODE) 41
#define CM_DEALLOC_CONFIRM_REJECT	(CM_RETURN_CODE) 42
#define CM_ALLOCATION_ERROR		(CM_RETURN_CODE) 43
#define CM_RETRY_LIMIT_EXCEEDED		(CM_RETURN_CODE) 44
#define CM_NO_SECONDARY_INFORMATION	(CM_RETURN_CODE) 45
#define CM_SECURITY_NOT_SUPPORTED	(CM_RETURN_CODE) 46
#define CM_SECURITY_MUTUAL_FAILED	(CM_RETURN_CODE) 47
#define CM_CALL_NOT_SUPPORTED		(CM_RETURN_CODE) 48
#define CM_PARM_VALUE_NOT_SUPPORTED	(CM_RETURN_CODE) 49
#define CM_TAKE_BACKOUT			(CM_RETURN_CODE) 100
#define CM_DEALLOCATED_ABEND_BO		(CM_RETURN_CODE) 130
#define CM_DEALLOCATED_ABEND_SVC_BO	(CM_RETURN_CODE) 131
#define CM_DEALLOCATED_ABEND_TIMER_BO	(CM_RETURN_CODE) 132
#define CM_RESOURCE_FAIL_NO_RETRY_BO	(CM_RETURN_CODE) 133
#define CM_RESOURCE_FAILURE_RETRY_BO	(CM_RETURN_CODE) 134
#define CM_DEALLOCATED_NORMAL_BO	(CM_RETURN_CODE) 135
#define CM_CONV_DEALLOC_AFTER_SYNCPT	(CM_RETURN_CODE) 136
#define CM_INCLUDE_PARTNER_REJECT_BO	(CM_RETURN_CODE) 137
#define CM_UNKNOWN_MAP_NAME_REQUESTED	(CM_RETURN_CODE) 138
#define CM_UNKNOWN_MAP_NAME_RECEIVED	(CM_RETURN_CODE) 139
#define CM_UMAP_ROUTINE_ERROR		(CM_RETURN_CODE) 140

/* return_control values. */
#define CM_WHEN_SESSION_ALLOCATED	(CM_RETURN_CONTROL) 0
#define CM_IMMEDIATE			(CM_RETURN_CONTROL) 1
#define CM_WHEN_CONWINNER_ALLOCATED	(CM_RETURN_CONTROL) 2
#define CM_WHEN_SESSION_FREE		(CM_RETURN_CONTROL) 3

/* send_receive_mode values. */
#define CM_HALF_DUPLEX			(CM_SEND_RECEIVE_MODE) 0
#define CM_FULL_DUPLEX			(CM_SEND_RECEIVE_MODE) 1

/* send_type values. */
#define CM_BUFFER_DATA			(CM_SEND_TYPE) 0
#define CM_SEND_AND_FLUSH		(CM_SEND_TYPE) 1
#define CM_SEND_AND_CONFIRM		(CM_SEND_TYPE) 2
#define CM_SEND_AND_PREP_TO_RECEIVE	(CM_SEND_TYPE) 3
#define CM_SEND_AND_DEALLOCATE		(CM_SEND_TYPE) 4

/* status_received values. */
#define CM_NO_STATUS_RECEIVED		(CM_STATUS_RECEIVED) 0
#define CM_SEND_RECEIVED		(CM_STATUS_RECEIVED) 1
#define CM_CONFIRM_RECEIVED		(CM_STATUS_RECEIVED) 2
#define CM_CONFIRM_SEND_RECEIVED	(CM_STATUS_RECEIVED) 3
#define CM_CONFIRM_DEALLOC_RECEIVED	(CM_STATUS_RECEIVED) 4
#define CM_TAKE_COMMIT			(CM_STATUS_RECEIVED) 5
#define CM_TAKE_COMMIT_SEND		(CM_STATUS_RECEIVED) 6
#define CM_TAKE_COMMIT_DEALLOCATE	(CM_STATUS_RECEIVED) 7
#define CM_TAKE_COMMIT_DATA_OK		(CM_STATUS_RECEIVED) 8
#define CM_TAKE_COMMIT_SEND_DATA_OK	(CM_STATUS_RECEIVED) 9
#define CM_TAKE_COMMIT_DEALLOC_DATA_OK	(CM_STATUS_RECEIVED) 10
#define CM_PREPARE_OK			(CM_STATUS_RECEIVED) 11
#define CM_JOIN_TRANSACTION		(CM_STATUS_RECEIVED) 12

/* sync_level values. */
#define CM_NONE				(CM_SYNC_LEVEL) 0
#define CM_CONFIRM			(CM_SYNC_LEVEL) 1
#define CM_SYNC_POINT			(CM_SYNC_LEVEL) 2
#define CM_SYNC_POINT_NO_CONFIRM	(CM_SYNC_LEVEL) 3

/* conversation_security_type values. */
#define CM_SECURITY_NONE		(CM_CONVERSATION_SECURITY_TYPE) 0
#define CM_SECURITY_SAME		(CM_CONVERSATION_SECURITY_TYPE) 1
#define CM_SECURITY_PROGRAM		(CM_CONVERSATION_SECURITY_TYPE) 2
#define CM_SECURITY_DISTRIBUTED		(CM_CONVERSATION_SECURITY_TYPE) 3
#define CM_SECURITY_MUTUAL		(CM_CONVERSATION_SECURITY_TYPE) 4
#define CM_SECURITY_PROGRAM_STRONG	(CM_CONVERSATION_SECURITY_TYPE) 5

/* transaction_control values. */
#define CM_CHAINED_TRANSACTIONS		(CM_TRANSACTION_CONTROL) 0
#define CM_UNCHAINED_TRANSACTIONS	(CM_TRANSACTION_CONTROL) 1

/* maximum sizes of strings and buffers. */
#define CM_CID_SIZE	(8)	/* converstation ID. */
#define CM_CTX_SIZE	(32)	/* context ID. */
#define CM_LD_SIZE	(512)	/* log data. */
#define CM_MN_SIZE	(8)	/* mode name. */
#define CM_PLN_SIZE	(17)	/* partner LU name. */
#define CM_PW_SIZE	(10)	/* password. */
#define CM_SDN_SIZE	(8)	/* symbolic destination name. */
#define CM_TPN_SIZE	(64)	/* TP name. */
#define CM_UID_SIZE	(10)	/* userid ID. */

CM_ENTRY cmacci(unsigned char CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmaccp(unsigned char CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmallc(unsigned char CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmcanc(unsigned char CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmcfm(unsigned char CM_PTR,
        CM_CONTROL_INFORMATION_RECEIVED CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmcfmd(unsigned char CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmcnvi(unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmcnvo(unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmdeal(unsigned char CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmdfde(unsigned char CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmeaeq(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_AE_QUAL_OR_AP_TITLE_FORMAT CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmeapt(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_AE_QUAL_OR_AP_TITLE_FORMAT CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmeacn(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmecs(unsigned char CM_PTR,
        CM_CONVERSATION_STATE CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmect(unsigned char CM_PTR,
        CM_CONVERSATION_TYPE CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmectx(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmeid(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmembs(CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmemn(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmepid(unsigned char CM_PTR,
        CM_PARTNER_ID_TYPE CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_INT32 CM_PTR,
        CM_PARTNER_ID_SCOPE CM_PTR,
        CM_DIRECTORY_SYNTAX CM_PTR,
        CM_DIRECTORY_ENCODING CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmepln(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmesi(unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_DATA_RECEIVED_TYPE CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmesl(unsigned char CM_PTR,
        CM_SYNC_LEVEL CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmesrm(unsigned char CM_PTR,
        CM_SEND_RECEIVE_MODE CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmesui(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmetc(unsigned char CM_PTR,
        CM_TRANSACTION_CONTROL CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmetpn(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmflus(unsigned char CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmincl(unsigned char CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cminic(unsigned char CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cminit(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmprep(unsigned char CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmptr(unsigned char CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmrcv(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_DATA_RECEIVED_TYPE CM_PTR,
        CM_INT32 CM_PTR,
        CM_STATUS_RECEIVED CM_PTR,
        CM_CONTROL_INFORMATION_RECEIVED CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmrcvx(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_INT32 CM_PTR,
        CM_CONTROL_INFORMATION_RECEIVED CM_PTR,
        CM_RECEIVE_TYPE CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmrltp(unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmrts(unsigned char CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsaeq(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_AE_QUAL_OR_AP_TITLE_FORMAT CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsac(unsigned char CM_PTR,
        CM_ALLOCATE_CONFIRM_TYPE CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsacn(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsapt(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_AE_QUAL_OR_AP_TITLE_FORMAT CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsbt(unsigned char CM_PTR,
        CM_BEGIN_TRANSACTION CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmscsp(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmscst(unsigned char CM_PTR,
        CM_CONVERSATION_SECURITY_TYPE CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmscsu(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsct(unsigned char CM_PTR,
        CM_CONVERSATION_TYPE CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmscu(unsigned char CM_PTR,
        CM_CONFIRMATION_URGENCY CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsdt(unsigned char CM_PTR,
        CM_DEALLOCATE_TYPE CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsed(unsigned char CM_PTR,
        CM_ERROR_DIRECTION CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsend(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_CONTROL_INFORMATION_RECEIVED CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmserr(unsigned char CM_PTR,
	CM_CONTROL_INFORMATION_RECEIVED CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsf(unsigned char CM_PTR,
        CM_FILL CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsid(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsld(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsltp(unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsmn(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsndx(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_CONTROL_INFORMATION_RECEIVED CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmspdp(unsigned char CM_PTR,
        CM_PREPARE_DATA_PERMITTED_TYPE CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmspid(unsigned char CM_PTR,
        CM_PARTNER_ID_TYPE CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_PARTNER_ID_SCOPE CM_PTR,
        CM_DIRECTORY_SYNTAX CM_PTR,
        CM_DIRECTORY_ENCODING CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmspln(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmspm(unsigned char CM_PTR,
        CM_PROCESSING_MODE CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsptr(unsigned char CM_PTR,
        CM_PREPARE_TO_RECEIVE_TYPE CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsqcf(unsigned char CM_PTR,
        CM_CONVERSATION_QUEUE CM_PTR,
        unsigned char CM_PTR,
        unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsqpm(unsigned char CM_PTR,
        CM_CONVERSATION_QUEUE CM_PTR,
        CM_PROCESSING_MODE CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsrc(unsigned char CM_PTR,
        CM_RETURN_CONTROL CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsrt(unsigned char CM_PTR,
        CM_RECEIVE_TYPE CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmssl(unsigned char CM_PTR,
        CM_SYNC_LEVEL CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmssrm(unsigned char CM_PTR,
        CM_SEND_RECEIVE_MODE CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmsst(unsigned char CM_PTR,
        CM_SEND_TYPE CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmstc(unsigned char CM_PTR,
        CM_TRANSACTION_CONTROL CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmstpn(unsigned char CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmtrts(unsigned char CM_PTR,
        CM_CONTROL_INFORMATION_RECEIVED CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmwait(unsigned char CM_PTR,
        CM_RETURN_CODE CM_PTR,
        CM_RETURN_CODE CM_PTR);
CM_ENTRY cmwcmp(unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        CM_INT32 CM_PTR,
        unsigned char CM_PTR,
        CM_INT32 CM_PTR,
        unsigned char CM_PTR,
        CM_RETURN_CODE CM_PTR);

/* Macros to use long descriptive names instead of crusty CPI-C names. */
#define Accept_Conversation(v1,v2)			cmaccp(v1,v2)
#define Accept_Incoming(v1,v2)				cmacci(v1,v2)
#define Allocate(v1,v2)					cmallc(v1,v2)
#define Cancel_Conversation(v1,v2)			cmcanc(v1,v2)
#define Confirm(v1,v2,v3)				cmcfm(v1,v2,v3)
#define Confirmed(v1,v2)				cmcfmd(v1,v2)
#define Convert_Incoming(v1,v2,v3)			cmcnvi(v1,v2,v3)
#define Convert_Outgoing(v1,v2,v3)			cmcnvo(v1,v2,v3)
#define Deallocate(v1,v2)				cmdeal(v1,v2)
#define Deferred_Deallocate(v1,v2)			cmdfde(v1,v2)
#define Extract_AE_Qualifier(v1,v2,v3,v4,v5)		cmeaeq(v1,v2,v3,v4,v5)
#define Extract_AP_Title(v1,v2,v3,v4,v5)		cmeapt(v1,v2,v3,v4,v5)
#define Extract_Application_Context_Name(v1,v2,v3,v4)	cmeacn(v1,v2,v3,v4)
#define Extract_Conversation_Context(v1,v2,v3,v4)	cmectx(v1,v2,v3,v4)
#define Extract_Conversation_State(v1,v2,v3)		cmecs(v1,v2,v3)
#define Extract_Conversation_Type(v1,v2,v3)		cmect(v1,v2,v3)
#define Extract_Initialization_Data(v1,v2,v3,v4,v5)	cmeid(v1,v2,v3,v4,v5)
#define Extract_Maximum_Buffer_Size(v1,v2)		cmembs(v1,v2)
#define Extract_Mode_Name(v1,v2,v3,v4,v5)		cmemn(v1,v2,v3,v4,v5)
#define Extract_Partner_ID_Name(v1,v2,v3,v4,v5,v6,v7,v8,v9) \
	cmepid(v1,v2,v3,v4,v5,v6,v7,v8,v9)
#define Extract_Partner_LU_Name(v1,v2,v3,v4)		cmpln(v1,v2,v3,v4)
#define Extract_Secondary_Information(v1,v2,v3,v4,v5,v6,v7) \
	cmesi(v1,v2,v3,v4,v5,v6,v7)
#define Extract_Security_User_ID(v1,v2,v3,v4)		cmesui(v1,v2,v3,v4)
#define Extract_Send_Receive_Mode(v1,v2,v3)		cmesrm(v1,v2,v3)
#define Extract_Sync_Level(v1,v2,v3)			cmesl(v1,v2,v3)
#define Extract_Transaction_Control(v1,v2,v3)		cmetc(v1,v2,v3)
#define Extract_TP_Name(v1,v2,v3,v4)			cmetpn(v1,v2,v3,v4)
#define Flush(v1,v2)					cmflus(v1,v2)
#define Include_Partner_In_Transaction(v1,v2)		cmincl(v1,v2)
#define Initialize_Conversation(v1,v2,v3)		cminit(v1,v2,v3)
#define Initialize_For_Incomint(v1,v2)			cminic(v1,v2)
#define Prepare(v1,v2)					cmprep(v1,v2)
#define Prepare_To_Receive(v1,v2)			cmptr(v1,v2)
#define Receive(v1,v2,v3,v4,v5,v6,v7,v8)		\
	cmrcv(v1,v2,v3,v4,v5,v6,v7,v8)
#define Receive_Expedited_Data(v1,v2,v3,v4,v5,v6,v7)	\
	cmrcvx(v1,v2,v3,v4,v5,v6,v7)
#define Release_Local_TP_Name(v1,v2,v3)			cmrltp(v1,v2,v3)
#define Request_To_Send(v1,v2)				cmrts(v1,v2)
#define Send_Data(v1,v2,v3,v4,v5)			cmsend(v1,v2,v3,v4,v5)
#define Send_Error(v1,v2,v3)				cmserr(v1,v2,v3)
#define Send_Expedited_Data(v1,v2,v3,v4,v5)		cmsndx(v1,v2,v3,v4,v5)
#define Set_AE_Qualifier(v1,v2,v3,v4,v5)		cmsaeq(v1,v2,v3,v4,v5)
#define Set_Allocate_Confirm(v1,v2,v3)			cmsac(v1,v2,v3)
#define Set_AP_Title(v1,v2,v3,v4,v5)			cmsapt(v1,v2,v3,v4,v5)
#define Set_Application_Context_Name(v1,v2,v3,v4)	cmsacn(v1,v2,v3,v4)
#define Set_Begin_Transaction(v1,v2,v3)			cmsbt(v1,v2,v3)
#define Set_Confirmation_Urgency(v1,v2,v3)		cmscu(v1,v2,v3)
#define Set_Conversation_Security_Password(v1,v2,v3,v4)	cmscsp(v1,v2,v3,v4)
#define Set_Conversation_Security_Type(v1,v2,v3)	cmscst(v1,v2,v3)
#define Set_Conversation_Security_User_ID(v1,v2,v3,v4)	cmscsu(v1,v2,v3,v4)
#define Set_Conversation_Type(v1,v2,v3)			cmsct(v1,v2,v3)
#define Set_Deallocate_Type(v1,v2,v3)			cmsdt(v1,v2,v3)
#define Set_Error_Direction(v1,v2,v3)			cmsed(v1,v2,v3)
#define Set_Fill(v1,v2,v3)				cmsf(v1,v2,v3)
#define Set_Initialization_Data(v1,v2,v3,v4)		cmsid(v1,v2,v3,v4)
#define Set_Log_Data(v1,v2,v3,v4)			cmsld(v1,v2,v3,v4)
#define Set_Mode_Name(v1,v2,v3,v4)			cmsmn(v1,v2,v3,v4)
#define Set_Partner_ID_Name(v1,v2,v3,v4,v5,v6,v7,v8)	\
	cmspid(v1,v2,v3,v4,v5,v6,v7,v8)
#define Set_Partner_LU_Name(v1,v2,v3,v4)		cmspln(v1,v2,v3,v4)
#define Set_Prepare_Data_Permitted(v1,v2,v3)		cmspdp(v1,v2,v3)
#define Set_Prepare_To_Receive_Type(v1,v2,v3)		cmsptr(v1,v2,v3)
#define Set_Processing_Mode(v1,v2,v3)			cmspm(v1,v2,v3)
#define Set_Queue_Callback_Function(v1,v2,v3,v4,v5,v6)	\
	cmsqcf(v1,v2,v3,v4,v5,v6)
#define Set_Queue_Processing_Mode(v1,v2,v3,v4,v5,v6)	\
	cmsqpm(v1,v2,v3,v4,v5,v6)
#define Set_Receive_Type(v1,v2,v3)			cmsrt(v1,v2,v3)
#define Set_Return_Control(v1,v2,v3)			cmsrc(v1,v2,v3)
#define Set_Send_Type(v1,v2,v3)				cmsst(v1,v2,v3)
#define Set_Sync_Level(v1,v2,v3)			cmssl(v1,v2,v3)
#define Set_TP_Name(v1,v2,v3,v4)			cmstpn(v1,v2,v3,v4)
#define Set_Transaction_Control(v1,v2,v3)		cmstc(v1,v2,v3)
#define Set_Send_Receive_Mode(v1,v2,v3)			cmssrm(v1,v2,v3)
#define Specify_Local_TP_Name(v1,v2,v3)			cmsltp(v1,v2,v3)
#define Test_Request_To_Send_Received(v1,v2,v3)		cmtrts(v1,v2,v3)
#define Wait_For_Completion(v1,v2,v3,v4,v5,v6,v7)	\
	cmwcmp(v1,v2,v3,v4,v5,v6,v7)
#define Wait_For_Conversation(v1,v2,v3)			cmwait(v1,v2,v3)

/* Input/Output CPI-C call structure. */
typedef struct {
        unsigned long   *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
} cpic_args;

#define cargo1(a, v1)                   \
        a = (cpic_args *)malloc(sizeof(cpic_args)); \
        a->a1 = (unsigned long *)v1;

#define cargo2(a, v1, v2)               \
        a = (cpic_args *)malloc(sizeof(cpic_args)); \
        a->a1 = (unsigned long *)v1; \
        a->a2 = (unsigned long *)v2;

#define cargo3(a, v1, v2, v3)           \
        a = (cpic_args *)malloc(sizeof(cpic_args)); \
        a->a1 = (unsigned long *)v1; \
        a->a2 = (unsigned long *)v2; \
        a->a3 = (unsigned long *)v3;

#define cargo4(a, v1, v2, v3, v4)       \
        a = (cpic_args *)malloc(sizeof(cpic_args)); \
        a->a1 = (unsigned long *)v1; \
        a->a2 = (unsigned long *)v2; \
        a->a3 = (unsigned long *)v3; \
        a->a4 = (unsigned long *)v4;

#define cargo5(a, v1, v2, v3, v4, v5)   \
        a = (cpic_args *)malloc(sizeof(cpic_args)); \
        a->a1 = (unsigned long *)v1; \
        a->a2 = (unsigned long *)v2; \
        a->a3 = (unsigned long *)v3; \
        a->a4 = (unsigned long *)v4; \
        a->a5 = (unsigned long *)v5;

#define cargo6(a, v1, v2, v3, v4, v5, v6) \
        a = (cpic_args *)malloc(sizeof(cpic_args)); \
        a->a1 = (unsigned long *)v1; \
        a->a2 = (unsigned long *)v2; \
        a->a3 = (unsigned long *)v3; \
        a->a4 = (unsigned long *)v4; \
        a->a5 = (unsigned long *)v5; \
        a->a6 = (unsigned long *)v6;

#define cargo7(a, v1, v2, v3, v4, v5, v6, v7) \
        a = (cpic_args *)malloc(sizeof(cpic_args)); \
        a->a1 = (unsigned long *)v1; \
        a->a2 = (unsigned long *)v2; \
        a->a3 = (unsigned long *)v3; \
        a->a4 = (unsigned long *)v4; \
        a->a5 = (unsigned long *)v5; \
        a->a6 = (unsigned long *)v6; \
        a->a7 = (unsigned long *)v7;

#endif	/* _CPIC_H */
