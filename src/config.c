#include "debug.h"
#include "actions.h"
#include "border.h"
#include "config.h"
#include "data.h"
#include "e_dir.h"
#include "file.h"
#include "keys.h"
#include "observer.h"
#include "util.h"

static char         cfg_root[] = "";

static char         cfg_grabs_db[PATH_MAX] = "";
static char         cfg_settings_db[PATH_MAX] = "";
static char         cfg_actions_db[PATH_MAX] = "";
static char         cfg_borders_db[PATH_MAX] = "";
static char         cfg_apps_menu_db[PATH_MAX] = "";
static char         cfg_match_db[PATH_MAX] = "";
static char         cfg_menus_dir[PATH_MAX] = "";
static char         cfg_entries_dir[PATH_MAX] = "";
static char         cfg_selections_dir[PATH_MAX] = "";
static char         cfg_scrollbars_dir[PATH_MAX] = "";
static char         cfg_guides_dir[PATH_MAX] = "";
static char         cfg_user_dir[PATH_MAX] = "";
static char         cfg_images_dir[PATH_MAX] = "";
static char         cfg_cursors_dir[PATH_MAX] = "";
static char         cfg_backgrounds_dir[PATH_MAX] = "";
static char         cfg_fonts_dir[PATH_MAX] = "";
static char         cfg_epplets_dir[PATH_MAX] = "";
static char         cfg_layout_dir[PATH_MAX] = "";

static E_Observer  *behavior_dir = NULL;
E_Config           *config_data;

E_Data_Base_Type   *cfg_actions = NULL;
E_Data_Base_Type   *cfg_config = NULL;
E_Data_Base_Type   *cfg_desktops = NULL;
E_Data_Base_Type   *cfg_grabs = NULL;
E_Data_Base_Type   *cfg_guides = NULL;
E_Data_Base_Type   *cfg_match = NULL;
E_Data_Base_Type   *cfg_menu = NULL;
E_Data_Base_Type   *cfg_move = NULL;
E_Data_Base_Type   *cfg_window = NULL;

void                e_config_behavior_changed(E_Observer * observer,
					      E_Observee * observee,
					      E_Event_Type event, void *data);
void                e_config_settings_reload(char *buf);

char               *
e_config_get(char *type)
{
   D_ENTER;

   /* for now use the system defaults and not the user copied settings */
   /* so if i chnage stuff i dont have to rm my personaly settings and */
   /* have e re-install them. yes this is different from e16 - the     */
   /* complexity of merging system and user settings is just a bit     */
   /* much for my liking and have decided, for usability, and          */
   /* user-freindliness to keep all settings in the user's home dir,   */
   /* as well as all data - so the only place to look is there. If you */
   /* have no data it is all copied over for you the first time E is   */
   /* run. It's a design decision.                                     */
   /* Later when things are a bit mroe stabilised these will look      */
   /* something like:                                                  */
   /* E_CONF("grabs", cfg_grabs_db,                                    */
   /*     "%sbehavior/default/grabs.db", e_config_user_dir());         */
   /* notice it would use the user config location instead             */
   /* but for now i'm keeping it as is for development "ease"          */

#define E_CONF(_key, _var, _args...) \
{ \
  if (!strcmp(type, _key)) \
    { \
      if ((_var)[0]) D_RETURN_(_var); \
      snprintf((_var), PATH_MAX, ## _args); \
      D_RETURN_(_var); \
    } \
}

   E_CONF("grabs", cfg_grabs_db, "%s/behavior/grabs.db", e_config_user_dir());
   E_CONF("settings", cfg_settings_db,
	  "%s/behavior/settings.db", e_config_user_dir());
   E_CONF("actions", cfg_actions_db,
	  "%s/behavior/actions.db", e_config_user_dir());
   E_CONF("apps_menu", cfg_apps_menu_db,
	  "%s/behavior/apps_menu.db", e_config_user_dir());
   E_CONF("match", cfg_match_db, "%s/behavior/match.db", e_config_user_dir());
   E_CONF("borders", cfg_borders_db, PACKAGE_DATA_DIR "/data/borders/");
   E_CONF("menus", cfg_menus_dir, PACKAGE_DATA_DIR "/data/menus/");
   E_CONF("entries", cfg_entries_dir, PACKAGE_DATA_DIR "/data/entries/");
   E_CONF("selections", cfg_selections_dir,
	  PACKAGE_DATA_DIR "/data/selections/");
   E_CONF("scrollbars", cfg_scrollbars_dir,
	  PACKAGE_DATA_DIR "/data/scrollbars/");
   E_CONF("guides", cfg_guides_dir, PACKAGE_DATA_DIR "/data/guides/");
   E_CONF("images", cfg_images_dir, PACKAGE_DATA_DIR "/data/images/");
   E_CONF("cursors", cfg_cursors_dir, PACKAGE_DATA_DIR "/data/cursors/");
   E_CONF("backgrounds", cfg_backgrounds_dir,
	  PACKAGE_DATA_DIR "/data/backgrounds/");
   E_CONF("fonts", cfg_fonts_dir, PACKAGE_DATA_DIR "/data/fonts/");
   E_CONF("epplets", cfg_epplets_dir, PACKAGE_DATA_DIR "/data/epplets/");
   E_CONF("layout", cfg_layout_dir, PACKAGE_DATA_DIR "/data/layout/");

   D_RETURN_("");
}

void
e_config_actions_init()
{
   D_ENTER;

   /*
    * Define the data type for the E_Actions struct.
    */
   cfg_actions = e_data_type_new();
   E_DATA_NODE(cfg_actions, "name", E_DATA_TYPE_STR, NULL, E_Action, name,
	       (E_Data_Value) "");
   E_DATA_NODE(cfg_actions, "action", E_DATA_TYPE_STR, NULL, E_Action, action,
	       (E_Data_Value) "");
   E_DATA_NODE(cfg_actions, "params", E_DATA_TYPE_STR, NULL, E_Action,
	       params, (E_Data_Value) "");
   E_DATA_NODE(cfg_actions, "event", E_DATA_TYPE_INT, NULL, E_Action, event,
	       (E_Data_Value) 0);
   E_DATA_NODE(cfg_actions, "button", E_DATA_TYPE_INT, NULL, E_Action, button,
	       (E_Data_Value) 0);
   E_DATA_NODE(cfg_actions, "key", E_DATA_TYPE_STR, NULL, E_Action, key,
	       (E_Data_Value) 0);
   E_DATA_NODE(cfg_actions, "modifiers", E_DATA_TYPE_INT, NULL, E_Action,
	       modifiers, (E_Data_Value) 0);

   D_RETURN;
}

void
e_config_desktops_init()
{
   cfg_desktops = e_data_type_new();
   E_DATA_NODE(cfg_desktops, "count", E_DATA_TYPE_INT, NULL, E_Config_Desktops,
	       count, (E_Data_Value) 8);
   E_DATA_NODE(cfg_desktops, "scroll", E_DATA_TYPE_INT, NULL, E_Config_Desktops,
	       scroll, (E_Data_Value) 1);
   E_DATA_NODE(cfg_desktops, "scroll_sticky", E_DATA_TYPE_INT, NULL, E_Config_Desktops,
	       scroll_sticky, (E_Data_Value) 1);
   E_DATA_NODE(cfg_desktops, "resist", E_DATA_TYPE_INT, NULL, E_Config_Desktops,
	       resist, (E_Data_Value) 5);
   E_DATA_NODE(cfg_desktops, "speed", E_DATA_TYPE_INT, NULL, E_Config_Desktops,
	       speed, (E_Data_Value) 30);
   E_DATA_NODE(cfg_desktops, "width", E_DATA_TYPE_INT, NULL, E_Config_Desktops,
	       width, (E_Data_Value) 1);
   E_DATA_NODE(cfg_desktops, "height", E_DATA_TYPE_INT, NULL, E_Config_Desktops,
	       height, (E_Data_Value) 1);
   E_DATA_NODE(cfg_desktops, "cursors/e_native", E_DATA_TYPE_INT,
	       NULL, E_Config_Desktops, e_native_cursors, (E_Data_Value) 1);
}

void
e_config_grabs_init()
{
   cfg_grabs = e_data_type_new();
   E_DATA_NODE(cfg_grabs, "button", E_DATA_TYPE_INT, NULL, E_Grab, button,
	       (E_Data_Value) 0);
   E_DATA_NODE(cfg_grabs, "modifiers", E_DATA_TYPE_INT, NULL, E_Grab, mods,
	       (E_Data_Value) 0);
}

void
e_config_guides_init()
{
   cfg_guides = e_data_type_new();
   E_DATA_NODE(cfg_guides, "display/location", E_DATA_TYPE_INT, NULL,
	       E_Config_Guides, location, (E_Data_Value) 0);
   E_DATA_NODE(cfg_guides, "display/x", E_DATA_TYPE_FLOAT, NULL,
	       E_Config_Guides, x, (E_Data_Value) 0);
   E_DATA_NODE(cfg_guides, "display/y", E_DATA_TYPE_FLOAT, NULL,
	       E_Config_Guides, y, (E_Data_Value) 0);
}

void
e_config_menu_init()
{
   cfg_menu = e_data_type_new();
   E_DATA_NODE(cfg_menu, "scroll/resist", E_DATA_TYPE_INT, NULL,
	       E_Config_Menu, resist, (E_Data_Value) 5);
   E_DATA_NODE(cfg_menu, "scroll/speed", E_DATA_TYPE_INT, NULL,
	       E_Config_Menu, speed, (E_Data_Value) 12);
}

void
e_config_move_init()
{
   cfg_move = e_data_type_new();
   E_DATA_NODE(cfg_move, "resist", E_DATA_TYPE_INT, NULL,
	       E_Config_Move, resist, (E_Data_Value) 0);
   E_DATA_NODE(cfg_move, "resist/win", E_DATA_TYPE_INT, NULL,
	       E_Config_Move, win_resist, (E_Data_Value) 0);
   E_DATA_NODE(cfg_move, "resist/desk", E_DATA_TYPE_INT, NULL,
	       E_Config_Move, desk_resist, (E_Data_Value) 0);
}

void
e_config_window_init()
{
   cfg_window = e_data_type_new();
   E_DATA_NODE(cfg_window, "resize/mode", E_DATA_TYPE_INT, NULL,
	       E_Config_Window, resize_mode, (E_Data_Value) 0);
   E_DATA_NODE(cfg_window, "move/mode", E_DATA_TYPE_INT, NULL,
	       E_Config_Window, move_mode, (E_Data_Value) 0);
   E_DATA_NODE(cfg_window, "focus/mode", E_DATA_TYPE_INT, NULL, E_Config_Window,
	       focus_mode, (E_Data_Value) 0);
   E_DATA_NODE(cfg_window, "raise/auto", E_DATA_TYPE_INT, NULL,
	       E_Config_Window, auto_raise, (E_Data_Value) 0);
   E_DATA_NODE(cfg_window, "raise/delay", E_DATA_TYPE_FLOAT, NULL,
	       E_Config_Window, raise_delay, (E_Data_Value) (float)0.6);
   E_DATA_NODE(cfg_window, "place/mode", E_DATA_TYPE_INT, NULL,
	       E_Config_Window, place_mode, (E_Data_Value) 0);
}

void
e_config_init(void)
{
   char                buf[PATH_MAX];
   E_Dir              *dir;

   D_ENTER;

   /* Start by initializing the data loading structures */
   e_config_actions_init();
   e_config_desktops_init();
   e_config_grabs_init();
   e_config_guides_init();
   e_config_menu_init();
   e_config_move_init();
   e_config_window_init();

   /* Then place the data structures within the config description */
   cfg_config = e_data_type_new();
   E_DATA_NODE(cfg_config, "actions", E_DATA_TYPE_LIST, cfg_actions,
	       E_Config, actions, (E_Data_Value) 0);
   E_DATA_NODE(cfg_config, "grabs", E_DATA_TYPE_LIST, cfg_grabs,
	       E_Config, grabs, (E_Data_Value) 0);

   E_DATA_NODE(cfg_config, "desktops", E_DATA_TYPE_PTR, cfg_desktops,
	       E_Config, desktops, (E_Data_Value) 0);
   E_DATA_NODE(cfg_config, "guides", E_DATA_TYPE_PTR, cfg_guides,
	       E_Config, guides, (E_Data_Value) 0);
   E_DATA_NODE(cfg_config, "menu", E_DATA_TYPE_PTR, cfg_menu,
	       E_Config, menu, (E_Data_Value) 0);
   E_DATA_NODE(cfg_config, "move", E_DATA_TYPE_PTR, cfg_move,
	       E_Config, move, (E_Data_Value) 0);
   E_DATA_NODE(cfg_config, "window", E_DATA_TYPE_PTR, cfg_window,
	       E_Config, window, (E_Data_Value) 0);

   /* Create directories as needed */
   if (!e_file_is_dir(e_config_user_dir()))
      e_file_mkdir(e_config_user_dir());
   snprintf(buf, PATH_MAX, "%sappearance", e_config_user_dir());
   if (!e_file_is_dir(buf))
      e_file_mkdir(buf);
   snprintf(buf, PATH_MAX, "%sappearance/borders", e_config_user_dir());
   if (!e_file_is_dir(buf))
      e_file_mkdir(buf);
   snprintf(buf, PATH_MAX, "%sbehavior", e_config_user_dir());
   if (!e_file_is_dir(buf))
      e_file_mkdir(buf);

   /* With the directories created, create files if needed and load config */
   snprintf(buf, PATH_MAX, "%sbehavior/grabs.db", e_config_user_dir());
   if (!e_file_exists(buf))
      e_file_cp(PACKAGE_DATA_DIR "/data/config/behavior/default/grabs.db", buf);
   snprintf(buf, PATH_MAX, "%sbehavior/settings.db", e_config_user_dir());
   if (!e_file_exists(buf))
      e_file_cp(PACKAGE_DATA_DIR "/data/config/behavior/default/settings.db",
		buf);
   snprintf(buf, PATH_MAX, "%sbehavior/actions.db", e_config_user_dir());
   if (!e_file_exists(buf))
      e_file_cp(PACKAGE_DATA_DIR "/data/config/behavior/default/actions.db",
		buf);
   snprintf(buf, PATH_MAX, "%sbehavior/apps_menu.db", e_config_user_dir());
   if (!e_file_exists(buf))
      e_file_cp(PACKAGE_DATA_DIR "/data/config/behavior/default/apps_menu.db",
		buf);
   snprintf(buf, PATH_MAX, "%sbehavior/behavior.db", e_config_user_dir());
   if (!e_file_exists(buf))
      e_file_cp(PACKAGE_DATA_DIR "/data/config/behavior/default/behavior.db",
		buf);

   /* Load config data and begin monitoring it with efsd */
   e_config_behavior_changed(NULL, NULL, 0, NULL);

   snprintf(buf, PATH_MAX, "%sbehavior", e_config_user_dir());
   dir = e_dir_new();
   e_dir_set_dir(dir, buf);

   behavior_dir = NEW(E_Observer, 1);
   ZERO(behavior_dir, sizeof(E_Observer), 1);
   e_observer_init(behavior_dir, E_EVENT_FILE_CHANGE,
		   e_config_behavior_changed, free);
   e_observer_register_observee(behavior_dir, E_OBSERVEE(dir));

   D_RETURN;
}

void
e_config_behavior_changed(E_Observer * observer, E_Observee * observee,
			  E_Event_Type event, void *data)
{
   char                buf[PATH_MAX];
   Evas_List          *l;

   if (config_data)
     {
	e_data_free(cfg_config, (char *)config_data);
	FREE(config_data);
     }

   snprintf(buf, PATH_MAX, "%sbehavior/behavior.db", e_config_user_dir());
   config_data = e_data_load(buf, "", cfg_config);

   /* FIXME: this should probably be a function in actions.c */
   for (l = config_data->actions; l; l = l->next)
     {
	E_Action           *a;

	a = l->data;
	e_object_init(E_OBJECT(a), (E_Cleanup_Func) e_action_cleanup);
	if ((a->key) && (strlen(a->key) > 0))
	  {
	     if (a->modifiers == -1)
		e_keys_grab(a->key, ECORE_EVENT_KEY_MODIFIER_NONE, 1);
	     else
		e_keys_grab(a->key, (Ecore_Event_Key_Modifiers) a->modifiers,
			    0);
	     a->grabbed = 1;
	  }
     }

   return;
   UN(observer);
   UN(observee);
   UN(event);
   UN(data);
}

void
e_config_set_user_dir(char *dir)
{
   D_ENTER;

   STRNCPY(cfg_root, dir, PATH_MAX);
   /* reset the cached dir paths */
   cfg_grabs_db[0] = 0;
   cfg_settings_db[0] = 0;
   cfg_actions_db[0] = 0;
   cfg_borders_db[0] = 0;
   cfg_apps_menu_db[0] = 0;
   cfg_match_db[0] = 0;
   cfg_menus_dir[0] = 0;
   cfg_entries_dir[0] = 0;
   cfg_selections_dir[0] = 0;
   cfg_scrollbars_dir[0] = 0;
   cfg_guides_dir[0] = 0;
   cfg_user_dir[0] = 0;
   cfg_images_dir[0] = 0;
   cfg_cursors_dir[0] = 0;
   cfg_backgrounds_dir[0] = 0;
   cfg_fonts_dir[0] = 0;
   /* init again - if the user hasnt got all the data */
   e_config_init();

   D_RETURN;
}

char               *
e_config_user_dir(void)
{
   D_ENTER;

   /* We copy the config files to the user's home dir, no need to fall back */
   if (cfg_user_dir[0])
      D_RETURN_(cfg_user_dir);

   snprintf(cfg_user_dir, PATH_MAX, "%s/.e/", e_util_get_user_home());

   D_RETURN_(cfg_user_dir);
}
