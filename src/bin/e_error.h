/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#define e_error_message_show(args...) \
{ \
   char __tmpbuf[4096]; \
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
