#include <Evas.h>
#include <Ecore.h>
#include "view.h"
#include "e_view_machine.h"

Ecore_Event_Key_Modifiers multi_select_mod = ECORE_EVENT_KEY_MODIFIER_SHIFT;
Ecore_Event_Key_Modifiers range_select_mod = ECORE_EVENT_KEY_MODIFIER_CTRL;
E_View_Machine     *VM = NULL;
