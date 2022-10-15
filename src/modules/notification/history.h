#ifndef NOTIF_HISTORY_H
#define NOTIF_HISTORY_H

#include "e_mod_main.h"

//Eet_Error read_history(Eina_List **items
Hist_eet *       history_init(void);                     
Hist_eet *       load_history(const char *filename);
Eina_Bool        store_history(const Hist_eet *hist);
void             popup_items_free(Popup_Items *items);
void             list_add_item(Popup_Data *popup);
Eina_Stringshare *get_time(const char *delimiter);
#endif
