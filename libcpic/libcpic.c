/* libcpic.c: CPI-C System Calls.
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

#include <linux/unistd.h>
#include <cpic.h>

_syscall2(void, cmacci, unsigned char CM_PTR, conversation_id,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall2(void, cmaccp, unsigned char CM_PTR, conversation_id,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall2(void, cmallc, unsigned char CM_PTR, conversation_id,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall2(void, cmcanc, unsigned char CM_PTR, conversation_id,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmcfm, unsigned char CM_PTR, conversation_id,
	CM_CONTROL_INFORMATION_RECEIVED CM_PTR, control_information_received,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall2(void, cmcfmd, unsigned char CM_PTR, conversation_id,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmcnvi, unsigned char CM_PTR, buffer,
	CM_INT32 CM_PTR, buffer_length,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmcnvo, unsigned char CM_PTR, buffer,
	CM_INT32 CM_PTR, buffer_length,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall2(void, cmdeal, unsigned char CM_PTR, conversation_id,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall2(void, cmdfde, unsigned char CM_PTR, conversation_id,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall5(void, cmeaeq, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, ae_qualifier,
	CM_INT32 CM_PTR, ae_qualifier_length,
	CM_AE_QUAL_OR_AP_TITLE_FORMAT CM_PTR, ap_title_format,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall5(void, cmeapt, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, ap_title,
	CM_INT32 CM_PTR, ap_title_length,
	CM_AE_QUAL_OR_AP_TITLE_FORMAT CM_PTR, ap_title_format,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall4(void, cmeacn, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, application_context_name,
	CM_INT32 CM_PTR, appl_context_name_length,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmecs, unsigned char CM_PTR, conversation_id,
	CM_CONVERSATION_STATE CM_PTR, conversation_state,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmect, unsigned char CM_PTR, conversation_id,
	CM_CONVERSATION_TYPE CM_PTR, conversation_type,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall4(void, cmectx, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, context_id,
	CM_INT32 CM_PTR, context_id_length,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall5(void, cmeid, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, initialization_data,
	CM_INT32 CM_PTR, requested_length,
	CM_INT32 CM_PTR, initialization_data_length,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall2(void, cmembs, CM_INT32 CM_PTR, maximum_buffer_size,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall4(void, cmemn, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, mode_name,
	CM_INT32 CM_PTR, mode_name_length,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall9(void, cmepid, unsigned char CM_PTR, conversation_id,
	CM_PARTNER_ID_TYPE CM_PTR, partner_id_type,
	unsigned char CM_PTR, partner_id,
	CM_INT32 CM_PTR, requested_length,
	CM_INT32 CM_PTR, partner_id_length,
	CM_PARTNER_ID_SCOPE CM_PTR, partner_id_scope,
	CM_DIRECTORY_SYNTAX CM_PTR, directory_syntax,
	CM_DIRECTORY_ENCODING CM_PTR, directory_encoding,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall4(void, cmepln, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, partner_lu_name,
	CM_INT32 CM_PTR, partner_lu_name_length,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall6(void, cmesi, unsigned char CM_PTR, conversation_id,
	CM_INT32 CM_PTR, call_id,
	unsigned char CM_PTR, buffer,
	CM_INT32 CM_PTR, requested_length,
	CM_DATA_RECEIVED_TYPE CM_PTR, data_received,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmesl, unsigned char CM_PTR, conversation_id,
	CM_SYNC_LEVEL CM_PTR, sync_level,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmesrm, unsigned char CM_PTR, conversation_id,
	CM_SEND_RECEIVE_MODE CM_PTR, send_receive_mode,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall4(void, cmesui, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, user_id,
	CM_INT32 CM_PTR, user_id_length,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmetc, unsigned char CM_PTR, conversation_id,
	CM_TRANSACTION_CONTROL CM_PTR, transaction_control,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall4(void, cmetpn, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, tp_name,
	CM_INT32 CM_PTR, tp_name_length,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall2(void, cmflus, unsigned char CM_PTR, conversation_id,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall2(void, cmincl, unsigned char CM_PTR, conversation_id,
        CM_RETURN_CODE CM_PTR, return_code);

_syscall2(void, cminic, unsigned char CM_PTR, conversation_id,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cminit, unsigned char CM_PTR, conversation_id, 
	unsigned char CM_PTR, sym_dest_name, 
	CM_RETURN_CODE CM_PTR, return_code);

_syscall2(void, cmprep, unsigned char CM_PTR, conversation_id,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall2(void, cmptr, unsigned char CM_PTR, conversation_id,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall8(void, cmrcv, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, bufer,
	CM_INT32 CM_PTR, requested_length,
	CM_DATA_RECEIVED_TYPE CM_PTR, data_received,
	CM_INT32 CM_PTR, received_length,
	CM_STATUS_RECEIVED CM_PTR, status_received,
	CM_CONTROL_INFORMATION_RECEIVED CM_PTR, control_information_received,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall7(void, cmrcvx, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, buffer,
	CM_INT32 CM_PTR, requested_length,
	CM_INT32 CM_PTR, received_length,
	CM_CONTROL_INFORMATION_RECEIVED CM_PTR, control_information_received,
	CM_RECEIVE_TYPE CM_PTR, expedited_receive_type,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmrltp, unsigned char CM_PTR, tp_name,
	CM_INT32 CM_PTR, tp_name_length,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall2(void, cmrts, unsigned char CM_PTR, conversation_id,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall5(void, cmsaeq, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, ae_qualifier,
	CM_INT32 CM_PTR, ae_qualifier_length,
	CM_AE_QUAL_OR_AP_TITLE_FORMAT CM_PTR, ap_title_format,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmsac, unsigned char CM_PTR, conversation_id,
	CM_ALLOCATE_CONFIRM_TYPE CM_PTR, allocate_confirm,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall4(void, cmsacn, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, application_context_name,
	CM_INT32 CM_PTR, appl_context_name_length,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall5(void, cmsapt, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, ap_title,
	CM_INT32 CM_PTR, ap_title_length,
	CM_AE_QUAL_OR_AP_TITLE_FORMAT CM_PTR, ap_title_format,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmsbt, unsigned char CM_PTR, conversation_id,
	CM_BEGIN_TRANSACTION CM_PTR, begin_transaction,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall4(void, cmscsp, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, password,
	CM_INT32 CM_PTR, password_length,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmscst, unsigned char CM_PTR, conversation_id,
	CM_CONVERSATION_SECURITY_TYPE CM_PTR, conv_security_type,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall4(void, cmscsu, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, user_id,
	CM_INT32 CM_PTR, user_id_length,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmsct, unsigned char CM_PTR, conversation_id,
	CM_CONVERSATION_TYPE CM_PTR, conversation_type,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmscu, unsigned char CM_PTR, conversation_id,
	CM_CONFIRMATION_URGENCY CM_PTR, confirmation_urgency,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmsdt, unsigned char CM_PTR, conversation_id,
	CM_DEALLOCATE_TYPE CM_PTR, deallocate_type,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmsed, unsigned char CM_PTR, conversation_id,
	CM_ERROR_DIRECTION CM_PTR, error_direction,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall5(void, cmsend, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, buffer,
	CM_INT32 CM_PTR, send_length,
	CM_CONTROL_INFORMATION_RECEIVED CM_PTR, control_information_received,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmserr, unsigned char CM_PTR, conversation_id,
	CM_CONTROL_INFORMATION_RECEIVED CM_PTR, control_information_recevied,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmsf, unsigned char CM_PTR, conversation_id,
	CM_FILL CM_PTR, fill,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall4(void, cmsid, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, initialization_data,
	CM_INT32 CM_PTR, init_data_length,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall4(void, cmsld, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, log_data,
	CM_INT32 CM_PTR, log_data_length,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmsltp, unsigned char CM_PTR, tp_name,
	CM_INT32 CM_PTR, tp_name_length,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall4(void, cmsmn, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, mode_name,
	CM_INT32 CM_PTR, mode_name_length,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall5(void, cmsndx, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, buffer,
	CM_INT32 CM_PTR, send_length,
	CM_CONTROL_INFORMATION_RECEIVED CM_PTR, control_information_received,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmspdp, unsigned char CM_PTR, conversation_id,
	CM_PREPARE_DATA_PERMITTED_TYPE CM_PTR, prepare_data_permitted,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall8(void, cmspid, unsigned char CM_PTR, conversation_id,
	CM_PARTNER_ID_TYPE CM_PTR, partner_id_type,
	unsigned char CM_PTR, partner_id,
	CM_INT32 CM_PTR, partner_id_length,
	CM_PARTNER_ID_SCOPE CM_PTR, partner_id_scope,
	CM_DIRECTORY_SYNTAX CM_PTR, directory_syntax,
	CM_DIRECTORY_ENCODING CM_PTR, directory_encoding,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall4(void, cmspln, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, partner_lu_name,
	CM_INT32 CM_PTR, partner_lu_name_length,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmspm, unsigned char CM_PTR, conversation_id,
	CM_PROCESSING_MODE CM_PTR, processing_mode,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmsptr, unsigned char CM_PTR, conversation_id,
	CM_PREPARE_TO_RECEIVE_TYPE CM_PTR, prepare_to_receive_type,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall6(void, cmsqcf, unsigned char CM_PTR, conversation_id,
	CM_CONVERSATION_QUEUE CM_PTR, conversation_queue,
	unsigned char CM_PTR, callback_function,
	unsigned char CM_PTR, callback_info,
	unsigned char CM_PTR, user_field,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall6(void, cmsqpm, unsigned char CM_PTR, conversation_id,
	CM_CONVERSATION_QUEUE CM_PTR, conversation_queue,
	CM_PROCESSING_MODE CM_PTR, queue_processing_mode,
	unsigned char CM_PTR, user_field,
	CM_INT32 CM_PTR, ooid,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmsrc, unsigned char CM_PTR, conversation_id,
	CM_RETURN_CONTROL CM_PTR, return_control,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmsrt, unsigned char CM_PTR, conversation_id,
	CM_RECEIVE_TYPE CM_PTR, receive_type,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmssl, unsigned char CM_PTR, conversation_id,
	CM_SYNC_LEVEL CM_PTR, sync_level,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmssrm, unsigned char CM_PTR, conversation_id,
	CM_SEND_RECEIVE_MODE CM_PTR, send_receive_mode,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmsst, unsigned char CM_PTR, conversation_id,
	CM_SEND_TYPE CM_PTR, send_type,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmstc, unsigned char CM_PTR, conversation_id,
	CM_TRANSACTION_CONTROL CM_PTR, transaction_control,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall4(void, cmstpn, unsigned char CM_PTR, conversation_id,
	unsigned char CM_PTR, tp_name,
	CM_INT32 CM_PTR, tp_name_length,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmtrts, unsigned char CM_PTR, conversation_id,
	CM_CONTROL_INFORMATION_RECEIVED CM_PTR, control_information_received,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall3(void, cmwait, unsigned char CM_PTR, conversation_id,
	CM_RETURN_CODE CM_PTR, conversation_ret_code,
	CM_RETURN_CODE CM_PTR, return_code);

_syscall7(void, cmwcmp, unsigned char CM_PTR, ooid_list,
	CM_INT32 CM_PTR, ooid_list_count,
	CM_INT32 CM_PTR, timeout,
	unsigned char CM_PTR, completed_op_index_list,
	CM_INT32 CM_PTR, completed_op_count,
	unsigned char CM_PTR, user_field_list,
	CM_RETURN_CODE CM_PTR, return_code);
