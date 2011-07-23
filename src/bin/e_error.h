#ifdef E_TYPEDEFS

#define e_error_message_show(args...) \
{ \
   char __tmpbuf[PATH_MAX]; \
 \
   snprintf(__tmpbuf, sizeof(__tmpbuf), ##args); \
   e_error_message_show_internal(__tmpbuf); \
}

#else
#ifndef E_ERROR_H
#define E_ERROR_H

EAPI void e_error_message_show_internal(char *txt);
  
#endif
#endif
