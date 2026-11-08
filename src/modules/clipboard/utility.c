#include "utility.h"
#include <ctype.h>

#define TRIM_SPACES   0
#define TRIM_NEWLINES 1

static char *_sanitize_ln(const char *text, const unsigned int n, const int ignore_ws);
static char *strip_whitespace(char *str, int mode);
static int   isnewline(const int c);

/**
 * @brief Makes a copy of text, optionally trimmed.
 *
 * mode: 0 don't trim, 1 trim new lines, 2 and 3 trim all white space
 *       (white space includes new lines).
 */
Eina_Bool
set_clip_content(char **content, const char *text, int mode)
{
  char *temp, *trim;

  if (!content) {
    ERR("Error: Clip content pointer is Null!!");
    return EINA_FALSE;
  }
  *content = NULL;
  /* Sanity check */
  if (!text) {
    WRN("ERROR: Text is NULL\n");
    text = "";
  }
  /* Always work on a copy: strip_whitespace() writes into the string and
   * the original belongs to somebody else (X event data, dialog entry). */
  temp = strdup(text);
  if (!temp) {
    /* This is bad, leave it to calling function */
    CRI("ERROR: Memory allocation Failed!!");
    return EINA_FALSE;
  }

  switch (mode) {
    case 0:
      break;
    case 1:
      trim = strip_whitespace(temp, TRIM_NEWLINES);
      if (trim != temp) memmove(temp, trim, strlen(trim) + 1);
      break;
    case 2:
    case 3:
      trim = strip_whitespace(temp, TRIM_SPACES);
      if (trim != temp) memmove(temp, trim, strlen(trim) + 1);
      break;
    default :
      /* Error Don't trim */
      WRN("ERROR: Invalid strip_mode %d\n", mode);
      break;
  }
  *content = temp;
  return EINA_TRUE;
}

Eina_Bool
set_clip_name(char **name, const char *text, int mode, int n)
{
  if (!name) {
    ERR("Error: Clip name pointer is Null!!");
    return EINA_FALSE;
  }
  /* Sanity check */
  if (!text) {
    WRN("ERROR: Text is NULL\n");
    text = "";
  }
  if (n < 1) n = 1;

  *name = _sanitize_ln(text, (unsigned int) n, mode);
  if (!*name) {
    /* This is bad, leave it to calling function */
    CRI("ERROR: Memory allocation Failed!!");
    return EINA_FALSE;
  }
  return EINA_TRUE;
}

/* Builds a one line label of at most n bytes. The source is not modified. */
static char *
_sanitize_ln(const char *text, const unsigned int n, const int ignore_ws)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(text, NULL);

  const unsigned char *p = (const unsigned char *) text;
  char *ret = calloc(n + 1, sizeof(char));
  unsigned int i = 0, k;

  if (!ret) return NULL;

  if (ignore_ws)
    while (*p && isspace(*p)) p++;

  while (*p && (i < n)) {
    if (*p == '\t') {
      /* default tab */
      for (k = 0; (k < 4) && (i < n); k++)
        ret[i++] = ' ';
    }
    else if (*p >= 32) {
      /* assume char is ok and add to the label */
      ret[i++] = (char) *p;
    }
    /* other control characters (new lines...) are dropped */
    p++;
  }

  /* We may have cut a multi byte UTF-8 sequence in half: drop the stub */
  if (*p && ((*p & 0xC0) == 0x80)) {
    while ((i > 0) && (((unsigned char) ret[i - 1] & 0xC0) == 0x80))
      i--;
    if (i > 0) i--;
  }
  if (ignore_ws)
    while ((i > 0) && (ret[i - 1] == ' '))
      i--;
  ret[i] = 0;

  return ret;
}


/**
 * @brief Strips whitespace from both ends of a string, IN PLACE.
 *
 * @param str char pointer to a string.
 *
 * @return a char pointer to a substring of the original string..
 *
 * The returned pointer points inside str: the caller must keep the original
 *  pointer for free() and never free the return value.
 */
static char *
strip_whitespace(char *str, int mode)
{
  char *end;
  int (*compare)(int);

  if (mode == TRIM_SPACES) compare = isspace;
  else                     compare = isnewline;

  while (*str && (*compare)((unsigned char) *str)) str++;

  if (*str == 0)  // empty string ?
    return str;

  end = str + strlen(str) - 1;
  while ((end > str) && (*compare)((unsigned char) *end))
    end--;

  // Write new null terminator
  *(end + 1) = 0;

  return str;
}

static int
isnewline(const int c)
{
  return (c == '\n') || (c == '\r');
}

Eina_Bool
is_empty(const char *str)
{
  EINA_SAFETY_ON_NULL_RETURN_VAL(str, EINA_TRUE);

  while (*str && isspace((unsigned char) *str)) str++;
  return !*str;
}
