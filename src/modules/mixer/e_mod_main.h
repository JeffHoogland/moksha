#define D_(str) dgettext(PACKAGE, str)

#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "config.h"
#include "e_mod_system.h"
#include <e.h>

#define MOD_CONF_VERSION 3

typedef struct E_Mixer_Gadget_Config
{
   int lock_sliders;
   int show_locked;
   const char *card;
   const char *channel_name;
   const char *id;
   E_Config_Dialog *dialog;
   struct E_Mixer_Instance *instance;
} E_Mixer_Gadget_Config;

typedef struct E_Mixer_Module_Config
{
    int version;
    const char *default_gc_id;
    Evas_Hash *gadgets;
} E_Mixer_Module_Config;

typedef struct E_Mixer_Instance
{
   E_Gadcon_Client *gcc;
   E_Gadcon_Popup *popup;
   E_Menu *menu;

   struct
     {
        Evas_Object *gadget;
        Evas_Object *label;
        Evas_Object *left;
        Evas_Object *right;
        Evas_Object *mute;
        Evas_Object *table;
        Evas_Object *button;
	struct
 	  {
	     Ecore_X_Window win;
	     Ecore_Event_Handler *mouse_up;
	     Ecore_Event_Handler *key_down;
	  } input;
     } ui;

   E_Mixer_System *sys;
   E_Mixer_Channel *channel;
   E_Mixer_Channel_State mixer_state;
   E_Mixer_Gadget_Config *conf;
} E_Mixer_Instance;

typedef struct E_Mixer_Module_Context
{
   E_Config_DD *module_conf_edd;
   E_Config_DD *gadget_conf_edd;
   E_Mixer_Module_Config *conf;
   E_Config_Dialog *conf_dialog;
   E_Mixer_Instance *default_instance;
   Eina_List *instances;
   E_Dialog *mixer_dialog;
   struct st_mixer_actions
     {
        E_Action *incr;
        E_Action *decr;
        E_Action *mute;
     } actions;
} E_Mixer_Module_Context;

EAPI extern E_Module_Api e_modapi;
EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

E_Config_Dialog *e_mixer_config_module_dialog_new(E_Container *con, E_Mixer_Module_Context *ctxt);
E_Config_Dialog *e_mixer_config_dialog_new(E_Container *con, E_Mixer_Gadget_Config *conf);
E_Dialog *e_mixer_app_dialog_new(E_Container *con, void (*func)(E_Dialog *dialog, void *data), void *data);
int e_mixer_app_dialog_select(E_Dialog *dialog, const char *card_name, const char *channel_name);

int e_mixer_update(E_Mixer_Instance *inst);
const char *e_mixer_theme_path(void);

#endif
