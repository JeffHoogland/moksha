#ifndef E_ERROR_H
#define E_ERROR_H

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

void e_error_message_show_internal(char *txt);
void e_error_dialog_show_internal(char *title, char *txt);

void e_error_gui_set(int on);
void e_error_message_manager_show(E_Manager *man, char *title, char *txt);
    
#endif
