/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#define print0(x, args...)      x ,print1(## args)
#define e_error_message_show(args...) \
{ \
   char __tmpbuf[4096]; \
 \
   snprintf(__tmpbuf, sizeof(__tmpbuf), ##args); \
   e_error_message_show_internal(__tmpbuf); \
}

#define e_error_dialog_show(title, args...) \
{ \
   char __tmpbuf[4096]; \
 \
   snprintf(__tmpbuf, sizeof(__tmpbuf), ##args); \
   e_error_dialog_show_internal(title, __tmpbuf); \
}

#else
#ifndef E_ERROR_H
#define E_ERROR_H

EAPI void e_error_message_show_internal(char *txt);
EAPI void e_error_dialog_show_internal(char *title, char *txt);

EAPI void e_error_gui_set(int on);
EAPI void e_error_message_manager_show(E_Manager *man, char *title, char *txt);
    
#endif
#endif
