#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

char * savebuf (char const *s, size_t size);
char * savestr (char const *s);
void memory_fatal(void);
