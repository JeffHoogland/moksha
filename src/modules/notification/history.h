#ifndef NOTIF_HISTORY_H
#define NOTIF_HISTORY_H

#include "e_mod_main.h"
int              dir_clear(void);
void             popup_items_free(Popup_Items *items);
void             list_add_item(Popup_Data *popup);
Eina_Stringshare *get_time(const char *delimiter);
#endif
