/* aftp - APPC file transfer program for Linux-SNA
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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <dirent.h>
#if defined (__USE_READLINE__)
#include <readline/readline.h>
#include <readline/history.h>
#endif

#if HAVE_SNA_CMC_H
#include <sna/cmc.h>
#include <port.h>
#else
#include <linux/cpic.h>
#endif

#include <appfftp.h>
#include <wildcard.h>
#include <asuitecommon.h>

#define DEFAULT_BUFFER_SIZE 40
#define DEFAULT_ARG_LIST_SIZE 5
#define DEFAULT_ARG_SIZE 25
#define COMMAND_STRING_SIZE 80

#define AFTP_CLIENT_VERSION "0.03"

#define AFTP_OPSYS          "Linux"

#define A_ACTION_YES  0
#define A_ACTION_NO   1
#define A_ACTION_GO   2
#define A_ACTION_QUIT 3

#define A_QMARK       0
#define A_ALLOC       1
#define A_ASC         2
#define A_ASCII       3
#define A_BIN         4
#define A_BINARY      5
#define A_BLOCK       6
#define A_BYE         7
#define A_CD          8
#define A_CLOSE       9
#define A_DATE       10
#define A_DEL        11
#define A_DELETE     12
#define A_DIR        13
#define A_DISC       14 
#define A_DISCONNECT 15
#define A_EXIT       16
#define A_GET        17
#define A_HELP       18
#define A_LCD        19
#define A_LPWD       20
#define A_LRECL      21
#define A_LS         22 
#define A_LSD        23
#define A_MD         24
#define A_MKDIR      25
#define A_MODENAME   26
#define A_OPEN       27
#define A_PROMPT     28
#define A_PUT        29
#define A_PWD        30
#define A_QUIT       31
#define A_RD         32
#define A_RECEIVE    33
#define A_RECFM      34
#define A_RECV       35
#define A_REN        36
#define A_RENAME     37
#define A_RMDIR      38
#define A_SEND       39
#define A_STAT       40
#define A_STATUS     41
#define A_SYS        42
#define A_SYSTEM     43
#define A_TPNAME     44
#define A_TRACE      45
#define A_TYPE       46
#define A_VERSION    47

struct _command_strings {
  char command_text[COMMAND_STRING_SIZE+1];
  int command_num;
};

typedef struct _command_strings command_strings; 

command_strings commands[] = {
  {"?", A_QMARK},
  {"alloc", A_ALLOC}, 
  {"asc", A_ASC},
  {"ascii", A_ASCII}, 
  {"bin", A_BIN},
  {"binary", A_BINARY},
  {"block", A_BLOCK},
  {"bye", A_BYE},
  {"cd", A_CD},
  {"close", A_CLOSE},
  {"date", A_DATE},
  {"del", A_DEL},
  {"delete", A_DELETE},
  {"dir", A_DIR},
  {"disc", A_DISC},
  {"disconnect", A_DISCONNECT},
  {"exit", A_EXIT},
  {"get", A_GET},
  {"help", A_HELP},
  {"lcd", A_LCD},
  {"lpwd", A_LPWD},
  {"lrecl", A_LRECL},
  {"ls", A_LS},
  {"lsd", A_LSD},
  {"md", A_MD},
  {"mkdir", A_MKDIR},
  {"modename", A_MODENAME},
  {"open", A_OPEN},
  {"prompt", A_PROMPT},
  {"put", A_PUT},
  {"pwd", A_PWD},
  {"quit", A_QUIT},
  {"rd", A_RD},
  {"receive", A_RECEIVE},
  {"recfm", A_RECFM},
  {"recv", A_RECV},
  {"ren", A_REN},
  {"rename", A_RENAME},
  {"rmdir", A_RMDIR},
  {"send", A_SEND},
  {"stat", A_STAT},
  {"status", A_STATUS},
  {"sys", A_SYS},
  {"system", A_SYSTEM},
  {"tpname", A_TPNAME},
  {"trace", A_TRACE},
  {"type", A_TYPE},
  {"version", A_VERSION},
  {"", -1}
};

command_strings formats[] = {
  {"0", AFTP_DEFAULT_RECORD_FORMAT},
  {"F", AFTP_F},
  {"FA", AFTP_FA},
  {"FB", AFTP_FB},
  {"FBA", AFTP_FBA},
  {"FBM", AFTP_FBM},
  {"FBS", AFTP_FBS},
  {"FBSA", AFTP_FBSA},
  {"FBSM", AFTP_FBSM},
  {"FM", AFTP_FM},
  {"U", AFTP_U},
  {"UA", AFTP_UA},
  {"UM", AFTP_UM},
  {"V", AFTP_V},
  {"VA", AFTP_VA},
  {"VB", AFTP_VB},
  {"VBA", AFTP_VBA},
  {"VBM", AFTP_VBM},
  {"VBS", AFTP_VBS},
  {"VBSA", AFTP_VBSA},
  {"VBSM", AFTP_VBSM},
  {"VM", AFTP_VM},
  {"VS", AFTP_VS},
  {"VSA", AFTP_VSA},
  {"VSM", AFTP_VSM}
};

char * help_strings[] = {
  "?\tPrint local help information",
  "alloc\tSets the allocation size for created files", 
  "asc\tChanges the data transmission type to ASCII",
  "ascii\tChanges the data transmission type to ASCII", 
  "bin\tChanges the data transmission type to binary",
  "binary\tChanges the data transmission type to binary",
  "block\tSets the block size for created files",
  "bye\tExits the AFTP environment",
  "cd\tSets the current directory on the server",
  "close\tCloses the current AFTP session",
  "date\tSets how file dates will be handles during transfers",
  "del\tDeletes a file on the server",
  "delete\tDeletes a file on the server",
  "dir\tProvides a listing of files an attributes on the server",
  "disc\tCloses the current AFTP session",
  "disconnect\tCloses the current AFTP session",
  "exit\tExits the AFTP environment",
  "get\tTransfers one or more files from the server",
  "help\tProvides a list of help topics, or help on a specific command",
  "lcd\tChanges the current directory on the local computer",
  "lpwd\tDisplay the current diretory on the local computer",
  "lrecl\tSets the logical record length",
  "ls\tDisplays a short directory listing (files only) from the server",
  "lsd\tDisplays a short directory list from the server containing only directories",
  "md\tCreate a new directory on the server computer",
  "mkdir\tCreate a new directory on the server computer",
  "modename\tSets the mode name used for file transfers",
  "open\tOpens a connection to an AFTP server",
  "prompt\tSets whether or not to use file by file confirmation",
  "put\tTransfers one or more files from the client to the server",
  "pwd\tDisplays the current working directory on the server",
  "quit\tExits from the AFTP environment",
  "rd\tRemoves a directory on the server",
  "receive\tTransfers one or more files from the server",
  "recfm\tSets the record format",
  "recv\tTransfers one or more files from the server",
  "ren\tRenames a file on the server",
  "rename\tRenames a file on the server",
  "rmdir\tRemoves a directory on the server",
  "send\tTransfers one or more files fromthe client to the server",
  "stat\tProvides information about the current session characteristics",
  "status\tProvides information about the current session characteristics",
  "sys\tProvides information about the AFTP server",
  "system\tProvides information about the AFTP server",
  "tpname\tSets the name of the transaction program on the server",
  "trace\tSets the level of trace data to be collected",
  "type\tSets the data transmission type or displays its current value",
  "version\tDisplays the version and maintainer information"
};

unsigned char * get_input(void);
GSList *parse_input(int * argcnt, char * line);
void free_args(int argcnt, GSList * arglist);
int compare(const void *arg1, const void *arg2);
int get_command(int argcnt, char *arg, command_strings * data);
void a_help(int argcnt, GSList * arglist);
void a_open(int argcnt, GSList * arglist);
void a_ls(int argcnt, GSList * arglist, int cmdcode, int attr);
void a_md(int argcnt, GSList * arglist);
void a_cd(int argcnt, GSList * arglist);
void a_type(int argcnt, GSList * arglist);
void a_system(int argcnt, GSList * arglist);
void a_status(int argcnt, GSList * arglist);
void a_rd(int argcnt, GSList * arglist);
void a_pwd(int argcnt, GSList * arglist);
void a_ren(int argcnt, GSList * arglist);
void a_alloc(int argcnt, GSList * arglist);
void a_block(int argcnt, GSList * arglist);
void a_close(int argcnt, GSList * arglist);
void a_date(int argcnt, GSList * arglist);
void a_del(int argcnt, GSList * arglist);
void a_lrecl(int argcnt, GSList * arglist);
void a_modename(int argcnt, GSList * arglist);
void a_tpname(int argcnt, GSList * arglist);
void a_prompt(int argcnt, GSList * arglist);
void a_recfm(int argcnt, GSList * arglist);
void a_get(int argcnt, GSList * arglist);
void a_put(int argcnt, GSList * arglist);
void a_lpwd(int argcnt, GSList * arglist);
void a_lcd(int argcnt, GSList * arglist);
void set_data_type(char * data_type);
void detail_help(int cmdcode);
int initialize(void);
int get_action(char * prompt, char * filename);














