/* libaname - APPC aname library for Linux-SNA
 *
 * Author: Michael Madore <mmadore@turbolinux.com>
 *
 * Copyright (C) 2000 TurboLinux, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "anameapi.h"

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


