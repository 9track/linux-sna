/* parse.c: Linux-SNA configuration file parser.
 * - largly stolen from the samba parsing ops.
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

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <net/if_arp.h>
#include <net/ethernet.h>
#include <linux/sna.h>
#include "snaconfig.h"

struct wordmap mode_types[] = {
        { "define",     0               },
        { "start",      1               },
        { "stop",       2               },
        { "delete",     3               },
	{ "sub_opts",	4		},
        { NULL,         -1              }
};

struct wordmap on_types[] = {
        { "off",        0               },
        { "on",         1               },
        { NULL,         -1              }
};

struct wordmap prop_types[] = {
        { "lan",        0               },
        { NULL,         -1              }
};

struct wordmap tx_types[] = {
        { "low",        0               },
        { "medium",     1               },
        { "high",       2               },
        { "network",    3               },
        { NULL,         -1              }
};

struct wordmap security_types[] = {
        { "none",       0               }
};

void *Realloc(void *p,size_t size)
{
  	void *ret=NULL;

  	if(size == 0) 
	{
    		if(p) 
			free(p);
    		return (NULL);
  	}

  	if(!p)
    		ret = (void *)malloc(size);
  	else
    		ret = (void *)realloc(p,size);

  	return (ret);
}

/* -------------------------------------------------------------------------- ** * Variables...
 *
 *  bufr        - pointer to a global buffer.  This is probably a kludge,
 *                but it was the nicest kludge I could think of (for now).
 *  bSize       - The size of the global buffer <bufr>.
 */

#define BUFR_INC 1024

static char *bufr  = NULL;
static int   bSize = 0;

/* Scan past whitespace (see ctype(3C)) and return the first non-whitespace
 * character, or newline, or EOF.
 *
 *  Input:  InFile  - Input source.
 *
 *  Output: The next non-whitespace character in the input stream.
 *
 *  Notes:  Because the config files use a line-oriented grammar, we
 *          explicitly exclude the newline character from the list of
 *          whitespace characters.
 *        - Note that both EOF (-1) and the nul character ('\0') are
 *          considered end-of-file markers.
 */
static int EatWhitespace(FILE *InFile)
{
	int c;

	for(c = getc(InFile); isspace(c) && ('\n' != c); c = getc(InFile))
    		;
  	return (c);
}

/* Scan to the end of a comment.
 *
 *  Input:  InFile  - Input source.
 *
 *  Output: The character that marks the end of the comment.  Normally,
 *          this will be a newline, but it *might* be an EOF.
 *
 *  Notes:  Because the config files use a line-oriented grammar, we
 *          explicitly exclude the newline character from the list of
 *          whitespace characters.
 *        - Note that both EOF (-1) and the nul character ('\0') are
 *          considered end-of-file markers.
 */
static int EatComment(FILE *InFile)
{
	int c;

  	for(c = getc(InFile); ('\n'!=c) && (EOF!=c) && (c>0); c = getc(InFile))
    		;
  	return (c);
}

/*****************************************************************************
 * Scan backards within a string to discover if the last non-whitespace
 * character is a line-continuation character ('\\').
 *
 *  Input:  line  - A pointer to a buffer containing the string to be
 *                  scanned.
 *          pos   - This is taken to be the offset of the end of the
 *                  string.  This position is *not* scanned.
 *
 *  Output: The offset of the '\\' character if it was found, or -1 to
 *          indicate that it was not.
 *
 *****************************************************************************/

static int Continuation(char *line, int pos)
{
  	int pos2 = 0;

  	pos--;
  	while((pos >= 0) && isspace(line[pos]))
     		pos--;

  	/* we should recognize if `\` is part of multibyte character or not. */
  	while(pos2 <= pos) 
	{
    		size_t skip = 0;
    //		skip = get_character_len(line[pos2]);
    		if(skip)
        		pos2 += skip;
    		else 
		{
			if(pos == pos2) 
        			return(((pos >= 0) 
					&& ('\\' == line[pos])) ? pos : -1);
    			else
        			pos2++;
		}
  	}
  	return (-1);
}

/* Scan a section name, and pass the name to function sfunc().
 *
 *  Input:  InFile  - Input source.
 *          sfunc   - Pointer to the function to be called if the section
 *                    name is successfully read.
 *
 *  Output: True if the section name was read and True was returned from
 *          <sfunc>.  False if <sfunc> failed or if a lexical error was
 *          encountered.
 *
 */
static int Section(FILE *InFile, int (*sfunc)(char *))
{
	int   c;
  	int   i;
  	int   end;
  	char *func  = "params.c:Section() -";

  	i = 0;      /* <i> is the offset of the next free byte in bufr[] and  */
  	end = 0;    /* <end> is the current "end of string" offset.  In most  */
      	            /* cases these will be the same, but if the last          */
                    /* character written to bufr[] is a space, then <end>     */
                    /* will be one less than <i>.                             */

  	c = EatWhitespace(InFile);      /* We've already got the '['.  Scan */
                                        /* past initial white space.        */

  	while((EOF != c) && (c > 0))
    	{

    		/* Check that the buffer is big enough for the next character */
    		if(i > (bSize - 2))
      		{
      			bSize += BUFR_INC;
      			bufr   = Realloc(bufr, bSize);
      			if(NULL == bufr)
        			return (0);
      		}

    		/* Handle a single character. */
    		switch(c)
      		{
      			case ']':
        			bufr[end] = '\0';
        			if(0 == end)                  /* Don't allow an empty name.       */
          			{
          				return (0);
          			}
        			if(!sfunc(bufr))            /* Got a valid name.  Deal with it. */
          				return (0);
        			(void)EatComment(InFile);     /* Finish off the line.             */
        			return (1);

      			case '\n':                      /* Got newline before closing ']'.    */
        			i = Continuation( bufr, i );    /* Check for line continuation.     */
        			if( i < 0 )
          			{
          				bufr[end] = '\0';
          				return(0);
          			}
        			end = ( (i > 0) && (' ' == bufr[i - 1]) ) ? (i - 1) : (i);
        			c = getc( InFile );             /* Continue with next line.         */
        			break;

      			default:                        /* All else are a valid name chars.   */
        			if( isspace( c ) )              /* One space per whitespace region. */
          			{
          				bufr[end] = ' ';
          				i = end + 1;
          				c = EatWhitespace( InFile );
          			}
        			else                            /* All others copy verbatim.        */
          			{
          				bufr[i++] = c;
          				end = i;
          				c = getc( InFile );
          			}
    		}
	}

  	return (0);
}

static int Parameter( FILE *InFile, int (*pfunc)(char *, char *), int c )
  /* ------------------------------------------------------------------------ **   * Scan a parameter name and value, and pass these two fields to pfunc().
   *
   *  Input:  InFile  - The input source.
   *          pfunc   - A pointer to the function that will be called to
   *                    process the parameter, once it has been scanned.
   *          c       - The first character of the parameter name, which
   *                    would have been read by Parse().  Unlike a comment
   *                    line or a section header, there is no lead-in
   *                    character that can be discarded.
   *
   *  Output: True if the parameter name and value were scanned and processed
   *          successfully, else False.
   *
   *  Notes:  This function is in two parts.  The first loop scans the
   *          parameter name.  Internal whitespace is compressed, and an
   *          equal sign (=) terminates the token.  Leading and trailing
   *          whitespace is discarded.  The second loop scans the parameter
   *          value.  When both have been successfully identified, they are
   *          passed to pfunc() for processing.
   *
   * ------------------------------------------------------------------------ **   */
  {
  int   i       = 0;    /* Position within bufr. */
  int   end     = 0;    /* bufr[end] is current end-of-string. */
  int   vstart  = 0;    /* Starting position of the parameter value. */
  char *func    = "params.c:Parameter() -";

  /* Read the parameter name. */
  while( 0 == vstart )  /* Loop until we've found the start of the value. */
    {

    if( i > (bSize - 2) )       /* Ensure there's space for next char.    */
      {
      bSize += BUFR_INC;
      bufr   = Realloc( bufr, bSize );
      if( NULL == bufr )
        {
        return(0);
        }
      }

    switch( c )
      {
      case '=':                 /* Equal sign marks end of param name. */
        if( 0 == end )              /* Don't allow an empty name.      */
          {
          return(0);
          }
        bufr[end++] = '\0';         /* Mark end of string & advance.   */
        i       = end;              /* New string starts here.         */
        vstart  = end;              /* New string is parameter value.  */
        bufr[i] = '\0';             /* New string is nul, for now.     */
        break;

      case '\n':                /* Find continuation char, else error. */
        i = Continuation( bufr, i );
        if( i < 0 )
          {
          bufr[end] = '\0';
          return(1);
          }
        end = ( (i > 0) && (' ' == bufr[i - 1]) ) ? (i - 1) : (i);
        c = getc( InFile );       /* Read past eoln.                   */
        break;

      case '\0':                /* Shouldn't have EOF within param name. */
      case EOF:
        bufr[i] = '\0';
        return(1);

      default:
        if( isspace( c ) )     /* One ' ' per whitespace region.       */
          {
          bufr[end] = ' ';
          i = end + 1;
          c = EatWhitespace( InFile );
          }
        else                   /* All others verbatim.                 */
          {
          bufr[i++] = c;
          end = i;
          c = getc( InFile );
          }
      }
    }

  /* Now parse the value. */
  c = EatWhitespace( InFile );  /* Again, trim leading whitespace. */
  while( (EOF !=c) && (c > 0) )
    {

    if( i > (bSize - 2) )       /* Make sure there's enough room. */
      {
      bSize += BUFR_INC;
      bufr   = Realloc( bufr, bSize );
      if( NULL == bufr )
        {
        return(0);
        }
      }

    switch( c )
      {
      case '\r':              /* Explicitly remove '\r' because the older */
        c = getc( InFile );   /* version called fgets_slash() which also  */
        break;                /* removes them.                            */

      case '\n':              /* Marks end of value unless there's a '\'. */
        i = Continuation( bufr, i );
        if( i < 0 )
          c = 0;
        else
          {
          for( end = i; (end >= 0) && isspace(bufr[end]); end-- )
            ;
          c = getc( InFile );
          }
        break;

      default:               /* All others verbatim.  Note that spaces do */
        bufr[i++] = c;       /* not advance <end>.  This allows trimming  */
        if( !isspace( c ) )  /* of whitespace at the end of the line.     */
          end = i;
        c = getc( InFile );
        break;
      }
    }
  bufr[end] = '\0';          /* End of value. */

  return( pfunc( bufr, &bufr[vstart] ) );   /* Pass name & value to pfunc().  */  } /* Parameter */

/* Scan & parse the input.
 *
 *  Input:  InFile  - Input source.
 *          sfunc   - Function to be called when a section name is scanned.
 *                    See Section().
 *          pfunc   - Function to be called when a parameter is scanned.
 *                    See Parameter().
 *
 *  Output: True if the file was successfully scanned, else False.
 *
 *  Notes:  The input can be viewed in terms of 'lines'.  There are four
 *          types of lines:
 *            Blank      - May contain whitespace, otherwise empty.
 *            Comment    - First non-whitespace character is a ';' or '#'.
 *                         The remainder of the line is ignored.
 *            Section    - First non-whitespace character is a '['.
 *            Parameter  - The default case.
 */
int Parse(FILE *InFile,
                   int (*sfunc)(char *),
                   int (*pfunc)(char *, char *))
{
  	int    c;

  	c = EatWhitespace(InFile);
  	while((EOF != c) && (c > 0))
    	{
    		switch(c)
      		{
      			case '\n':                        /* Blank line. */
        			c = EatWhitespace(InFile);
        			break;

      			case ';':                         /* Comment line. */
      			case '#':
        			c = EatComment(InFile);
        			break;

      			case '[':                         /* Section Header. */
        			if(!Section(InFile, sfunc))
          				return (0);
        			c = EatWhitespace(InFile);
        			break;

      			case '\\':                        /* Bogus backslash. */
        			c = EatWhitespace(InFile);
        			break;

      			default:                          /* Parameter line. */
        			if(!Parameter(InFile, pfunc, c))
          				return (0);
        			c = EatWhitespace(InFile);
        		break;
      		}
    	}

  	return (1);
}

/* Open a configuration file.
 *
 *  Input:  FileName  - The pathname of the config file to be opened.
 *
 *  Output: A pointer of type (FILE *) to the opened file, or NULL if the
 *          file could not be opened.
 */
FILE *OpenConfFile(char *FileName)
{
  	FILE *OpenedFile;
  	char *func = "params.c:OpenConfFile() -";
  	extern int in_client;
  	int lvl = in_client?1:0;

  	if(NULL == FileName || 0 == *FileName)
    	{
    		return (NULL);
    	}

  	OpenedFile = fopen(FileName, "r");
  	return (OpenedFile);
}

/* Process the named parameter file.
 *
 *  Input:  FileName  - The pathname of the parameter file to be opened.
 *          sfunc     - A pointer to a function that will be called when
 *                      a section name is discovered.
 *          pfunc     - A pointer to a function that will be called when
 *                      a parameter name and value are discovered.
 *
 *  Output: TRUE if the file was successfully parsed, else FALSE.
 */
int pm_process(char *FileName, int (*sfunc)(char *),
	int (*pfunc)(char *, char *))
{
  	int   result;
  	FILE *InFile;
  	char *func = "params.c:pm_process() -";

  	InFile = OpenConfFile(FileName);          /* Open the config file. */
  	if(NULL == InFile)
    		return (0);

  	if(NULL != bufr)                  /* If we already have a buffer */
    		result = Parse(InFile, sfunc, pfunc);   /* (recursive call), then just */
                                              /* use it.                     */

  	else                                  /* If we don't have a buffer   */
    	{                                     /* allocate one, then parse,   */
    		bSize = BUFR_INC;             /* then free.                  */
    		bufr = (char *)malloc( bSize );
    		if( NULL == bufr )
      		{
      			fclose(InFile);
      			return(0);
      		}
    		result = Parse( InFile, sfunc, pfunc );
    		free( bufr );
    		bufr  = NULL;
    		bSize = 0;
    	}

  	fclose(InFile);

  	if(!result)                              /* Generic failure. */
    		return (0);

  	return (1);                             /* Generic success. */
}
