/*
  Wildcard matching function.
*/

#include <assert.h>

int 
wc_match(char *str, char *pattern)
{
  char ch;
  
  assert(str);
  assert(pattern);

  /* read the first character */
  ch = *pattern++;

  /* run this loop for each character in the */
  while(ch)
    {
      switch(ch)
	{
	case '?':
	  /* always matches */
	  break;
	case '*':
	  /* matches any string. If there are no */
	  /* further characters it always matches */
	  if(*pattern == 0)
	    return 1;
	  /* now call recursively to match each position in the string */
	  while(*str) {
	    /* except for special characters, skip mis-matches */
	    if(*pattern != '?' && *pattern != '\\' && *pattern !=  '*')
	      {
		while(*str && *str != *pattern)
		  str++;
		if(!*str)
		  return 0;
	      }
	    if( wc_match(str, pattern) )
	      return 1;
	    str++;
	  }
	  /* we have tested the whole string and not found a match */
	  return 0;
	case '\\':
	  /* the escape character */
	  ch = *pattern++;
	  /* deliberate case fall-through */
	default:
	  if(*str != ch)
	    return 0;
	}
      
      /* get the next characters from pattern and from str */
      ch = *pattern++;
      str++;
    }
  
  /* if we have got to the end of the string it is a match */
  if(*str == 0)
    return 1;
  else
    return 0;
}



