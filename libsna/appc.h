/* appc.h: APPC Header.
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

/* Linux-SNA APPC Call OPCODES */
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

/* Linux-SNA APPC call data structures. */
struct sna_allocate {

}

struct sna_mc_allocate {

};

struct sna_deallocate {

};

struct sna_mc_deallocate {

};

struct sna_get_attributes {

};

struct sna_get_tp_properties {

};

struct sna_receive_and_wait {

};

struct sna_mc_receive_and_wait {

};

struct sna_request_to_send {

};

struct sna_mc_request_to_send {

};

struct sna_send_data {

};

struct sna_mc_send_data {

};

struct sna_confirm {

};

struct sna_mc_confirm {

};

struct sna_confirmed {

};

struct sna_mc_confirmed {

};

struct sna_send_error {

};

struct sna_mc_send_error {

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

};

struct sna_get_type {

};

struct sna_set_syncpt_options {

};

struct sna_flush {

};

struct sna_mc_flush {

};

struct sna_post_on_receipt {

};

struct sna_mc_post_on_receipt {

};

struct sna_prepare_to_receive {

};

struct sna_mc_prepare_to_receive {

};

struct sna_receive_expedited_data {

};

struct sna_mc_receive_expedited_data {

};

struct sna_receive_immediate {

};

struct sna_mc_receive_immediate {

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

struct sna_chang sna_appc {
	unsigned short	opcode;
	unsigned char	opext;
	unsigned short	rcpri;
	unsigned long	rcsec;

	union {
		struct sna_allocate		allocate;
		struct sna_mc_allocate		mc_allocate;
		struct sna_deallocate		deallocate;
		struct sna_mc_deallocate	mc_deallocate;
		struct sna_get_attributes	get_attributes;
		struct sna_get_tp_properties	get_tp_properties;
		struct sna_receive_and_wait	receive_and_wait;
		struct sna_mc_receive_and_wait	mc_receive_and_wait;
		struct sna_request_to_send	request_to_send;
		struct sna_mc_request_to_send	mc_request_to_send;
		struct sna_send_data		send_data;
		struct sna_mc_send_data		mc_send_data;
		struct sna_confirm		confirm;
		struct sna_mc_confirm		mc_confirm;
		struct sna_confirmed		confirmed;
		struct sna_mc_confirmed		mc_confirmed;
		struct sna_send_error		send_error;
		struct sna_mc_send_error	mc_send_error;
		struct sna_change_session_limit	change_session_limit;
		struct sna_init_session_limit	init_session_limit;
		struct sna_process_session_limit process_session_limit;
		struct sna_reset_session_limit	reset_session_limit;
		struct sna_mc_get_attributes	mc_get_attributes;
		struct sna_get_type		get_type;
		struct sna_set_syncpt_options	set_syncpt_options;
		struct sna_flush		flush;
		struct sna_mc_flush		mc_flush;
		struct sna_post_on_receipt	post_on_receipt;
		struct sna_mc_post_on_receipt	mc_post_on_receipt;
		struct sna_prepare_to_receive	prepare_to_receive;
		struct sna_mc_prepare_to_receive mc_prepare_to_receive;
		struct sna_receive_expedited_data receive_expedited_data;
		struct sna_receive_immediate	receive_immediate;
		struct sna_mc_receive_immediate	mc_receive_immediate;
		struct sna_send_expedited_data	send_expedited_data;
		struct sna_mc_send_expedited_data mc_send_expedited_data;
		struct sna_test			test;
		struct sna_mc_test		mc_test;
		struct sna_wait			wait;
		struct sna_wait_for_completion	wait_for_completion;
		struct sna_backout		backout;
		struct sna_prepare_for_syncpt	prepare_for_syncpt;
		struct sna_mc_prepare_for_syncpt mc_prepare_for_syncpt;
		struct sna_activate_session	activate_session;
		struct sna_deactivate_conversation_group deactive_conversation_group;
		struct sna_deactive_session	deactive_session;
		struct sna_define_local_lu	define_local_lu;
		struct sna_define_mode		define_mode;
		struct sna_define_remote_lu	define_remote_lu;
		struct sna_define_tp		define_tp;
		struct sna_delete		delete;
		struct sna_display_local_lu	display_local_lu;
		struct sna_display_mode		display_mode;
		struct sna_display_remote_lu	display_remote_lu;
		struct sna_display_tp		display_tp;
		struct sna_process_signoff	process_signoff;
		struct sna_signoff		signoff;
	} verb;
};

#endif /* _APPC_H */
