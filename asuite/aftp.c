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

#include <config.h>
#include "aftp.h"
#include <version.h>

AFTP_HANDLE_TYPE session;
int connected = 0;
int commandcnt;
int prompting = 0;

int
main(void)
{

  unsigned char * input_line;
  int done;
  int argcnt;
  int command;
  int rc;
  AFTP_RETURN_CODE_TYPE ftp_rc;
  GSList * argarray;

  rc = initialize();

  if(rc != 0) 
    return(1);

  done = 0;
  while(!done) 
    {
      input_line = get_input(); 
      
      if(input_line == NULL) 
	return(EXIT_FAILURE);    

      argarray = parse_input(&argcnt, input_line);

      if(argcnt > 0) {
	command = get_command(argcnt, argarray->data, commands);
	    
	switch(command) 
	  {
	  case A_QUIT:
	  case A_BYE:
	  case A_EXIT:
	    done = 1;
	    break;
	  case A_QMARK:
	  case A_HELP:
	    a_help(argcnt, argarray); 
	    break;
	  case A_OPEN:
	    a_open(argcnt, argarray);
	    break;
	  case A_LS:
	    a_ls(argcnt, argarray, A_LS, 0);
	    break;
	  case A_LSD:
	    a_ls(argcnt, argarray, A_LSD, 0);
	    break;
	  case A_MKDIR:
	  case A_MD:
	    a_md(argcnt, argarray);
	    break;
	  case A_CD:
	    a_cd(argcnt, argarray);
	    break;
	  case A_TYPE:
	    a_type(argcnt, argarray);
	    break;
	  case A_BIN:
	  case A_BINARY:
	    set_data_type("binary");
	    break;
	  case A_ASC:
	  case A_ASCII:
	    set_data_type("ascii");
	    break;
	  case A_SYS:
	  case A_SYSTEM:
	    a_system(argcnt, argarray);
	    break;
	  case A_STAT:
	  case A_STATUS:
	    a_status(argcnt, argarray);
	    break;
	  case A_RD:
	  case A_RMDIR:
	    a_rd(argcnt, argarray);
	    break;
	  case A_PWD:
	    a_pwd(argcnt, argarray);
	    break;
	  case A_REN:
	  case A_RENAME:
	    a_ren(argcnt, argarray);
	    break;
	  case A_ALLOC:
	    a_alloc(argcnt, argarray);
	    break;
	  case A_BLOCK:
	    a_block(argcnt, argarray);
	    break;
	  case A_CLOSE:
	  case A_DISC:
	  case A_DISCONNECT:
	    a_close(argcnt, argarray);
	    break;
	  case A_DATE:
	    a_date(argcnt, argarray);
	    break;
	  case A_DEL:
	  case A_DELETE:
	    a_del(argcnt, argarray);
	    break;
	  case A_LRECL:
	    a_lrecl(argcnt, argarray);
	    break;
	  case A_DIR:
	    a_ls(argcnt, argarray, A_LS, 1);
	    break;
	  case A_MODENAME:
	    a_modename(argcnt, argarray);
	    break;
	  case A_PROMPT:
	    a_prompt(argcnt, argarray);
	    break;
	  case A_RECFM:
	    a_recfm(argcnt, argarray);
	    break;
	  case A_TPNAME:
	    a_tpname(argcnt, argarray);
	    break;
	  case A_PUT:
	  case A_SEND:
	    a_put(argcnt, argarray);
	    break;
	  case A_GET:
	  case A_RECV:
	  case A_RECEIVE:
	    a_get(argcnt, argarray);
	    break;
	  case A_LPWD:
	    a_lpwd(argcnt, argarray);
	    break;
	  case A_LCD:
	    a_lcd(argcnt, argarray);
	    break;
	  case A_TRACE:
	    break;
	  case A_VERSION:
	    printf("aftp v%s\n%s\n", ToolsVersion, ToolsMaintain);
	    break;
	  default:
	    printf("Unknown command %s\n", (char *)argarray->data);
	}
      }
      if(argcnt > 0)
	{
	  free_single_list(argarray); 
	}
      g_free(input_line);
    }
  if(connected) {
    aftp_close(session, &ftp_rc);
  }

  return(0);
}

/*
  Obtains a line of input from the user.  Uses readline if it's available to 
  allow history and enhanced editing.  Otherwise it reads characters from the 
  user until a newline is entered.  In both cases memory is allocated 
  dynamically so the caller is responsible for calling free.
*/
unsigned char *
get_input(void)
{
  unsigned char *line;  
#ifdef __USE_READLINE__
  line = readline("aftp> ");
  if (!line) return NULL;
  if (line[0]) 
    add_history(line);

#else
    unsigned char curch;
    int curpos;
    int maxbuf;

    printf("aftp> ");
    fflush(stdin);

    line = g_malloc(DEFAULT_BUFFER_SIZE);

    if(line == NULL) 
      return NULL;

    curpos = 0;
    maxbuf = DEFAULT_BUFFER_SIZE;

    while( (curch = fgetc(stdin)) != '\n')
      {
	if(curpos < maxbuf)
	  {
	    line[curpos] = curch;
	    curpos++;
	  } else {
	    unsigned char *newbuf;
	    newbuf = realloc(line, curpos+DEFAULT_BUFFER_SIZE);
	    if(newbuf == NULL)
	      {
		g_free(line);
		return(NULL);
	      }
	    maxbuf = curpos+DEFAULT_BUFFER_SIZE;
	    line = newbuf;
	    line[curpos] = curch;
	    curpos++;
	  }
      }
    line[curpos] = '\0';
#endif
    return line;
}

/*
  Parses line into individual arguments.  The breaking is done at whitespace.
  The arguments are returned in a dynamically allocated array of variable 
  length strings.  free_args needs to be called by the caller when the array 
  is no longer needed.
*/
GSList * 
parse_input(int * argcnt, char * line)
{
  unsigned char * curpos = line;
  char * curarg;
  int argpos;
  int maxpos;
  GSList * argarray;

  *argcnt = 0;

  argarray = NULL;

  while(*curpos)
    {
      while(isspace(*curpos))
	{
	  curpos++;
	}
      argpos = 0;
      maxpos = DEFAULT_ARG_SIZE; 

      curarg = g_malloc(DEFAULT_ARG_SIZE+1);

      if(curarg == NULL)
	{
	  return NULL;
	}

      while(*curpos && !isspace(*curpos) )
	{
	  if(argpos < maxpos)
	    {
	      curarg[argpos] = *curpos;
	    } else {
	      unsigned char *newbuf;
	      newbuf = realloc(curarg, argpos+DEFAULT_ARG_SIZE);
	      if(newbuf == NULL)
		{
		  g_free(curarg);
		  return NULL;
		}
	      curarg = newbuf;
	      curarg[argpos] = *curpos;
	      maxpos = maxpos + DEFAULT_ARG_SIZE;
	    }
	  argpos++;
	  curpos++;
	}
      curarg[argpos] = '\0';

      if(strlen(curarg) > 0 ) {
	argarray = g_slist_append(argarray, curarg);
	*argcnt = *argcnt + 1;
      }
    }

  return(argarray);

}

/*
  Comparison function used in the get_command function.
*/
int compare(const void *arg1, const void *arg2)
{
  int rc;
  command_strings * cmds;

  cmds = (command_strings *)arg1;

  rc = strcmp( cmds->command_text, (char *)arg2 );

  return(rc);
    
}

/*
  Returns the integer value associated with a command string.  Returns -1
  if the command is not valid.
*/
int get_command(int argcnt, char *arg, command_strings * data)
{
  command_strings * rc;

  rc = (command_strings *)bsearch(arg, data, commandcnt, 
				  sizeof(command_strings), compare);

  if(rc != NULL)
    return(rc->command_num);
  else
    return(-1);
 
}

/*
  When invoked with 0 arguments, the functions will display a list of all 
  available aftp commands.
*/
void 
a_help(int argcnt, GSList * arglist)
{

  int curcmd = 0;
  int nl = 0;

  if(argcnt == 1) {
    
    while(commands[curcmd].command_num >= 0) 
      {
	printf("%s", commands[curcmd].command_text); 
	
	if(nl == 4) {
	  printf("\n");
	  nl = 0;
	}
	else {
	  printf("\t");
	  if(strlen(commands[curcmd].command_text) < 8)
	    printf("\t");
	  nl++;
	}
	curcmd++;

      }
    printf("\n");

  } else if(argcnt == 2) {
    curcmd = A_HELP;
    if(curcmd >= 0) {
      detail_help(curcmd);
    } else {
      printf("Unknown command %s.\n", (char *)g_slist_nth_data(arglist,1) );
    }
  } else {
    detail_help(A_HELP);
  }
}

/*
  Create the initial FTP session and perform other setup.
*/
int
initialize(void)
{
  AFTP_RETURN_CODE_TYPE rc;

  /* 
     Doesn't make sense to expand local filename when we are connecting to a 
     remote system.
  */
  #if defined(__USE_READLINE__)
  rl_unbind_key(TAB);
  #endif

  /*
    Dynamically measure the number of commands supported.
  */
  commandcnt = 0;
  while(commands[commandcnt].command_num >= 0) {
    commandcnt++;
  }

  aftp_create(session, &rc);

  if(rc != AFTP_RC_OK)
    return(1);
  else
    return(0);
}

/*
  Connect the remote host.
*/
void 
a_open(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  char * dest;
  
  if(connected) {
    printf("Already connected\n");
    return;
  }

  if(argcnt != 2) {
    detail_help(A_OPEN);
  } else {
    dest = g_slist_nth_data(arglist, 1);
    aftp_set_destination(session, dest, strlen(dest), &rc);
    if(rc != AFTP_RC_OK) {
      printf("Could not connect to destination: %s\n", dest);
    } else {
      aftp_connect(session, &rc);
      if(rc != AFTP_RC_OK) {
	printf("Could not connect to destination: %s %lu\n",dest, rc);
      } else {
	printf("Connected to %s\n", dest);
	connected = 1;
      }
    }
  }
}

/*
  List all files, or only directories based on the value of cmdcode.
*/
void
a_ls(int argcnt, GSList * arglist, int cmdcode, int attr)
{
  AFTP_RETURN_CODE_TYPE rc;
  unsigned char path[513];
  AFTP_LENGTH_TYPE returned_length;
  AFTP_BOOLEAN_TYPE no_more_entries;
  unsigned char dir_entry[81];
  unsigned char AFTP_PTR file_spec = "*";
  AFTP_FILE_TYPE_TYPE file_type;
  AFTP_INFO_LEVEL_TYPE style;

  if(connected) {

    if(argcnt == 2) {
      file_spec = g_slist_nth_data(arglist,1);
    }

    if(cmdcode == A_LS) {
      file_type = AFTP_ALL_FILES;
    } else {
      file_type = AFTP_DIRECTORY;
    }

    if(attr) {
      style = AFTP_NATIVE_ATTRIBUTES;
    } else {
      style = AFTP_NATIVE_FILENAMES;
    }

    aftp_dir_open(session, file_spec, strlen(file_spec), file_type, 
		  style, path, sizeof(path)-1, 
		  &returned_length, &rc);
    
    if(rc != AFTP_RC_OK) {
      printf("Directory listing failed\n");
      return;
    } else {
      path[returned_length] = '\0';

      printf("Directory for %s.\n", path);

      do {
	aftp_dir_read(session, dir_entry, sizeof(dir_entry)-1,
		      &returned_length, &no_more_entries, &rc);
           
	if (rc == AFTP_RC_OK && no_more_entries == 0) {
	  dir_entry[returned_length] = '\0';
	  printf("%s\n", dir_entry);
	}
      } while (rc == AFTP_RC_OK && no_more_entries == 0);

      printf("\nDirectory listing complete.\n");

      aftp_dir_close(session, &rc);
      if (rc != AFTP_RC_OK) {
	printf("Error closing directory\n");
      }
      
    }
  } else {
    printf("Not connected\n");
  }
}

/*
  Create a directory on the remote host.
*/
void
a_md(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  char * dir;
  
  if(connected) {
    if(argcnt != 2) {
      detail_help(A_MKDIR);
    } else {
      dir = g_slist_nth_data(arglist, 1);
      aftp_create_dir(session, dir, strlen(dir), &rc);
      if(rc != AFTP_RC_OK) {
	printf("Error creating directory.\n");
      } else {
	printf("Directory %s created.\n", dir);
      }
    }
  } else {
    printf("Not connected.\n");
  }
}

/*
  Change to the specified directory on the host.
*/
void
a_cd(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  char * dir;

  if(connected) {
    if(argcnt == 2) {
      dir = g_slist_nth_data(arglist, 1);
      aftp_change_dir(session, dir, strlen(dir), &rc);
      if(rc != AFTP_RC_OK) {
	printf("Could not change to directory %s\n", dir);
      }
    } else {
      detail_help(A_CD);
    }
  } else {
    printf("Not connected.\n");
  }
}

/*
  Sets the data type to the user provided value.
*/
void
a_type(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  AFTP_DATA_TYPE_TYPE data_type;
  unsigned char data_type_string[AFTP_DATA_TYPE_SIZE+1];
  AFTP_LENGTH_TYPE returned_length;

  if(argcnt == 1) {
    aftp_extract_data_type(session, &data_type, &rc);
    if(rc != AFTP_RC_OK) {
      printf("Could not get data type 1.\n");
    } else {
      aftp_get_data_type_string(data_type,
				data_type_string, AFTP_DATA_TYPE_SIZE, 
				&returned_length, &rc);

      if(rc != AFTP_RC_OK) {
	printf("Could not get data type 2.\n");
      } else {
	printf("data type: %s\n", data_type_string);
      }
    }

  } else if(argcnt == 2) {
    set_data_type( g_slist_nth_data(arglist,1) );
  } else {
    detail_help(A_TYPE);
  }
}

/*
  Sets the data type for file transfers.
*/
void
set_data_type(char * data_type)
{
  AFTP_RETURN_CODE_TYPE rc;
  AFTP_DATA_TYPE_TYPE type_code;
  unsigned char data_type_string[AFTP_DATA_TYPE_SIZE+1];
  AFTP_LENGTH_TYPE returned_length;

  if(!strcmp(data_type, "ascii")) {
    type_code = AFTP_ASCII;
  } else if(!strcmp(data_type, "default")) {
    type_code = AFTP_ASCII;
  } else if(!strcmp(data_type, "binary")) {
    type_code = AFTP_BINARY;
  } else {
    printf("Unknown data type %s\n", data_type);
    return;
  }

  aftp_set_data_type(session, type_code, &rc);
  
  if(rc != AFTP_RC_OK) {
    printf("Could not set data type.\n");
  } else {
    aftp_get_data_type_string(type_code,
			      data_type_string, AFTP_DATA_TYPE_SIZE, 
			      &returned_length, &rc);

    if(rc != AFTP_RC_OK) {
      printf("Could not get data type 2.\n");
    } else {
      printf("data type: %s\n", data_type_string);
    }

    printf("Data type set to %s\n", data_type_string);
  }
}

/*
  Retreives and displays information about the host the client is conneted
  to.
*/
void
a_system(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  AFTP_LENGTH_TYPE returned_length;
  unsigned char system_info[AFTP_SYSTEM_INFO_SIZE+1];

  if(connected) {
    aftp_query_system_info(session, system_info, sizeof(system_info)-1, 
			   &returned_length, &rc);

    if(rc != AFTP_RC_OK) {
      printf("Could not retrieve system information.\n");
    } else {
      printf("Server information.\n\n");
      printf("%s\n", system_info);
    }
  } else {
    printf("Not connected.\n");
  }
}

/*
  Displays the status of the client aftp session.
*/
void
a_status(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  AFTP_DATA_TYPE_TYPE data_type;
  unsigned char data_type_string[AFTP_DATA_TYPE_SIZE+1];
  AFTP_DATE_MODE_TYPE date_mode;
  unsigned char date_mode_string[AFTP_DATE_MODE_SIZE+1];
  AFTP_RECORD_FORMAT_TYPE record_format;
  unsigned char record_format_string[AFTP_RECORD_FORMAT_SIZE+1];
  AFTP_LENGTH_TYPE record_length;
  AFTP_LENGTH_TYPE block_size;
  AFTP_LENGTH_TYPE alloc_size;
  unsigned char mode_name[AFTP_MODE_NAME_SIZE+1] = "TEST";
  unsigned char tp_name[AFTP_TP_NAME_SIZE+1];
  AFTP_LENGTH_TYPE returned_length;

  aftp_extract_tp_name(session, tp_name, sizeof(tp_name)-1,
		       &returned_length, &rc);

  if(rc != AFTP_RC_OK) {
    goto errout;
  }

  aftp_extract_mode_name(session, mode_name, sizeof(mode_name)-1,
			 &returned_length, &rc);

  if(rc != AFTP_RC_OK) {
    goto errout;
  }

  aftp_extract_data_type(session, &data_type, &rc);

  if(rc != AFTP_RC_OK) {
    goto errout;
  }

  aftp_get_data_type_string(data_type, data_type_string, AFTP_DATA_TYPE_SIZE,
			    &returned_length, &rc);

  if(rc != AFTP_RC_OK) {
    goto errout;
  }

  aftp_extract_date_mode(session, &date_mode, &rc);

  if(rc != AFTP_RC_OK) {
    goto errout;
  }

  aftp_get_date_mode_string(date_mode, date_mode_string, AFTP_DATE_MODE_SIZE,
			    &returned_length, &rc);

  if(rc != AFTP_RC_OK) {
    goto errout;
  }

  aftp_extract_record_format(session, &record_format, &rc);

  if(rc != AFTP_RC_OK) {
    goto errout;
  }

  aftp_get_record_format_string(record_format, record_format_string, 
				AFTP_RECORD_FORMAT_SIZE, 
				&returned_length, &rc);

  if(rc != AFTP_RC_OK) {
    goto errout;
  }

  aftp_extract_record_length(session, &record_length, &rc);

  if(rc != AFTP_RC_OK) {
    goto errout;
  }

  aftp_extract_block_size(session, &block_size, &rc);

  if(rc != AFTP_RC_OK) {
    goto errout;
  }

  aftp_extract_allocation_size(session, &alloc_size, &rc);

  if(rc != AFTP_RC_OK) {
    goto errout;
  }

  printf("AFTP version is %s\n", AFTP_CLIENT_VERSION);
  printf("Operating system is %s\n\n", AFTP_OPSYS);
  printf("Data Type\t: %s\n", data_type_string);
  printf("Date Mode\t: %s\n", date_mode_string);
  printf("Record Format\t: %s\n", record_format_string); 
  printf("Record Length\t: %lu\n", record_length);
  printf("Block Size\t: %lu\n", block_size);
  printf("Allocation Size\t: %lu\n", alloc_size);
  printf("Mode Name\t: %s\n", mode_name);
  printf("TP Name\t\t: %s\n", tp_name);
  if(prompting) {
    printf("Prompting\t: ON\n");
  } else {
    printf("Prompting\t: OFF\n");
  }

  return;

 errout:
  printf("Fatal error!\n");
  exit(1);
}

/*
  Removes the specified directory on the server.
*/
void
a_rd(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  unsigned char error_str[512];
  AFTP_LENGTH_TYPE returned_length;
  char * dir;

  if(connected) {
    if(argcnt != 2) {
      detail_help(A_MD);
    } else {
      dir = g_slist_nth_data(arglist,1);
      aftp_remove_dir(session, dir, strlen(dir), &rc);
      if(rc != AFTP_RC_OK) {
	printf("Error removing directory.\n");
	aftp_format_error(session, AFTP_DETAIL_RC, error_str, 
			  sizeof(error_str)-1, &returned_length, &rc);
	if(rc != AFTP_RC_OK) {
	  printf("%s\n", error_str);
	}
      } else {
	printf("Directory %s removed.\n", dir);
      }
    }
  } else {
    printf("Not connected.\n");
  }
  
}

/*
  Displays the current working directory on the server.
*/
void
a_pwd(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  AFTP_LENGTH_TYPE returned_length;
  unsigned char working_dir[81];

  if(connected) {
    aftp_query_current_dir(session, working_dir, sizeof(working_dir)-1, 
			   &returned_length, &rc);

    if(rc != AFTP_RC_OK) {
      printf("Could not retrieve working directory.\n");
    } else {
      printf("%s is the server's current directory\n", working_dir);
    }
  } else {
    printf("Not connected.\n");
  }
  
}

/*
  Rename the specified file on the server.
*/
void
a_ren(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  char * oldname;
  char * newname;

  if(connected) {
    if(argcnt == 3) {
      oldname = g_slist_nth_data(arglist,1);
      newname = g_slist_nth_data(arglist,2);
      aftp_rename(session, oldname, strlen(oldname),
		  newname, strlen(newname), &rc);

      if(rc != AFTP_RC_OK) {
	printf("Could not rename file %s.\n", oldname);
      } else {
	printf("Renamed %s to %s.\n", oldname, newname);
      }
    } else {
      detail_help(A_REN);
    }
  } else {
    printf("Not connected.\n");
  }
  
}

/*
  Sets the allocation size for file transfers.  This command is probably only 
  useful when sending files to a system running MVS.
*/
void
a_alloc(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  AFTP_ALLOCATION_SIZE_TYPE alloc_size;
  char *invalid;

  if(connected) {
    if(argcnt == 1) {

      aftp_extract_allocation_size(session, &alloc_size, &rc);

      assert(rc == AFTP_RC_OK);

      printf("Allocation size\t: %lu\n", alloc_size);
      
    } else if(argcnt == 2) {

      alloc_size = strtol(g_slist_nth_data(arglist,1), &invalid, 10);

      if(*invalid) {
	printf("Invalid allocation size.\n");
	return;
      }

      aftp_set_allocation_size(session, alloc_size, &rc);

      if(rc != AFTP_RC_OK) {
	printf("Could not set allocation size.\n");
      } else {
	printf("Allocation size set to %lu\n", alloc_size);
      }
    } else {
      detail_help(A_ALLOC);
    }
  } else {
    printf("Not connected.\n");
  }
  
}

/*
  Sets the block size for file transfers.
*/
void
a_block(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  AFTP_BLOCK_SIZE_TYPE block_size;
  char *invalid;

  if(connected) {
    if(argcnt == 1) {

      aftp_extract_block_size(session, &block_size, &rc);

      assert(rc == AFTP_RC_OK);

      printf("Block size:\t %lu\n", block_size);

    } else if(argcnt == 2) {

      block_size = strtol(g_slist_nth_data(arglist,1), &invalid, 10);

      if(*invalid) {
	printf("Invalid block size.\n");
	return;
      }

      aftp_set_block_size(session, block_size, &rc);

      if(rc != AFTP_RC_OK) {
	printf("Could not set block size.\n");
      } else {
	printf("Block size set to %lu\n", block_size);
      }
    } else {
      detail_help(A_BLOCK);
    }
  } else {
    printf("Not connected.\n");
  }
  
}

/*
  Closes the connection to the server.
*/
void
a_close(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;

  if(connected) {
    aftp_close(session, &rc);

    if(rc != AFTP_RC_OK) {
      printf("Could not close connection.\n");
    } else {
      printf("Goodbye.\n");
      connected = 0;
    }
  } else {
    printf("Not connected.\n");
  }
}

/*
  Sets the logical record length  for file transfers.  This command only takes
  effect when files are stored on a computer that handles record oriented 
  files, such as VM or MVS.
*/
void
a_lrecl(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  AFTP_RECORD_LENGTH_TYPE record_length;
  char *invalid;

  if(connected) {
    if(argcnt == 1) {

      aftp_extract_record_length(session, &record_length, &rc);

      assert(rc == AFTP_RC_OK);

      printf("Record length\t: %lu\n", record_length);
      
    } else if(argcnt == 2) {

      record_length = strtol(g_slist_nth_data(arglist,1), &invalid, 10);

      if(*invalid) {
	printf("Invalid record length.\n");
	return;
      }

      aftp_set_record_length(session, record_length, &rc);

      if(rc != AFTP_RC_OK) {
	printf("Could not set record length.\n");
      } else {
	printf("Record length set to %lu\n", record_length);
      }
    } else {
      detail_help(A_LRECL);
    }
  } else {
    printf("Not connected.\n");
  }
  
}

/*
  Sets the date mode.
*/
void
a_date(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  AFTP_DATE_MODE_TYPE date_code;
  AFTP_DATE_MODE_TYPE date_mode;
  AFTP_LENGTH_TYPE returned_length;
  unsigned char date_mode_string[AFTP_DATE_MODE_SIZE+1];
  char * date;

  if(connected) {
    if(argcnt == 1) {
      
      aftp_extract_date_mode(session, &date_mode, &rc);
      
      assert(rc == AFTP_RC_OK);
  
      aftp_get_date_mode_string(date_mode, date_mode_string, 
				AFTP_DATE_MODE_SIZE, &returned_length, &rc);

      assert(rc == AFTP_RC_OK);

      printf("Date mode:\t %s\n", date_mode_string);

    } else if(argcnt == 2) {
      date = g_slist_nth_data(arglist,1);
      if(!strcmp(date, "new")) {
	date_code = AFTP_NEWDATE;
      } else if(!strcmp(date, "old")) {
	date_code = AFTP_OLDDATE;
      } else {
	printf("Unknown date mode %s\n", date);
	return;
      }

      aftp_set_date_mode(session, date_code, &rc);

      if(rc != AFTP_RC_OK) {
	printf("Could not set date mode.\n");
      } else {
	printf("Date mode set to %s\n", date);
      }

    } else {
      detail_help(A_DATE);
    }
  } else {
    printf("Not connected.\n");
  }
}

/*
  Deletes the specified file(s) from the server.
*/
void
a_del(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  GSList * filelist;
  unsigned char path[513];
  AFTP_LENGTH_TYPE returned_length;
  AFTP_BOOLEAN_TYPE no_more_entries;
  unsigned char dir_entry[81];
  GSList * index;
  int action;
  int prompt_user;
  char * filespec;

  if(connected) {
    if(argcnt == 2) {

      filelist = NULL;
     
      filespec = g_slist_nth_data(arglist,1);

      aftp_dir_open(session, filespec, strlen(filespec), AFTP_FILE, 
		    AFTP_NATIVE_FILENAMES, path, sizeof(path)-1, 
		    &returned_length, &rc);
    
      if(rc != AFTP_RC_OK) {
	printf("File list retrieval failed.\n");
	return;
      } 
      
      path[returned_length] = '\0';

      do {
	aftp_dir_read(session, dir_entry, sizeof(dir_entry)-1,
		      &returned_length, &no_more_entries, &rc);
           
	if (rc == AFTP_RC_OK && no_more_entries == 0) {
	  dir_entry[returned_length] = '\0';
	  filelist = g_slist_append(filelist, dir_entry);
	}
      } while (rc == AFTP_RC_OK && no_more_entries == 0);

      aftp_dir_close(session, &rc);
      if (rc != AFTP_RC_OK) {
	printf("Error closing directory\n");
      }

      prompt_user = prompting;

      index = filelist;

      while(index != NULL) {

	if(prompt_user) {
	  action = get_action("Delete", index->data);
	} else {
	  action = A_ACTION_YES;
	}

	switch(action) 
	  {
	  case A_ACTION_GO:
	    action = A_ACTION_YES;
	    prompt_user = 0;
	    break;
	  case A_ACTION_QUIT:
	    goto quit;
	    break;
	  }

	if(action == A_ACTION_YES) {
	  aftp_delete(session, index->data, 
		      strlen(index->data), &rc);
	  
	  if(rc != AFTP_RC_OK) {
	    printf("Error deleting %s\n", (char *)index->data);
	  }
	  index = g_slist_next(index);
	}
      }
    quit:

      free_single_list(filelist);

    } else {
      detail_help(A_DEL);
    }
  } else {
    printf("Not connected.\n");
  }

}

/*
  Sets the mode for the connection.
*/
void
a_modename(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  unsigned char mode_name[AFTP_MODE_NAME_SIZE+1];
  AFTP_LENGTH_TYPE returned_length;
  char * mode;

  if(!connected) {
    if(argcnt == 1) {

      aftp_extract_mode_name(session, mode_name, sizeof(mode_name)-1,
			     &returned_length, &rc);

      assert(rc == AFTP_RC_OK);

      printf("Mode name:\t %s\n", mode_name);
      
    } else if (argcnt == 2) {
      mode = g_slist_nth_data(arglist,1);
      aftp_set_mode_name(session, mode, strlen(mode), &rc);

      if(rc != AFTP_RC_OK) {
	printf("Could not set mode name.\n");
      } else {
	printf("Mode name set to %s.\n", mode);
      }
    } else {
      detail_help(A_MODENAME);
    }
  } else {
    printf("Cannot set mode while connected.\n");
  }
}

/*
  Sets the tpname for the connection.
*/
void
a_tpname(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  unsigned char tp_name[AFTP_TP_NAME_SIZE+1];
  AFTP_LENGTH_TYPE returned_length;
  char * tpname;

  if(!connected) {
    if(argcnt == 1) {

      aftp_extract_tp_name(session, tp_name, sizeof(tp_name)-1,
			     &returned_length, &rc);

      assert(rc == AFTP_RC_OK);

      printf("TP name:\t %s\n", tp_name);
      
    } else if (argcnt == 2) {
      tpname = g_slist_nth_data(arglist,1);
      aftp_set_tp_name(session, tpname, strlen(tpname), &rc);

      if(rc != AFTP_RC_OK) {
	printf("Could not set TP name.\n");
      } else {
	printf("TP name set to %s.\n", tpname);
      }
    } else {
      detail_help(A_TPNAME);
    }
  } else {
    printf("Cannot set TP name while connected.\n");
  }
}

/*
  Turns prompting on or off.  When prompting is on, the user is asked for 
  confirmation before each get, put or delete operation.
*/
void
a_prompt(int argcnt, GSList * arglist)
{
  char * prompt;

  if(connected) {
    if(argcnt == 1) {
      if(prompting) {
	printf("Prompting is ON\n");
      } else {
	printf("Prompting is OFF\n");
      }
    } else if (argcnt == 2) {
      prompt = g_slist_nth_data(arglist,1);
      if(!strcmp(prompt, "on")) {
	prompting = 1;
	printf("Prompting is ON\n");
      } else if(!strcmp(prompt, "off")) {
	prompting = 0;
	printf("Prompting is OFF\n");
      } else {
	detail_help(A_PROMPT);
      }
    } else {
      detail_help(A_PROMPT);
    }
  } else {
    printf("Not connected.\n");
  }
}

/*
  Sets the record format for file transfers.  This command only takes
  effect when files are stored on a computer that handles record oriented 
  files, such as VM or MVS.
*/
void
a_recfm(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  AFTP_RECORD_FORMAT_TYPE record_format;
  unsigned char record_format_string[AFTP_RECORD_FORMAT_SIZE+1];
  AFTP_LENGTH_TYPE returned_length;
  int format;
  char * recfm;

  if(connected) {
    if(argcnt == 1) {
      aftp_extract_record_format(session, &record_format, &rc);

      assert(rc == AFTP_RC_OK);

      aftp_get_record_format_string(record_format, record_format_string,
				    sizeof(record_format_string)-1, 
				    &returned_length, &rc);

      assert(rc == AFTP_RC_OK);

      printf("Record format\t: %s\n", record_format_string);

    } else if(argcnt == 2) {
      recfm = g_slist_nth_data(arglist,1);
      format = get_command(argcnt, recfm, formats);

      if(format >= 0) {
	aftp_set_record_format(session, format, &rc);

	if(rc == AFTP_RC_OK) {
	  printf("Record format set to %s\n", recfm);
	} else {
	  printf("Could not set record format.\n");
	}
      } else {
	printf("Invalid format.\n");
      }
    } else {
      detail_help(A_RECFM);
    }
  } else {
    printf("Not connected.\n");
  }
}

/*
  Transfer one or more files to the server.
*/
void
a_put(int argcnt, GSList * arglist)
{
  GSList * filelist;
  GSList * curelem;
  int prompt_user;
  AFTP_RETURN_CODE_TYPE rc;
  int action;
  char * directory;
  char * filename;
  char * filespec;
  struct stat filestats;

  if(connected) {
    if(argcnt == 2) {

      filespec = g_slist_nth_data(arglist,1);
      directory = g_dirname(filespec);
      filename = g_basename(filespec);

      if( !strcmp(directory, ".") ) {
	g_free(directory);
	directory = safe_get_cwd();
      }

      rc = lstat(filespec, &filestats);

      if(rc < 0) {
	rc = lstat(directory, &filestats);
	if(rc < 0) {
	  fprintf(stderr, "Invalid path.\n");
	  g_free(directory);
	  return;
	} else {
	  if( S_ISDIR(filestats.st_mode) ) {
	    rc = get_local_file_list(directory, filename, &filelist, 1);
	  } else {
	    fprintf(stderr, "Invalid path.\n");
	    g_free(directory);
	    return;
	  }
	}
      } else {
	if( S_ISDIR(filestats.st_mode) ) {
	  rc = get_local_file_list(filespec, "*", &filelist, 1);
	} else {
	  rc = get_local_file_list(directory, filename, &filelist, 1);
	}
      }
      
      if(filelist != NULL) {

	prompt_user = prompting;

	curelem = filelist;

	while(curelem != NULL) {

	  printf("curelem->data = %s\n", (char *)curelem->data);

	  if(prompt_user) {
	    action = get_action("Send", curelem->data);
	  } else {
	    action = A_ACTION_YES;
	  }

	  switch(action) 
	    {
	    case A_ACTION_GO:
	      action = A_ACTION_YES;
	      prompt_user = 0;
	      break;
	    case A_ACTION_QUIT:
	      goto quit;
	      break;
	    }

	  if(action == A_ACTION_YES) {
	    aftp_send_file(session, curelem->data, 
			   strlen(curelem->data), 
			   curelem->data,
			   strlen(curelem->data),&rc);
	  
	    if(rc != AFTP_RC_OK) {
	      printf("Error sending %s\n", (char *)curelem->data);
	    }
	  }
	  curelem = g_slist_next(curelem);
	}

      quit:

	g_slist_free(filelist);

      }
    } else {
      detail_help(A_PUT);
    }
  }
  else
    {
      printf("Not connected.\n");
    }
}

/*
  Receive one or more files from the server.
*/
void
a_get(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  GSList * filelist;
  unsigned char path[513];
  AFTP_LENGTH_TYPE returned_length;
  AFTP_BOOLEAN_TYPE no_more_entries;
  unsigned char dir_entry[81];
  GSList * index;
  int action;
  int prompt_user;
  char * filespec;

  if(connected) {
    if(argcnt == 2) {

      filelist = NULL;
     
      filespec = g_slist_nth_data(arglist,1);

      aftp_dir_open(session, filespec, strlen(filespec), AFTP_FILE, 
		    AFTP_NATIVE_FILENAMES, path, sizeof(path)-1, 
		    &returned_length, &rc);
    
      if(rc != AFTP_RC_OK) {
	printf("File list retrieval failed.\n");
	return;
      } 
      
      path[returned_length] = '\0';

      do {
	aftp_dir_read(session, dir_entry, sizeof(dir_entry)-1,
		      &returned_length, &no_more_entries, &rc);
           
	if (rc == AFTP_RC_OK && no_more_entries == 0) {
	  dir_entry[returned_length] = '\0';
	  filelist = g_slist_append(filelist, dir_entry);
	}
      } while (rc == AFTP_RC_OK && no_more_entries == 0);

      aftp_dir_close(session, &rc);
      if (rc != AFTP_RC_OK) {
	printf("Error closing directory\n");
      }

      prompt_user = prompting;

      index = filelist;

      while(index != NULL) {

	if(prompt_user) {
	  action = get_action("Receive", index->data);
	} else {
	  action = A_ACTION_YES;
	}

	switch(action) 
	  {
	  case A_ACTION_GO:
	    action = A_ACTION_YES;
	    prompt_user = 0;
	    break;
	  case A_ACTION_QUIT:
	    goto quit;
	    break;
	  }

	if(action == A_ACTION_YES) {
	  aftp_receive_file(session, index->data, 
			    strlen(index->data), 
			    index->data,
			    strlen(index->data),&rc);
	  
	  if(rc != AFTP_RC_OK) {
	    printf("Error receiving %s\n", (char *)index->data);
	  }
	}
	index = g_slist_next(index);
      }
    quit:

      free_single_list(filelist);

    } else {
      detail_help(A_GET);
    }
  } else {
    printf("Not connected.\n");
  }

}

/*
 */
void
a_lpwd(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  char currentdir[81];
  AFTP_LENGTH_TYPE returned_length;

  aftp_local_query_current_dir(session, currentdir, sizeof(currentdir)-1,
			       &returned_length, &rc);

  if(rc == AFTP_RC_OK) {
    printf("%s is the client's current directory.\n", currentdir);
  } else {
    printf("Could not retreive current working directory.\n");
  }
}

/*
  Changes the current working directory on the client.
 */
void
a_lcd(int argcnt, GSList * arglist)
{
  AFTP_RETURN_CODE_TYPE rc;
  char currentdir[81];
  AFTP_LENGTH_TYPE returned_length;
  char * filespec;

  if(argcnt == 1) {
    a_lpwd(argcnt, arglist);
  } else if(argcnt == 2) {
    filespec = g_slist_nth_data(arglist,1);
    aftp_local_change_dir(session, filespec, strlen(filespec), &rc);
    if(rc != AFTP_RC_OK) {
      printf("Could not change local working directory.\n");
    } else {
      aftp_local_query_current_dir(session, currentdir, sizeof(currentdir)-1,
				   &returned_length, &rc);
      if(rc == AFTP_RC_OK) {
	printf("%s is the client's current working directory.\n", currentdir);
      } else {
	printf("Error retrieving current working directory.\n");
      }
    }
  } else {
    detail_help(A_LCD);
  }
}

/*
  Displays detailed help for the specified command
*/
void 
detail_help(int cmdcode)
{
  if(cmdcode < commandcnt) {
    printf("%s\n", help_strings[cmdcode]);
  } else {
    printf("No help available\n");
  }
}

/*
  Prompts the user for the action to take.  Returns a numeric action code.
 */
int
get_action(char * prompt, char * filename)
{
  unsigned char action;

  while(1) {
    printf("\n%s %s? (Yes, No, Go, Quit) ", prompt, filename);

    action = getchar();
    while(isspace(action)) {
      action = getchar();
    }

    action = toupper(action);

    switch(action) 
      {
      case 'Y':
	return(A_ACTION_YES);
	break;
      case 'N':
	return(A_ACTION_NO);
	break;
      case 'G':
	return(A_ACTION_GO);
	break;
      case 'Q':
	return(A_ACTION_QUIT);
	break;
      }
  }
	
  
}







