/* appc.h: Advanced Program-to-Program Communications structures and defs.
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

#ifndef _APPC_H
#define _APPC_H

#define AC_MAX_BUFFER				32767

/* Linux-SNA APPC Call OPCODES */
#ifdef NOT
#define AC_ALLOCATE				0x0001
#define AC_MC_ALLOCATE				0x0010
#define AC_DEALLOCATE				0x0020
#define AC_MC_DEALLOCATE			0x0030
#define AC_GET_ATTRIBUTES			0x0040
#define AC_GET_TP_PROPERTIES			0x0050
#define AC_RECEIVE_AND_WAIT			0x0060
#define AC_MC_RECEIVE_AND_WAIT			0x0070
#define AC_REQUEST_TO_SEND			0x0080
#define AC_MC_REQUEST_TO_SEND			0x0090
#define AC_SEND_DATA				0x00A0
#define AC_MC_SEND_DATA				0x00B0
#define AC_CONFIRM				0x00C0
#define AC_MC_CONFIRM				0x00D0
#define AC_CONFIRMED				0x00E0
#define AC_MC_CONFIRMED				0x00F0
#define AC_SEND_ERROR				0x0100
#define AC_MC_SEND_ERROR			0x0110
#define AC_CHANGE_SESSION_LIMIT			0x0120
#define AC_INIT_SESSION_LIMIT			0x0130
#define AC_PROCESS_SESSION_LIMIT		0x0140
#define AC_RESET_SESSION_LIMIT			0x0150
#define AC_MC_GET_ATTRIBUTES			0x0160
#define AC_GET_TYPE				0x0170
#define AC_SET_SYNCPT_OPTIONS			0x0180
#define AC_FLUSH				0x0190
#define AC_MC_FLUSH				0x01A0
#define AC_POST_ON_RECEIPT			0x01B0
#define AC_MC_POST_ON_RECEIPT			0x01C0
#define AC_PREPARE_TO_RECEIVE			0x01D0
#define AC_MC_PREPARE_TO_RECEIVE		0x01E0
#define AC_RECEIVE_EXPEDITED_DATA		0x01F0
#define AC_RECEIVE_IMMEDIATE			0x0200
#define AC_MC_RECEIVE_IMMEDIATE			0x0210
#define AC_SEND_EXPEDITED_DATA			0x0220
#define AC_MC_SEND_EXPEDITED_DATA		0x0230
#define AC_TEST					0x0240
#define AC_MC_TEST				0x0250
#define AC_WAIT					0x0260
#define AC_WAIT_FOR_COMPLETION			0x0270
#define AC_BACKOUT				0x0280
#define AC_PREPARE_FOR_SYNCPT			0x0290
#define AC_MC_PREPARE_FOR_SYNCPT		0x02A0
#define AC_ACTIVATE_SESSION			0x02B0
#define AC_DEACTIVATE_CONVERSATION_GROUP	0x02C0
#define AC_DEACTIVATE_SESSION			0x02D0
#define AC_DEFINE_LOCAL_LU			0x02E0
#define AC_DEFINE_MODE				0x02F0
#define AC_DEFINE_REMOTE_LU			0x0300
#define AC_DEFINE_TP				0x0310
#define AC_DELETE				0x0320
#define AC_DISPLAY_LOCAL_LU			0x0330
#define AC_DISPLAY_MODE				0x0340
#define AC_DISPLAY_REMOTE_LU			0x0350
#define AC_DISPLAY_TP				0x0360
#define AC_PROCESS_SIGNOFF			0x0370
#define AC_SIGNOFF				0x0380
#endif

typedef enum {
	AC_REASON_NONE = 0,
	AC_REASON_FULL_LL,
	AC_REASON_FULL_BUFFER
} appc_reasons;

/* what received values, maps to cpic data_received values. */
#define AC_WR_NO_DATA_RECEIVED			0
#define AC_WR_DATA                		1
#define AC_WR_DATA_COMPLETE       		2
#define AC_WR_DATA_INCOMPLETE     		3
#define AC_WR_LL_TRUNCATED			4
#define AC_WR_CONFIRM				5
#define AC_WR_CONFIRM_SEND			6
#define AC_WR_SEND				7
#define AC_WR_CONFIRM_DEALLOCATE		8
#define AC_WR_DEALLOCATE_NORMAL			9

/* status_received values. */
#define AC_SR_NO_STATUS_RECEIVED		0
#define AC_SR_SEND_RECEIVED                	1
#define AC_SR_CONFIRM_RECEIVED             	2
#define AC_SR_CONFIRM_SEND_RECEIVED        	3
#define AC_SR_CONFIRM_DEALLOCATE_RECEIVED     	4
#define AC_SR_TAKE_COMMIT                  	5
#define AC_SR_TAKE_COMMIT_SEND             	6
#define AC_SR_TAKE_COMMIT_DEALLOCATE       	7
#define AC_SR_TAKE_COMMIT_DATA_OK          	8
#define AC_SR_TAKE_COMMIT_SEND_DATA_OK     	9
#define AC_SR_TAKE_COMMIT_DEALLOC_DATA_OK  	10
#define AC_SR_PREPARE_OK                   	11
#define AC_SR_JOIN_TRANSACTION             	12

typedef enum {
	AC_FILL_LL = 0,
	AC_FILL_BUFFER
} appc_fill_types;

typedef enum {
	AC_SYNC_LEVEL_NONE = 0,
	AC_SYNC_LEVEL_CONFIRM,
	AC_SYNC_LEVEL_NO_CONFIRM,
	AC_SYNC_LEVEL_SYNCPT
} appc_sync_levels;

typedef enum {
	AC_INITIALIZATION_QUEUE = 0,
	AC_SEND_RECEIVE_QUEUE,
	AC_EXPEDITED_SEND_QUEUE,
	AC_EXPEDITED_RECEIVE_QUEUE
} appc_queues;

typedef enum {
	AC_CONVERSATION_TYPE_BASIC = 0,
	AC_CONVERSATION_TYPE_MAPPED
} appc_conversation_types;

typedef enum {
	AC_ALLOCATE_NO_CONFIRM = 0,
	AC_ALLOCATE_CONFIRM
} appc_allocate_confirm;

typedef enum {
	AC_BEGIN_IMPLICIT = 0,
	AC_BEGIN_EXPLICIT
} appc_begin_transaction;

typedef enum {
	AC_CONFIRMATION_NOT_URGENT = 0,
	AC_CONFIRMATION_URGENT
} appc_confirm_urgency;

typedef enum {
	AC_SECURITY_NONE = 0,
	AC_SECURITY_SAME,
	AC_SECURITY_PROGRAM
} appc_security;

typedef enum {
	AC_DEALLOCATE_LOCAL = 0,
	AC_DEALLOCATE_FLUSH,
	AC_DEALLOCATE_CONFIRM,
	AC_DEALLOCATE_SYNC_LEVEL,
	AC_DEALLOCATE_ABEND,
	AC_DEALLOCATE_ABEND_PROG,
	AC_DEALLOCATE_ABEND_SVC,
	AC_DEALLOCATE_ABEND_TIMER
} appc_deallocate_types;

typedef enum {
	AC_DEFAULT_ENCODING = 0,
	AC_UNICODE_ENCODING
} appc_directory_type;

typedef enum {
	AC_DEFAULT_SYNTAX = 0,
	AC_DCE_SYNTAX,
	AC_XDS_SYNTAX,
	AC_NDS_SYNTAX
} appc_directory_syntax;

typedef enum {
	AC_RECEIVE_ERROR = 0,
	AC_SEND_ERROR
} appc_error_direction;

typedef enum {
	AC_EXPLICIT = 0,
	AC_REFERENCE
} appc_partner_id_scope;

typedef enum {
	AC_DISTINGUISHED_NAME = 0,
	AC_LOCAL_DISTINGUISHED_NAME,
	AC_PROGRAM_FUNCTION_ID,
	AC_OSI_TPSU_TITLE_OID,
	AC_PROGRAM_BINDING
} appc_parnter_id_type;

typedef enum {
	AC_PREPARE_DATA_NOT_PERMITTED = 0,
	AC_PREPARE_DATA_PERMITTED
} appc_prepare_data_permitted;

typedef enum {
	AC_PREP_TO_RECEIVE_SYNC_LEVEL = 0,
	AC_PREP_TO_RECEIVE_FLUSH,
	AC_PREP_TO_RECEIVE_CONFIRM
} appc_prepare_to_receive_type;

typedef enum {
	AC_BLOCKING = 0,
	AC_NON_BLOCKING
} appc_processing_mode;

typedef enum {
	AC_RECEIVE_AND_WAIT = 0,
	AC_RECEIVE_IMMEDIATE
} appc_receive_type;

typedef enum {
	AC_WHEN_SESSION_ALLOCATED = 0,
	AC_IMMEDIATE
} appc_return_control;

typedef enum {
	AC_HALF_DUPLEX = 0,
	AC_FULL_DUPLEX
} appc_send_receive_mode;

typedef enum {
	AC_BUFFER_DATA = 0,
	AC_SEND_AND_FLUSH,
	AC_SEND_AND_CONFIRM,
	AC_SEND_AND_PREP_TO_RECEIVE,
	AC_SEND_AND_DEALLOCATE
} appc_send_type;

typedef enum {
	AC_CHAINED_TRANSACTIONS = 0,
	AC_UNCHAINED_TRANSACTIONS
} appc_tranaction_control;

/* Linux-SNA APPC return codes */
typedef enum {
	AC_RC_OK = 0,
	AC_RC_UNSUCCESSFUL,
	AC_RC_INVALID_VERB,
	AC_RC_PROGRAM_PARAMETER_CHECK,
	AC_RC_PROGRAM_STATE_CHECK,
	AC_RC_RESOURCE_FAILURE_RETRY,
	AC_RC_RESOURCE_FAILURE_NO_RETRY,
	AC_RC_ACTIVATION_FAILURE_RETRY,
	AC_RC_ACTIVATION_FAILIRE_NO_RETRY,
	AC_RC_PROG_ERROR_NO_TRUNC,
	AC_RC_PROG_ERROR_PURGING,
	AC_RC_SVC_ERROR_NO_TRUNC,
	AC_RC_SVC_ERROR_TRUNC,
	AC_RC_SVC_ERROR_PURGING,
	AC_RC_DEALLOCATE_NORMAL,
	AC_RC_DEALLOCATE_ABEND,
	AC_RC_DEALLOCATE_ABEND_PROG,
	AC_RC_DEALLOCATE_ABEND_SVC,
	AC_RC_DEALLOCATE_ABEND_TIMER,
	AC_RC_PIP_NOT_ALLOWED,
	AC_RC_PIP_NOT_SPECIFIED_CORRECTLY,
	AC_RC_CONVERSATION_TYPE_MISMATCH,
	AC_RC_SYNC_LEVEL_NOT_SUPPORTED,
	AC_RC_SECURITY_NOT_VALID,
	AC_RC_TP_NAME_NOT_RECOGNIZED,
	AC_RC_TP_NOT_AVAIL_RETRY,
	AC_RC_TP_NOT_AVAIL_NO_RETRY,
	AC_RC_PRODUCT_SPECIFIC,
	AC_RC_ALLOCATION_FAILURE_NO_RETRY,
	AC_RC_ALLOCATION_FAILURE_RETRY
} appc_return_codes;

#define AC_BAD_TP_ID				0x0010
#define AC_BAD_CONV_ID				0x0020
#define AC_BAD_LU_ALIAS				0x0030
#define AC_INVALID_DATA_SEGMENT			0x0040
#define AC_BAD_CONV_TYPE			0x0050
#define AC_BAD_SYNC_LEVEL			0x0060
#define AC_BAD_SECURITY				0x0070
#define AC_BAD_RETURN_CONTROL			0x0080
#define AC_PIP_LEN_INCORRECT			0x0090
#define AC_NO_USE_OF_SNASVCMG			0x00A0
#define AC_UNKNOWN_PARTNER_MODE			0x00B0
#define AC_CONFIRM_ON_SYNC_LEVEL_NONE		0x00C0
#define AC_DEALLOC_BAD_TYPE			0x00D0
#define AC_DEALLOC_LOG_LL_WRONG			0x00E0
#define AC_P_TO_R_INVALID_TYPE			0x00F0
#define AC_RCV_AND_WAIT_BAD_FILL		0x0100
#define AC_RCV_IMMD_BAD_FILL			0x0110
#define AC_RCV_AND_POST_BAD_FILL		0x0120
#define AC_INVALID_SEMAPHONE_HANDLE		0x0130
#define AC_BAD_RETURN_STATUS_WITH_DATA		0x0140
#define AC_BAD_LL				0x0150
#define AC_SEND_INVALID_TYPE			0x0160
#define AC_INVALID_SESSION_ID			0x0170
#define AC_INVALID_POLARITY			0x0180
#define AC_INVALID_TYPE				0x0190
#define AC_INVALID_LU_ALIAS			0x01A0
#define AC_INVALID_PLU_ALIAS			0x01B0
#define AC_INVALID_MODE_NAME			0x01C0
#define AC_INVALID_TRANSACT_ID			0x01D0
#define AC_SEND_DATA_CONFIRM_SYNC_LEVEL_NONE	0x01E0
#define AC_BAD_PARTNER_LU_ALIAS			0x01F0
#define AC_SEND_ERROR_LOG_LL_WRONG		0x0200
#define AC_SEND_ERROR_BAD_TYPE			0x0210
#define AC_BAD_ERROR_DIRECTION			0x0220
#define AC_TOO_MANY_TPS				0x0230
#define AC_BAD_TYPE				0x0240
#define AC_UNDEFINED_TP_NAME			0x0250
#define AC_INVALID_SET_PROT			0x0260
#define AC_INVALID_NEW_PROT			0x0270
#define AC_INVALID_SET_UNPROT			0x0280
#define AC_INVALID_NEW_UNPROT			0x0290
#define AC_INVALID_SET_USER			0x02A0
#define AC_INVALID_DATA_TYPE			0x02B0
#define AC_BAD_LOCAL_LU_ALIAS			0x02C0
#define AC_BAD_REMOTE_LU_ALIAS			0x02D0
#define AC_POST_ON_RECEIPT_BAD_FILL		0x02E0
#define AC_CONFIRM_BAD_STATE			0x0300
#define AC_CONFIRM_NO_LL_BDY			0x0310
#define AC_CONFIRMED_BAD_STATE			0x0320
#define AC_DEALLOC_FLUSH_BAD_STATE		0x0330
#define AC_DEALLOC_CONFIRM_BAD_STATE		0x0340
#define AC_DEALLOC_NOT_LL_BDY			0x0350
#define AC_FLUSH_NOT_SEND_STATE			0x0360
#define AC_P_TO_R_NOT_LL_BDY			0x0370
#define AC_P_TO_R_NOT_SEND_STATE		0x0380
#define AC_RCV_AND_WAIT_BAD_STATE		0x0390
#define AC_RCV_AND_WAIT_NOT_LL_BDY		0x03A0
#define AC_RCV_IMMD_BAD_STATE			0x03B0
#define AC_RCV_AND_POST_BAD_STATE		0x03C0
#define AC_RCV_AND_POST_NOT_LL_BDY		0x03D0
#define AC_R_T_S_BAD_STATE			0x03E0
#define AC_SEND_DATA_NOT_SEND_STATE		0x03F0
#define AC_SEND_DATA_NOT_LL_BDY			0x0400
#define AC_ATTACH_MANAGER_INACTIVE		0x0410
#define AC_ALLOCATE_NOT_PENDING			0x0420
#define AC_INVALID_PROCESS			0x0430
#define AC_ALLOCATION_ERROR			0x0440
#define AC_CONV_FAILURE_RETRY			0x0560
#define AC_CONV_FAILURE_NO_RETRY		0x0570
#define AC_CONVERSATION_TYPE_MIXED		0x05B0
#define AC_CANCELLED				0x05C0
#define AC_SECURITY_REQUESTED_NOT_SUPPORTED	0x05D0
#define AC_TP_BUSY				0x05E0
#define AC_BACKED_OUT				0x05F0
#define AC_BO_NO_RESYNC				0x0600
#define AC_BO_RESYNC				0x0610
#define AC_SESSION_LIMITS_CLOSED		0x0640
#define AC_SESSION_LIMITS_EXCEEDED		0x0650
#define AC_VERB_IN_PROGRESS			0x0660
#define AC_SESSION_DEACTIVATED			0x0670
#define AC_COMM_SUBSYSTEM_ABENDED		0x0680
#define AC_COMM_SUBSYSTEM_NOT_LOADED		0x0690
#define AC_CONV_BUSY				0x06A0

#ifdef NOT

/* Linux-SNA APPC call data structures. */
struct sna_allocate {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
	unsigned char	conv_type;
	unsigned char	sync_level;
	unsigned char	rtn_ctl;
	unsigned long	conv_group_id;
	unsigned long	sense_data;
	unsigned char	plu_alias[8];
	unsigned char	mode_name[8];
	unsigned char 	tp_name[64];
	unsigned char	security;
	unsigned char	pwd[10];
	unsigned char	user_id[10];
	unsigned short	pip_dlen;
	unsigned char	*pip_dptr;
	unsigned char	fqplu_name[17];
};

struct sna_mc_allocate {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
	unsigned char	sync_level;
	unsigned char	rtn_ctl;
	unsigned long	conv_group_id;
	unsigned long	sense_data;
	unsigned char	plu_alias[8];
	unsigned char	mode_name[8];
	unsigned char	tp_name[64];
	unsigned char	security;
	unsigned char	pwd[10];
	unsigned char	user_id[10];
	unsigned short	pip_dlen;
	unsigned char	*pip_dptr;
	unsigned char	fqplu_name[17];
};

struct sna_deallocate {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
	unsigned char	dealloc_type;
	unsigned short	log_dlen;
	unsigned char	*log_dptr;
};

struct sna_mc_deallocate {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
	unsigned char	dealloc_type;
};

struct sna_get_attributes {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
	unsigned char	sync_level;
	unsigned char	mode_name[8];
	unsigned char	net_name[8];
	unsigned char	lu_name[8];
	unsigned char	lu_alias[8];
	unsigned char	plu_alias[8];
	unsigned char	plu_un_name[8];
	unsigned char	fqplu_name[17];
	unsigned char	user_id[10];
	unsigned long	conv_group_id;
	unsigned char	conv_corr_len;
	unsigned char	conv_corr[8];
	unsigned char	luw_id[26];
	unsigned char	sess_id[8];
};

struct sna_get_tp_properties {

};

struct sna_receive_and_wait {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
	unsigned short	what_rcvd;
	unsigned char	rtn_status;
	unsigned char	fill;
	unsigned char	rts_rcvd;
	unsigned short	max_len;
	unsigned short	dlen;
	unsigned char	*dptr;
};

struct sna_mc_receive_and_wait {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
	unsigned short	what_rcvd;
	unsigned char	rtn_status;
	unsigned char	rts_rcvd;
	unsigned short	max_len;
	unsigned short	dlen;
	unsigned char	*dptr;
};

struct sna_request_to_send {

};

struct sna_mc_request_to_send {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
};

struct sna_send_data {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
	unsigned char	rts_rcvd;
	unsigned char	data_type;
	unsigned short	dlen;
	unsigned char	*dptr;
	unsigned char	type;
};

struct sna_mc_send_data {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
	unsigned char	rts_rcvd;
	unsigned char	data_type;
	unsigned short	dlen;
	unsigned char	*dptr;
	unsigned char	type;
};

struct sna_confirm {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
	unsigned char	rts_rcvd;
};

struct sna_mc_confirm {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
	unsigned char	rts_rcvd;
};

struct sna_confirmed {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
};

struct sna_mc_confirmed {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
};

struct sna_send_error {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
	unsigned char	rts_rcvd;
	unsigned char	err_type;
	unsigned char	err_dir;
	unsigned short	log_dlen;
	unsigned char	*log_dptr;
};

struct sna_mc_send_error {
	unsigned char   tp_id[8];
	unsigned long   conv_id;
	unsigned char   rts_rcvd;
	unsigned char   err_type;
	unsigned char   err_dir;
	unsigned short  log_dlen;
	unsigned char   *log_dptr;
};

struct sna_change_session_limit {

};

struct sna_init_session_limit {

};

struct sna_process_session_limit {

};

struct sna_reset_session_limit {

};

struct sna_mc_get_attributes {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
	unsigned char	sync_level;
	unsigned char	mode_name[8];
	unsigned char	net_name[8];
	unsigned char	lu_name[8];
	unsigned char	lu_alias[8];
	unsigned char	plu_alias[8];
	unsigned char	plu_un_name[8];
	unsigned char	fqplu_name[17];
	unsigned char	user_id[10];
	unsigned long	conv_group_id;
	unsigned char	conv_corr_len;
	unsigned char	conv_corr[8];
	unsigned char	luw_id[26];
	unsigned char	sess_id[8];
};

struct sna_get_type {

};

struct sna_set_syncpt_options {

};

struct sna_flush {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
};

struct sna_mc_flush {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
};

struct sna_post_on_receipt {

};

struct sna_mc_post_on_receipt {

};

struct sna_prepare_to_receive {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
	unsigned char	ptr_type;
	unsigned char	locks;
};

struct sna_mc_prepare_to_receive {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
	unsigned char	ptr_type;
	unsigned char	locks;
};

struct sna_receive_expedited_data {

};

struct sna_mc_receive_expedited_data {

};

struct sna_receive_immediate {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
	unsigned short	what_rcvd;
	unsigned char	rtn_status;
	unsigned char	fill;
	unsigned char	rts_rcvd;
	unsigned short	max_len;
	unsigned short	dlen;
	unsigned char	*dptr;
};

struct sna_mc_receive_immediate {
	unsigned char	tp_id[8];
	unsigned long	conv_id;
	unsigned short	what_rcvd;
	unsigned char	rtn_status;
	unsigned char	rts_rcvd;
	unsigned short	max_len;
	unsigned short	dlen;
	unsigned char	*dptr;
};

struct sna_send_expedited_data {

};

struct sna_mc_send_expedited_data {

};

struct sna_test {

};

struct sna_mc_test {

};

struct sna_wait {

};

struct sna_wait_for_completion {

};

struct sna_backout {

};

struct sna_prepare_for_syncpt {

};

struct sna_mc_prepare_for_syncpt {

};

struct sna_syncpt {

};

struct sna_activate_session {

};

struct sna_deactivate_conversation_group {

};

struct sna_deactivate_session {

};

struct sna_define_local_lu {

};

struct sna_define_mode {

};

struct sna_define_remote_lu {

};

struct sna_define_tp {

};

struct sna_delete {

};

struct sna_display_local_lu {

};

struct sna_display_mode {

};

struct sna_display_remote_lu {

};

struct sna_display_tp {

};

struct sna_process_signoff {

};

struct sna_signoff {

};
#endif

extern void appc(unsigned short opcode, unsigned char opext,
	unsigned short rcpri, unsigned long rcsec, void *uaddr);

#ifdef __KERNEL__

struct appc_ops {
	int family;
};

#endif /* __KERNEL__ */
#endif /* _APPC_H */
