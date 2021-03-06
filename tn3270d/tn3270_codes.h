/* codes3270.h:
 *
 * Original author, Michael Madore <mmadore@turbolinux.com>
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

#define TN3270_SESSION_CTL_RESET_MDT        	0x01
#define TN3270_SESSION_CTL_KEYBOARD_RESTORE 	0x02
#define TN3270_SESSION_CTL_RESET            	0x40

#define CMD_3270_WRITE         			0x01
#define CMD_3270_ERASE_WRITE   			0x05
#define CMD_3270_READ_MODIFIED 			0x06

#define ORDER_3270_SF 				0x1d

/* 3270E Data Types */
#define TN3270E_TYPE_3270_DATA    		0x00
#define TN3270E_TYPE_SCS_DATA     		0x01
#define TN3270E_TYPE_RESPONSE     		0x02
#define TN3270E_TYPE_BIND_IMAGE   		0x03
#define TN3270E_TYPE_UNBIND       		0x04
#define TN3270E_TYPE_NVT_DATA     		0x05
#define TN3270E_TYPE_REQUEST      		0x06
#define TN3270E_TYPE_SSCP_LU_DATA 		0x07
#define TN3270E_TYPE_PRINT_EOJ    		0x08

/* Request flag values */
#define TN3270E_ERR_COND_CLEARED  		0x00

/* Response flag values */
#define TN3270E_NO_RESPONSE       		0x00
#define TN3270E_ERROR_RESPONSE    		0x01
#define TN3270E_ALWAYS_RESPONSE   		0x02
#define TN3270E_POSITIVE_RESPONSE 		0x00
#define TN3270E_NEGATIVE_RESPONSE 		0x01

/* Function options - bitwise values for internal use */
#define TN3270E_BIND_IMAGE      		1
#define TN3270E_DATA_STREAM_CTL 		2
#define TN3270E_RESPONSES       		4
#define TN3270E_SCS_CTL_CODES   		8
#define TN3270E_SYSREQ          		16

#define TN3270E_CLIENT_SUCCESS           	0x00
#define TN3270E_CLIENT_INVALID_COMMAND   	0x00
#define TN3270E_CLIENT_PRINTER_NOT_READY 	0x01
#define TN3270E_CLIENT_ILLEGAL_ADDRESS   	0x02
#define TN3270E_CLIENT_PRINTER_OFF       	0x03

#define TN3270E_SERVER_INVALID_COMMAND   	0x10030000
#define TN3270E_SERVER_PRINTER_NOT_READY 	0x08020000
#define TN3270E_SERVER_ILLEGAL_ADDRESS   	0x10050000
#define TN3270E_SERVER_PRINTER_OFF       	0x08310000
