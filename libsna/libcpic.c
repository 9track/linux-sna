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

#include <config.h>
#include <syscall_pic.h>
#include <stdio.h>
#include <stdlib.h>

#include <linux/unistd.h>
#include <linux/cpic.h>

/* Single CPI-C system call into the kernel. */
_syscall4_pic(void, cpicall, unsigned char CM_PTR, conversation_id,
        unsigned short, opcode, void *, uaddr, CM_RETURN_CODE CM_PTR, 
	return_code);

/*
 * CPI-C call wrappers.
 */

CM_ENTRY cmacci(unsigned char CM_PTR conversation_id,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpicall(conversation_id, CM_CMACCI, NULL, return_code);
}

CM_ENTRY cmaccp(unsigned char CM_PTR conversation_id,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	pid_t pid = getpid();
	cargo1(args, &pid);
	cpicall(conversation_id, CM_CMACCP, args, return_code);
}

CM_ENTRY cmallc(unsigned char CM_PTR conversation_id,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpicall(conversation_id, CM_CMALLC, NULL, return_code);
}

CM_ENTRY cmcanc(unsigned char CM_PTR conversation_id,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpicall(conversation_id, CM_CMCANC, NULL, return_code);
}

CM_ENTRY cmcfm(unsigned char CM_PTR conversation_id,
        CM_CONTROL_INFORMATION_RECEIVED CM_PTR control_information_received,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, control_information_received);
	cpicall(conversation_id, CM_CMCFM, args, return_code);
}

CM_ENTRY cmcfmd(unsigned char CM_PTR conversation_id,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpicall(conversation_id, CM_CMCFMD, NULL, return_code);
}

CM_ENTRY cmcnvi(unsigned char CM_PTR buffer,
        CM_INT32 CM_PTR buffer_length,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo2(args, buffer, buffer_length);
	cpicall(NULL, CM_CMCNVI, args, return_code);
}

CM_ENTRY cmcnvo(unsigned char CM_PTR buffer,
        CM_INT32 CM_PTR buffer_length,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo2(args, buffer, buffer_length);
	cpicall(NULL, CM_CMCNVO, args, return_code);
}

CM_ENTRY cmdeal(unsigned char CM_PTR conversation_id,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpicall(conversation_id, CM_CMDEAL, NULL, return_code);
}

CM_ENTRY cmdfde(unsigned char CM_PTR conversation_id,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpicall(conversation_id, CM_CMDFDE, NULL, return_code);
}

CM_ENTRY cmeaeq(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR ae_qualifier,
        CM_INT32 CM_PTR ae_qualifier_length,
        CM_AE_QUAL_OR_AP_TITLE_FORMAT CM_PTR ap_title_format,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args; 
	cargo3(args, ae_qualifier, ae_qualifier_length, ap_title_format);
	cpicall(conversation_id, CM_CMEAEQ, args, return_code);
}

CM_ENTRY cmeapt(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR ap_title,
        CM_INT32 CM_PTR ap_title_length,
        CM_AE_QUAL_OR_AP_TITLE_FORMAT CM_PTR ap_title_format,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo3(args, ap_title, ap_title_length, ap_title_format);
	cpicall(conversation_id, CM_CMEAPT, args, return_code);
}

CM_ENTRY cmeacn(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR application_context_name,
        CM_INT32 CM_PTR appl_context_name_length,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo2(args, application_context_name, appl_context_name_length);
	cpicall(conversation_id, CM_CMEACN, args, return_code);
}

CM_ENTRY cmecs(unsigned char CM_PTR conversation_id,
        CM_CONVERSATION_STATE CM_PTR conversation_state,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, conversation_state);
	cpicall(conversation_id, CM_CMECS, args, return_code);
}

CM_ENTRY cmect(unsigned char CM_PTR conversation_id,
        CM_CONVERSATION_TYPE CM_PTR conversation_type,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, conversation_type);
	cpicall(conversation_id, CM_CMECT, args, return_code);
}

CM_ENTRY cmectx(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR context_id,
        CM_INT32 CM_PTR context_id_length,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo2(args, context_id, context_id_length);
	cpicall(conversation_id, CM_CMECTX, args, return_code);
}

CM_ENTRY cmeid(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR initialization_data,
        CM_INT32 CM_PTR requested_length,
        CM_INT32 CM_PTR initialization_data_length,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo3(args, initialization_data, requested_length, 
		initialization_data_length);
	cpicall(conversation_id, CM_CMEID, args, return_code);
}

CM_ENTRY cmembs(CM_INT32 CM_PTR maximum_buffer_size,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, maximum_buffer_size);
	cpicall(NULL, CM_CMEMBS, args, return_code);
}

CM_ENTRY cmemn(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR mode_name,
        CM_INT32 CM_PTR mode_name_length,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo2(args, mode_name, mode_name_length);
	cpicall(conversation_id, CM_CMEMN, args, return_code);
}

CM_ENTRY cmepid(unsigned char CM_PTR conversation_id,
        CM_PARTNER_ID_TYPE CM_PTR partner_id_type,
        unsigned char CM_PTR partner_id,
        CM_INT32 CM_PTR requested_length,
        CM_INT32 CM_PTR partner_id_length,
        CM_PARTNER_ID_SCOPE CM_PTR partner_id_scope,
        CM_DIRECTORY_SYNTAX CM_PTR directory_syntax,
        CM_DIRECTORY_ENCODING CM_PTR directory_encoding,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo7(args, partner_id_type, partner_id, requested_length,
		partner_id_length, partner_id_scope, directory_syntax,
		directory_encoding);
	cpicall(conversation_id, CM_CMEPID, args, return_code);
}

CM_ENTRY cmepln(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR partner_lu_name,
        CM_INT32 CM_PTR partner_lu_name_length,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo2(args, partner_lu_name, partner_lu_name_length);
	cpicall(conversation_id, CM_CMEPLN, args, return_code);
}

CM_ENTRY cmesi(unsigned char CM_PTR conversation_id,
        CM_INT32 CM_PTR call_id,
        unsigned char CM_PTR buffer,
        CM_INT32 CM_PTR requested_length,
        CM_DATA_RECEIVED_TYPE CM_PTR data_received,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo4(args, call_id, buffer, requested_length, data_received);
	cpicall(conversation_id, CM_CMESI, args, return_code);
}

CM_ENTRY cmesl(unsigned char CM_PTR conversation_id,
        CM_SYNC_LEVEL CM_PTR sync_level,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, sync_level);
	cpicall(conversation_id, CM_CMESL, args, return_code);
}

CM_ENTRY cmesrm(unsigned char CM_PTR conversation_id,
        CM_SEND_RECEIVE_MODE CM_PTR send_receive_mode,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, send_receive_mode);
	cpicall(conversation_id, CM_CMESRM, args, return_code);
}

CM_ENTRY cmesui(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR user_id,
        CM_INT32 CM_PTR user_id_length,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo2(args, user_id, user_id_length);
	cpicall(conversation_id, CM_CMESUI, args, return_code);
}

CM_ENTRY cmetc(unsigned char CM_PTR conversation_id,
        CM_TRANSACTION_CONTROL CM_PTR transaction_control,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, transaction_control);
	cpicall(conversation_id, CM_CMETC, args, return_code);
}

CM_ENTRY cmetpn(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR tp_name,
        CM_INT32 CM_PTR tp_name_length,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo2(args, tp_name, tp_name_length);
	cpicall(conversation_id, CM_CMETPN, args, return_code);
}

CM_ENTRY cmflus(unsigned char CM_PTR conversation_id,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpicall(conversation_id, CM_CMFLUS, NULL, return_code);
}

CM_ENTRY cmincl(unsigned char CM_PTR conversation_id,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpicall(conversation_id, CM_CMINCL, NULL, return_code);
}

CM_ENTRY cminic(unsigned char CM_PTR conversation_id,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpicall(conversation_id, CM_CMINIC, NULL, return_code);
}

CM_ENTRY cminit(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR sym_dest_name,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
        cargo1(args, sym_dest_name);
	cpicall(conversation_id, CM_CMINIT, args, return_code);
}

CM_ENTRY cmprep(unsigned char CM_PTR conversation_id,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpicall(conversation_id, CM_CMPREP, NULL, return_code);
}

CM_ENTRY cmptr(unsigned char CM_PTR conversation_id,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpicall(conversation_id, CM_CMPTR, NULL, return_code);
}

CM_ENTRY cmrcv(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR buffer,
        CM_INT32 CM_PTR requested_length,
        CM_DATA_RECEIVED_TYPE CM_PTR data_received,
        CM_INT32 CM_PTR received_length,
        CM_STATUS_RECEIVED CM_PTR status_received,
        CM_CONTROL_INFORMATION_RECEIVED CM_PTR control_information_received,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo6(args, buffer, requested_length, data_received, received_length,
		status_received, control_information_received);
	cpicall(conversation_id, CM_CMRCV, args, return_code);
}

CM_ENTRY cmrcvx(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR buffer,
        CM_INT32 CM_PTR requested_length,
        CM_INT32 CM_PTR received_length,
        CM_CONTROL_INFORMATION_RECEIVED CM_PTR control_information_received,
        CM_RECEIVE_TYPE CM_PTR expedited_receive_type,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo5(args, buffer, requested_length, received_length,
		control_information_received, expedited_receive_type);
	cpicall(conversation_id, CM_CMRCVX, args, return_code);
}

CM_ENTRY cmrltp(unsigned char CM_PTR tp_name,
        CM_INT32 CM_PTR tp_name_length,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo2(args, tp_name, tp_name_length);
	cpicall(NULL, CM_CMRLTP, args, return_code);
}

CM_ENTRY cmrts(unsigned char CM_PTR conversation_id,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpicall(conversation_id, CM_CMRTS, NULL, return_code);
}

CM_ENTRY cmsaeq(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR ae_qualifier,
        CM_INT32 CM_PTR ae_qualifier_length,
        CM_AE_QUAL_OR_AP_TITLE_FORMAT CM_PTR ap_title_format,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo3(args, ae_qualifier, ae_qualifier_length, ap_title_format);
	cpicall(conversation_id, CM_CMSAEQ, args, return_code);
}

CM_ENTRY cmsac(unsigned char CM_PTR conversation_id,
        CM_ALLOCATE_CONFIRM_TYPE CM_PTR allocate_confirm,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, allocate_confirm);
	cpicall(conversation_id, CM_CMSAC, args, return_code);
}

CM_ENTRY cmsacn(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR application_context_name,
        CM_INT32 CM_PTR appl_context_name_length,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo2(args, application_context_name, appl_context_name_length);
	cpicall(conversation_id, CM_CMSACN, args, return_code);
}

CM_ENTRY cmsapt(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR ap_title,
        CM_INT32 CM_PTR ap_title_length,
        CM_AE_QUAL_OR_AP_TITLE_FORMAT CM_PTR ap_title_format,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo3(args, ap_title, ap_title_length, ap_title_format);
	cpicall(conversation_id, CM_CMSAPT, args, return_code);
}

CM_ENTRY cmsbt(unsigned char CM_PTR conversation_id,
        CM_BEGIN_TRANSACTION CM_PTR begin_transaction,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, begin_transaction);
	cpicall(conversation_id, CM_CMSBT, args, return_code);
}

CM_ENTRY cmscsp(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR password,
        CM_INT32 CM_PTR password_length,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo2(args, password, password_length);
	cpicall(conversation_id, CM_CMSCSP, args, return_code);
}

CM_ENTRY cmscst(unsigned char CM_PTR conversation_id,
        CM_CONVERSATION_SECURITY_TYPE CM_PTR conv_security_type,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, conv_security_type);
	cpicall(conversation_id, CM_CMSCST, args, return_code);
}

CM_ENTRY cmscsu(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR user_id,
        CM_INT32 CM_PTR user_id_length,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo2(args, user_id, user_id_length);
	cpicall(conversation_id, CM_CMSCSU, args, return_code);
}

CM_ENTRY cmsct(unsigned char CM_PTR conversation_id,
        CM_CONVERSATION_TYPE CM_PTR conversation_type,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, conversation_type);
	cpicall(conversation_id, CM_CMSCT, args, return_code);
}

CM_ENTRY cmscu(unsigned char CM_PTR conversation_id,
        CM_CONFIRMATION_URGENCY CM_PTR confirmation_urgency,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, confirmation_urgency);
	cpicall(conversation_id, CM_CMSCU, args, return_code);
}

CM_ENTRY cmsdt(unsigned char CM_PTR conversation_id,
        CM_DEALLOCATE_TYPE CM_PTR deallocate_type,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, deallocate_type);
	cpicall(conversation_id, CM_CMSDT, args, return_code);
}

CM_ENTRY cmsed(unsigned char CM_PTR conversation_id,
        CM_ERROR_DIRECTION CM_PTR error_direction,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, error_direction);
	cpicall(conversation_id, CM_CMSED, args, return_code);
}

CM_ENTRY cmsend(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR buffer,
        CM_INT32 CM_PTR send_length,
        CM_CONTROL_INFORMATION_RECEIVED CM_PTR control_information_received,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo3(args, buffer, send_length, control_information_received);
	cpicall(conversation_id, CM_CMSEND, args, return_code);
}

CM_ENTRY cmserr(unsigned char CM_PTR conversation_id,
        CM_CONTROL_INFORMATION_RECEIVED CM_PTR control_information_received,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, control_information_received);
	cpicall(conversation_id, CM_CMSERR, args, return_code);
}

CM_ENTRY cmsf(unsigned char CM_PTR conversation_id,
        CM_FILL CM_PTR fill,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, fill);
	cpicall(conversation_id, CM_CMSF, args, return_code);
}

CM_ENTRY cmsid(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR initialization_data,
        CM_INT32 CM_PTR init_data_length,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo2(args, initialization_data, init_data_length);
	cpicall(conversation_id, CM_CMSID, args, return_code);
}

CM_ENTRY cmsld(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR log_data,
        CM_INT32 CM_PTR log_data_length,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo2(args, log_data, log_data_length);
	cpicall(conversation_id, CM_CMSLD, args, return_code);
}

CM_ENTRY cmsltp(unsigned char CM_PTR tp_name,
        CM_INT32 CM_PTR tp_name_length,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo2(args, tp_name, tp_name_length);
	cpicall(NULL, CM_CMSLTP, args, return_code);
}

CM_ENTRY cmsmn(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR mode_name,
        CM_INT32 CM_PTR mode_name_length,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo2(args, mode_name, mode_name_length);
	cpicall(conversation_id, CM_CMSMN, args, return_code);
}

CM_ENTRY cmsndx(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR buffer,
        CM_INT32 CM_PTR send_length,
        CM_CONTROL_INFORMATION_RECEIVED CM_PTR control_information_received,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo3(args, buffer, send_length, control_information_received);
	cpicall(conversation_id, CM_CMSNDX, args, return_code);
}

CM_ENTRY cmspdp(unsigned char CM_PTR conversation_id,
        CM_PREPARE_DATA_PERMITTED_TYPE CM_PTR prepare_data_permitted,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, prepare_data_permitted);
	cpicall(conversation_id, CM_CMSPDP, args, return_code);
}

CM_ENTRY cmspid(unsigned char CM_PTR conversation_id,
        CM_PARTNER_ID_TYPE CM_PTR partner_id_type,
        unsigned char CM_PTR partner_id,
        CM_INT32 CM_PTR partner_id_length,
        CM_PARTNER_ID_SCOPE CM_PTR partner_id_scope,
        CM_DIRECTORY_SYNTAX CM_PTR directory_syntax,
        CM_DIRECTORY_ENCODING CM_PTR directory_encoding,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo6(args, partner_id_type, partner_id, partner_id_length,
		partner_id_scope, directory_syntax, directory_encoding);
	cpicall(conversation_id, CM_CMSPID, args, return_code);
}

CM_ENTRY cmspln(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR partner_lu_name,
        CM_INT32 CM_PTR partner_lu_name_length,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo2(args, partner_lu_name, partner_lu_name_length);
	cpicall(conversation_id, CM_CMSPLN, args, return_code);
}

CM_ENTRY cmspm(unsigned char CM_PTR conversation_id,
        CM_PROCESSING_MODE CM_PTR processing_mode,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, processing_mode);
	cpicall(conversation_id, CM_CMSPM, args, return_code);
}

CM_ENTRY cmsptr(unsigned char CM_PTR conversation_id,
        CM_PREPARE_TO_RECEIVE_TYPE CM_PTR prepare_to_receive_type,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, prepare_to_receive_type);
	cpicall(conversation_id, CM_CMSPTR, args, return_code);
}

CM_ENTRY cmsqcf(unsigned char CM_PTR conversation_id,
        CM_CONVERSATION_QUEUE CM_PTR conversation_queue,
        unsigned char CM_PTR callback_function,
        unsigned char CM_PTR callback_info,
        unsigned char CM_PTR user_field,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo4(args, conversation_queue, callback_function, callback_info,
		user_field);
	cpicall(conversation_id, CM_CMSQCF, args, return_code);
}

CM_ENTRY cmsqpm(unsigned char CM_PTR conversation_id,
        CM_CONVERSATION_QUEUE CM_PTR conversation_queue,
        CM_PROCESSING_MODE CM_PTR queue_processing_mode,
        unsigned char CM_PTR user_field,
        CM_INT32 CM_PTR ooid,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo4(args, conversation_queue, queue_processing_mode, user_field,
		ooid);
	cpicall(conversation_id, CM_CMSQPM, args, return_code);
}

CM_ENTRY cmsrc(unsigned char CM_PTR conversation_id,
        CM_RETURN_CONTROL CM_PTR return_control,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, return_control);
	cpicall(conversation_id, CM_CMSRC, args, return_code);
}

CM_ENTRY cmsrt(unsigned char CM_PTR conversation_id,
        CM_RECEIVE_TYPE CM_PTR receive_type,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo1(args, receive_type);
	cpicall(conversation_id, CM_CMSRT, args, return_code);
}

CM_ENTRY cmssl(unsigned char CM_PTR conversation_id,
        CM_SYNC_LEVEL CM_PTR sync_level,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
        cargo1(args, sync_level);
	cpicall(conversation_id, CM_CMSSL, args, return_code);
}

CM_ENTRY cmssrm(unsigned char CM_PTR conversation_id,
        CM_SEND_RECEIVE_MODE CM_PTR send_receive_mode,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
        cargo1(args, send_receive_mode);
	cpicall(conversation_id, CM_CMSSRM, args, return_code);
}

CM_ENTRY cmsst(unsigned char CM_PTR conversation_id,
        CM_SEND_TYPE CM_PTR send_type,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
        cargo1(args, send_type);
	cpicall(conversation_id, CM_CMSST, args, return_code);
}

CM_ENTRY cmstc(unsigned char CM_PTR conversation_id,
        CM_TRANSACTION_CONTROL CM_PTR transaction_control,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
        cargo1(args, transaction_control);
	cpicall(conversation_id, CM_CMSTC, args, return_code);
}

CM_ENTRY cmstpn(unsigned char CM_PTR conversation_id,
        unsigned char CM_PTR tp_name,
        CM_INT32 CM_PTR tp_name_length,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo2(args, tp_name, tp_name_length);
	cpicall(conversation_id, CM_CMSTPN, args, return_code);
}

CM_ENTRY cmtrts(unsigned char CM_PTR conversation_id,
        CM_CONTROL_INFORMATION_RECEIVED CM_PTR control_information_received,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
        cargo1(args, control_information_received);
	cpicall(conversation_id, CM_CMTRTS, args, return_code);
}

CM_ENTRY cmwait(unsigned char CM_PTR conversation_id,
        CM_RETURN_CODE CM_PTR conversation_ret_code,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
        cargo1(args, conversation_ret_code);
	cpicall(conversation_id, CM_CMWAIT, args, return_code);
}

CM_ENTRY cmwcmp(unsigned char CM_PTR ooid_list,
        CM_INT32 CM_PTR ooid_list_count,
        CM_INT32 CM_PTR timeout,
        unsigned char CM_PTR completed_op_index_list,
        CM_INT32 CM_PTR completed_op_count,
        unsigned char CM_PTR user_field_list,
        CM_RETURN_CODE CM_PTR return_code)
{
	cpic_args *args;
	cargo6(args, ooid_list, ooid_list_count, timeout, 
		completed_op_index_list, completed_op_count, user_field_list);
	cpicall(NULL, CM_CMWCMP, args, return_code);
}
