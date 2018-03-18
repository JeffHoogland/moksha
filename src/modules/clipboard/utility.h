#ifndef CLIPBOARD_UTILITY_H
#define CLIPBOARD_UTILITY_H

#include <string.h>
#include "common.h"

#ifndef HAVE_STRNDUP
char *strndup(const char *s, size_t n);
#endif

#ifndef HAVE_STRDUP
char *strdup(const char *s);
#endif

Eina_Bool set_clip_content(char **content, char* text, int mode);
Eina_Bool set_clip_name(char **name, char * text, int mode, int n);
Eina_Bool is_empty(const char *str);

#endif
