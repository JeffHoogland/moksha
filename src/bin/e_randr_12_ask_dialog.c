#include "e.h"

static void _e_randr_ask_cb(void *data, E_Dialog *dia);

static void e_randr_12_memorize_monitor_dialog_new(void);
static void _e_randr_ask_memorize_monitor_cb(void *data, E_Dialog *dia);

static E_Randr_Output_Info *_ask_output_info = NULL;

EINTERN void e_randr_12_ask_dialog_new(E_Randr_Output_Info *oi)
{
   E_Dialog *dia = NULL;

   if (!oi) return;
   dia = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_randr_ask");
   if (!dia) return;

   _ask_output_info = oi;
   e_dialog_title_set(dia, _("Position of New Monitor"));
   e_dialog_icon_set(dia, "display", 64);
   e_dialog_text_set(dia, _("<b>Where</b> should the newly connected monitor be put?"));
   e_dialog_button_add(dia, _("Left"), "stock-left", _e_randr_ask_cb, (void*)ECORE_X_RANDR_OUTPUT_POLICY_LEFT);
   e_dialog_button_add(dia, _("Right"), "stock-right", _e_randr_ask_cb, (void*)ECORE_X_RANDR_OUTPUT_POLICY_RIGHT);
   e_dialog_button_add(dia, _("Above"), "stock-top", _e_randr_ask_cb, (void*)ECORE_X_RANDR_OUTPUT_POLICY_ABOVE);
   e_dialog_button_add(dia, _("Below"), "stock-bottom", _e_randr_ask_cb, (void*)ECORE_X_RANDR_OUTPUT_POLICY_BELOW);
   e_dialog_button_focus_num(dia, 1);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
}

static void
_e_randr_ask_cb(void *data __UNUSED__, E_Dialog *dia)
{
   Ecore_X_Randr_Output_Policy pos = (Ecore_X_Randr_Output_Policy)data;
   Eina_Bool successful = EINA_FALSE;

   if (!_ask_output_info)
     goto _close_ret;

   successful = e_randr_12_try_enable_output(_ask_output_info, pos, EINA_FALSE);

   if (successful)
     e_randr_12_memorize_monitor_dialog_new();

   _ask_output_info = NULL;

_close_ret:
   e_object_del(E_OBJECT(dia));
}

static void e_randr_12_memorize_monitor_dialog_new(void)
{
   E_Dialog *dia = NULL;

   dia = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_randr_ask");
   if (!dia) return;

   e_dialog_title_set(dia, _("Memorize This Monitor?"));
   e_dialog_icon_set(dia, "display", 64);
   e_dialog_text_set(dia, _("Should the position of this monitor be <b>memorized</b>?"));
   e_dialog_button_add(dia, _("Yes"), "stock-yes", _e_randr_ask_memorize_monitor_cb, (void*)EINA_TRUE);
   e_dialog_button_add(dia, _("No"), "stock-no", _e_randr_ask_memorize_monitor_cb, (void*)EINA_FALSE);
   e_dialog_button_focus_num(dia, 1);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
}

static void
_e_randr_ask_memorize_monitor_cb(void *data, E_Dialog *dia)
{
   Eina_Bool memorize = (Eina_Bool)(intptr_t)data;
   const E_Randr_Configuration_Store_Modifier modifier = (
         E_RANDR_CONFIGURATION_STORE_RESOLUTIONS
         | E_RANDR_CONFIGURATION_STORE_ARRANGEMENT
         | E_RANDR_CONFIGURATION_STORE_ORIENTATIONS);

   if (memorize)
     e_randr_store_configuration(modifier);

   e_object_del(E_OBJECT(dia));
}
