/* sna_errors.h: Linux-SNA Error codes and declarations.
 * - Kept in one place to ease maintance headaches
 *
 * Copyright (c) 1999-2002 by Jay Schulist <jschlst@linux-sna.org>
 *
 * This program can be redistributed or modified under the terms of the
 * GNU General Public License as published by the Free Software Foundation.
 * This program is distributed without any warranty or implied warranty
 * of merchantability or fitness for a particular purpose.
 *
 * See the GNU General Public License for more details.
 */

#ifndef __NET_SNA_ERRORS_H
#define __NET_SNA_ERRORS_H


#define	RESOURCE_FAILURE_NO_RETRY	1008600B

/* Allocation error codes */
#define TPN_NOT_RECOGNIZED		10086021
#define PIP_NOT_ALLOWED			10086031
#define PIP_NOT_SPECIFIED_CORRECTLY	10086032
#define CONVERSATION_TYPE_MISMATCH	10086034
#define SYNC_LEVEL_NOT_SUPPORTED_BY_PGM	10086041
#define ACCESS_DENIED			080F0983
#define SECURITY_NOT_VALID		080F6051
#define BACKED_OUT			08240000
#define TP_NOT_AVAIL_RETRY		084B6031
#define TP_NOT_AVAIL_NO_RETRY		084C0000
#define DEALLOCATE_ABEND_PROG		08640000
#define DEALLOCATE_ABEND_SVC		08640001
#define DEALLOCATE_ABEND_TIMER		08640002
#define PROG_ERROR_NO_TRUNC		08890000
#define PROG_ERROR_PURGING		08890000
#define PROG_ERROR_TRUNC		08890001
#define SVC_ERROR_NO_TRUNC		08890100
#define SVC_ERROR_PURGING		08890100
#define SVC_ERROR_TRUNC			08890101

#endif	/* __NET_SNA_ERRORS_H */
