#ifndef E_FOCUS_H
#define E_FOCUS_H

#include "border.h"

void e_focus_set_focus(E_Border *b);
int  e_focus_can_focus(E_Border *b);
void e_focus_list_border_add(E_Border *b);
void e_focus_list_border_del(E_Border *b);
void e_focus_list_clear(void);

#endif

