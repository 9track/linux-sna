/* cpicsessoin -- helper functions for Linux-SNA cpic programming
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

#include "cpicsession.h"
#include <stdlib.h>
#include <stdio.h>

#if !defined(_WIN32)
#include <ctype.h>
#endif

/****f* cpic_session_new
 * NAME
 *    cpic_session_new
 * SYNOPSIS
 *    ret = cpic_session_new ();
 * INPUTS
 *    None
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
cpic_session *cpic_session_new()
{
   cpic_session *This;

   This = cpic_new(cpic_session, 1);
   if (This == NULL)
      return NULL;

   return This;
}

/****f* cpic_session_destroy
 * NAME
 *    cpic_session_destroy
 * SYNOPSIS
 *    cpic_session_destroy (This);
 * INPUTS
 *    cpic_session *      This       - 
 * DESCRIPTION
 *    DOCUMENT ME!!!
 *****/
void cpic_session_destroy(cpic_session * This)
{
   free (This);
}

int cpic_session_set_tp_name(cpic_session * This, char * tp_name)
{
    int length;
    int rc;

    length = strlen(tp_name);

    if (length < (CM_INT32)sizeof(This->tp_name)) {
        rc = 0;
        memcpy(This->tp_name, tp_name, length);
        This->tp_name[length] = '\0';
    } else {
        rc = 1;
    }
    return rc;
}

int cpic_session_set_mode_name(cpic_session * This, char * mode_name)
{
    int length;
    int rc;

    length = strlen(mode_name);

    if (length < (CM_INT32)sizeof(This->mode_name)) {
        rc = 0;
        memcpy(This->mode_name, mode_name, length);
        This->mode_name[length] = '\0';
        strupr(This->mode_name);
    } else {
        rc = 1;
    }

    return rc;
}

int cpic_session_set_symbolic_name(cpic_session * This, char * symbolic_name)
{
    int length;
    int rc;

    length = strlen(symbolic_name);

    This->symbolic = 1;
    if (length < (CM_INT32)sizeof(This->sym_dest_name)) {
        rc = 0;
        memset(This->sym_dest_name, ' ', sizeof(This->sym_dest_name));
        memcpy(This->sym_dest_name, symbolic_name, length);
        This->sym_dest_name[MAX_SYM_DEST_NAME] = '\0';
        strupr(This->sym_dest_name);
    } else {
        rc = 1;
    }

    return rc;
}

int cpic_session_set_destination(cpic_session * This, char * destination)
{
    int length;
    int rc;

    length = strlen(destination);

    This->symbolic = 0;

    if (length < (CM_INT32)sizeof(This->destination)) {
        rc = 0;
        memset(This->destination, ' ', sizeof(This->destination));
        memcpy(This->destination, destination, length);
        This->destination[length] = '\0';
        strupr(This->destination);
    } else {
        rc = 1;
    }

    return rc;
}

int cpic_session_set_userid(cpic_session * This, char * userid)
{
    int length;
    int rc;

    length = strlen(userid);

    if (length < (CM_INT32)sizeof(This->userid)) {
        rc = 0;
        memcpy(This->userid, userid, length);
        This->userid[length] = '\0';
        This->security_type = CM_SECURITY_PROGRAM;
    } else {
        rc = 1;
    }

    return rc;
}

int cpic_session_set_password(cpic_session * This, char * password)
{
    int length;
    int rc;

    length = strlen(password);

    if (length < (CM_INT32)sizeof(This->password)) {
        rc = 0;
        memcpy(This->password, password, length);
        This->password[length] = '\0';
        This->security_type = CM_SECURITY_PROGRAM;
    } else {
        rc = 1;
    }

    return rc;
}

void cpic_session_set_security_type(cpic_session * This, unsigned long security_type)
{
    This->security_type = security_type;
}

int cpic_session_init(cpic_session * This, unsigned char * cm_conv_id, CM_INT32 * cpicrc)
{

   CM_INT32 cm_rc = CM_OK;
   CM_INT32 length;
   char sym_dest_name[MAX_SYM_DEST_NAME];
           
   if (This->symbolic) {

      cminit(cm_conv_id,
             (unsigned char *)This->sym_dest_name,
              &cm_rc);
   }
   else {
      /* Use special symbolic destination name of all blank.  Must set partner LU, etc... */
      memset(sym_dest_name, ' ', sizeof(sym_dest_name));
      cminit(cm_conv_id,
             (unsigned char *)sym_dest_name,
             &cm_rc);
             
      if(cm_rc != CM_OK)
         goto errout;

      /* Set partner LU name */
      length = strlen(This->destination);
      cmspln(cm_conv_id,                  
             (unsigned char *)This->destination,
             &length,
             &cm_rc);
             
      if(cm_rc != CM_OK)
         goto errout;

      length = strlen(This->tp_name);
    
      /* Set TP name */
      cmstpn(cm_conv_id,                      
             (unsigned char *)This->tp_name,
              &length,
              &cm_rc);
              
      if(cm_rc != CM_OK)
         goto errout;        
              
      /* Set mode name */
      length = strlen(This->mode_name);
      cmsmn(cm_conv_id,                   
            (unsigned char *)This->mode_name,
             &length,
             &cm_rc);
   }

errout:

   *cpicrc = cm_rc;
   
   return(0);
}

#if !defined(_WIN32)
/* Convert a string to upper case */
void strupr (char * string) 
{ 
  for (;*string;++string)
  *string=toupper(*string);
} 
#endif
