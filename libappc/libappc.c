/* libappc.c: APPC call mapping for Linux-SNA
 *
 * Author:
 * Jay Schulist         <jschlst@turbolinux.com>
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

/* Notes:
 * - Full Duplex, Half Duples, and Mapped Conversation verbs are all
 *   accessed through the same function, but with different flags.
 */

#include <appc.h>

allocate()
{

}

confirm()
{

}

confirmed()
{

}

deallocate()
{

}

flush()
{

}

get_attributes()
{

}

post_on_receipt()
{

}

prepare_for_syncpt()
{

}

prepare_to_receive()
{

}

receive_and_wait()
{

}

receive_expedited_data()
{

}

receive_immediate()
{

}

request_to_send()
{

}

send_data()
{

}

send_error()
{

}

send_expedited_data()
{

}

test()
{

}

backout()
{

}

get_tp_properties()
{

}

get_type()
{

}

set_syncpt_options()
{

}

syncpt()
{

}

wait()
{

}

wait_for_completion()
{

}

change_session_limit()
{

}

initialize_session_limit()
{

}

reset_session_limit()
{

}

process_session_limit()
{

}

activate_session()
{

}

deactivate_conversation_group()
{

}

deactivate_session()
{

}

define_local_lu()
{

}

define_mode()
{

}

define_remote_lu()
{

}

define_tp()
{

}

delete()
{

}

display_local_lu()
{

}

display_mode()
{

}

display_remote_lu()
{

}

display_tp()
{

}

display_signed_on_list()
{

}

signoff()
{

}

process_signoff()
{

}

unsigned long appc(struct sna_appc *appc)
{
	switch(appc->opcode)
	{
		case (AC_ALLOCATE):
			break;

		case (AC_MC_ALLOCATE):
			break;

		case (AC_DEALLOCATE):
			break;

		case (AC_MC_DEALLOCATE):
			break;

		case (AC_GET_ATTRIBUTES):
			break;

		case (AC_GET_TP_PROPERTIES):
			break;

		case (AC_RECEIVE_AND_WAIT):
			break;

		case (AC_MC_RECEIVE_AND_WAIT):
			break;

		case (AC_REQUEST_TO_SEND):
			break;

		case (AC_MC_REQUEST_TO_SEND):
			break;

		case (AC_SEND_DATA):
			break;

		case (AC_MC_SEND_DATA):
			break;

		case (AC_CONFIRM):
			break;

		case (AC_MC_CONFIRM):
			break;

		case (AC_CONFIRMED):
			break;

		case (AC_MC_CONFIRMED):
			break;

		case (AC_SEND_ERROR):
			break;

		case (AC_MC_SEND_ERROR):
			break;

		case (AC_CHANGE_SESSION_LIMIT):
			break;

		case (AC_INIT_SESSION_LIMIT):
			break;

		case (AC_PROCESS_SESSION_LIMIT):
			break;

		case (AC_RESET_SESSION_LIMIT):
			break;

		case (AC_MC_GET_ATTRIBUTES):
			break;

		case (AC_GET_TYPE):
			break;

		case (AC_SET_SYNCPT_OPTIONS):
			break;

		case (AC_FLUSH):
			break;

		case (AC_MC_FLUSH):
			break;

		case (AC_POST_ON_RECEIPT):
			break;

		case (AC_MC_POST_ON_RECEIPT):
			break;

		case (AC_PREPARE_TO_RECEIVE):
			break;

		case (AC_MC_PREPARE_TO_RECEIVE):
			break;

		case (AC_RECEIVE_EXPEDITED_DATA):
			break;

		case (AC_RECEIVED_IMMEDIATE):
			break;

		case (AC_MC_RECEIVE_IMMEDIATE):
			break;

		case (AC_SEND_EXPEDITED_DATA):
			break;

		case (AC_MC_SEND_EXPEDITED_DATA):
			break;

		case (AC_TEST):
			break;

		case (AC_MC_TEST):
			break;

		case (AC_WAIT):
			break;

		case (AC_WAIT_FOR_COMPLETION):
			break;

		case (AC_BACKOUT):
			break;

		case (AC_PREPARE_FOR_SYNCPT):
			break;

		case (AC_MC_PREPARE_FOR_SYNCPT):
			break;

		case (AC_ACTIVATE_SESSION):
			break;

		case (AC_DEACTIVATE_CONVERSATION_GROUP):
			break;

		case (AC_DEACTIVATE_SESSION):
			break;

		case (AC_DEFINE_LOCAL_LU):
			break;

		case (AC_DEFINE_MODE):
			break;

		case (AC_DEFINE_REMOTE_LU):
			break;

		case (AC_DEFINE_TP):
			break;

		case (AC_DELETE):
			break;

		case (AC_DISPLAY_LOCAL_LU):
			break;

		case (AC_DISPLAY_MODE):
			break;

		case (AC_DISPLAY_REMOTE_LU):
			break;

		case (AC_DISPLAY_TP):
			break;

		case (AC_PROCESS_SIGNOFF):
			break;

		case (AC_SIGNOFF):
			break;

		default:
			return (-1);
	}

	return (0);
}
