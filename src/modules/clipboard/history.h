#ifndef CLIPBOARD_HISTORY_H
#define CLIPBOARD_HISTORY_H

#include "common.h"
#include "utility.h"

Eet_Error read_history(Eina_List **items,
                       unsigned ignore_ws, unsigned label_length);
Eet_Error save_history(Eina_List *items);

#endif
