#include "e.h"

static char cfg_grabs_db[4096] = "";
static char cfg_settings_db[4096] = "";
static char cfg_actions_db[4096] = "";
static char cfg_borders_db[4096] = "";
static char cfg_user_dir[4096] = "";
static char cfg_images_dir[4096] = "";
static char cfg_fonts_dir[4096] = "";

void
e_config_init(void)
{
   if (!e_file_is_dir(e_config_user_dir()))
     {
	char buf[4096];
	
	sprintf(buf, "%s",                     e_config_user_dir());
	e_file_mkdir(buf);
	sprintf(buf, "%sappearance",           e_config_user_dir());
	e_file_mkdir(buf);
	sprintf(buf, "%sappearance/borders",   e_config_user_dir());
	e_file_mkdir(buf);
	sprintf(buf, "%sbehavior",             e_config_user_dir());
	e_file_mkdir(buf);
	sprintf(buf, "%sbehavior/grabs.db",    e_config_user_dir());
	e_file_cp(PACKAGE_DATA_DIR"/data/config/behavior/default/grabs.db", buf);
	sprintf(buf, "%sbehavior/settings.db", e_config_user_dir());
	e_file_cp(PACKAGE_DATA_DIR"/data/config/behavior/default/settings.db", buf);
	sprintf(buf, "%sbehavior/actions.db",  e_config_user_dir());
	e_file_cp(PACKAGE_DATA_DIR"/data/config/behavior/default/actions.db", buf);

	sprintf(buf,                                 "%sappearance/borders/border.bits.db",  e_config_user_dir());
	e_file_cp(PACKAGE_DATA_DIR"/data/config/appearance/default/borders/border.bits.db", buf);
	sprintf(buf,                                 "%sappearance/borders/border2.bits.db",  e_config_user_dir());
	e_file_cp(PACKAGE_DATA_DIR"/data/config/appearance/default/borders/border2.bits.db", buf);
	sprintf(buf,                                 "%sappearance/borders/borderless.bits.db",  e_config_user_dir());
	e_file_cp(PACKAGE_DATA_DIR"/data/config/appearance/default/borders/borderless.bits.db", buf);
     }
}

char *
e_config_user_dir(void)
{
   if (cfg_user_dir[0]) return cfg_user_dir;
   sprintf(cfg_user_dir, "%s/.e/", e_file_home());
   return cfg_user_dir;
}

#define E_CONF(_key, _var, _args...) \
{ \
  if (!strcmp(type, _key)) \
    { \
      if ((_var)[0]) return (_var); \
      sprintf((_var), ## _args); \
      return (_var); \
    } \
}

char *
e_config_get(char *type)
{
   E_CONF("grabs", cfg_grabs_db, 
	  PACKAGE_DATA_DIR"/data/config/behavior/default/grabs.db");
   E_CONF("settings", cfg_settings_db,
	  PACKAGE_DATA_DIR"/data/config/behavior/default/settings.db");
   E_CONF("actions", cfg_actions_db,
	  PACKAGE_DATA_DIR"/data/config/behavior/default/actions.db");
   E_CONF("borders", cfg_borders_db,
	  PACKAGE_DATA_DIR"/data/config/appearance/default/borders/");
   E_CONF("images", cfg_images_dir,
	  PACKAGE_DATA_DIR"/data/images/");
   E_CONF("fonts", cfg_fonts_dir,
	  PACKAGE_DATA_DIR"/data/fonts/");
   return "";
}
