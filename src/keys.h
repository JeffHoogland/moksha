#ifndef E_KEYS_H
#define E_KEYS_H

#include "e.h"

void     ecore_keys_init(void);
void     ecore_keys_grab(char *key, Ecore_Event_Key_Modifiers mods, int anymod);
void     ecore_keys_ungrab(char *key, Ecore_Event_Key_Modifiers mods, int anymod);

#endif
