#include "utility.h"

#define TRIM_SPACES   0
#define TRIM_NEWLINES 1

char * _sanitize_ln(char *text, const unsigned int n, const int mode);

#ifndef HAVE_STRNDUP
char *
strndup(const char *s, size_t n)
{
  char *ret = malloc(n + 1);
  if (!ret) return NULL;

  memcpy(ret, s, n);
  ret[n] = 0;

  return (char *) ret;
}
#endif

#ifndef HAVE_STRDUP
char *
strdup (const char *s)
{
  size_t len = strlen(s) + 1;
  void *ret = malloc(len);

  if (!ret) return NULL;

  return (char *) memcpy(ret, s, len);
}
#endif

char *strip_whitespace(char *str, int mode);
int isnewline(const int c);

Eina_Bool
set_clip_content(char **content, char* text, int mode)
{
  Eina_Bool ret = EINA_TRUE;
  char *temp, *trim;
  char buf[50];
  /* Sanity check */
  if (!text) {
    WRN("ERROR: Text is NULL\n");
    text = "";
  }
  if (content) {
    switch (mode) {
      case 0:
        /* Don't trim */
        temp = strdup(text);
        break;
      case 1:
        /* Trim new lines */
        trim = strip_whitespace(text, TRIM_NEWLINES);
        temp = strdup(trim);
        break;
      case 2:
        /* Trim all white Space
         *  since white space includes new lines
         *  drop thru here */
      case 3:
        /* Trim white space and new lines */
        trim = strip_whitespace(text, TRIM_SPACES);
        temp = strdup(trim);
        break;
      default :
        /* Error Don't trim */
        WRN("ERROR: Invalid strip_mode %d\n", mode);
        temp = strdup(text);
        break;
    }
    if (!temp) {
      /* This is bad, leave it to calling function */
      CRI("ERROR: Memory allocation Failed!!");
      ret = EINA_FALSE;
    }
    *content = temp;
  } else
    ERR("Error: Clip content pointer is Null!!");
  return ret;
}

Eina_Bool
set_clip_name(char **name, char * text, int mode, int n)
{
  Eina_Bool ret = EINA_TRUE;
  char *temp, *trim;

  /* Sanity check */
  if (!text) {
    WRN("ERROR: Text is NULL\n");
    text = "";
  }
  /* to be continued latter */
  if (name)
    *name = _sanitize_ln(text, n, mode);
    //~ *name = text;
  else {
    ERR("Error: Clip name pointer is Null!!");
    return EINA_FALSE;
  }

  if (!*name) {
      /* This is bad, leave it to calling function */
      CRI("ERROR: Memory allocation Failed!!");
      ret = EINA_FALSE;
    }

  return ret;
}

char *
_sanitize_ln(char *text, const unsigned int n, const int mode)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(text, NULL);

  char *ret = malloc(n + 1);
  char *temp = ret;
  int chr, i = 0;

  if (!ret) return NULL;

  if (mode)
    text = strip_whitespace(text, TRIM_SPACES);
    
  while (1) {
    chr = *text;
    if (chr == 0)
      break;
    if ((chr < 32) && (chr >0)) {
      /* is it a tab */
      if (chr == 9){
        // default tab
        for (i; i + 4; i++){
          if (i == n) break;
          *temp++ = ' ';
        }
        text++;
      }
      else {
        text++;
      }
    }
    else {
    
    /* assume char is ok and add to temp buffer */
    *temp++ = *text++;
    
    i++;
    }
    if (i == n) break;
  }
  *temp = 0;
      

  return ret;
}


/**
 * @brief Strips whitespace from a string.
 *
 * @param str char pointer to a string.
 *
 * @return a char pointer to a substring of the original string..
 *
 * If the given string was allocated dynamically, the caller must not overwrite
 *  that pointer with the returned value. The original pointer must be
 *  deallocated using the same allocator with which it was allocated.  The return
 *  value must NOT be deallocated using free etc.
 *
 * You have been warned!!
 */
char *
strip_whitespace(char *str, int mode)
{
  char *end;
  int (*compare)(int);

  if (mode == TRIM_SPACES) compare = isspace;
  else                     compare = isnewline;

  while((*compare)(*str)) str++;

  if(*str == 0)  // empty string ?
    return str;

  end = str + strlen(str) - 1;
  while(end > str && (*compare)(*end))
    end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

int
isnewline(const int c)
{
  return (c == '\n')||(c == '\r');
}

Eina_Bool 
is_empty(const char *str)
{
  EINA_SAFETY_ON_NULL_RETURN(str);
  
  while (isspace((unsigned char) *str) && str++);
  return !*str;
}
