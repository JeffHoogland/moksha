#include "e.h"

static char cfg_grabs_db[4096];
static char cfg_settings_db[4096];
static char cfg_actions_db[4096];
static char cfg_borders[4096];

char *
e_config_get(char *type)
{
   if (!strcmp(type, "grabs"))
     {
	sprintf(cfg_grabs_db, 
		PACKAGE_DATA_DIR"/data/config/behavior/default/grabs.db");
	return cfg_grabs_db;
     }
   if (!strcmp(type, "settings"))
     {
	sprintf(cfg_settings_db, 
		PACKAGE_DATA_DIR"/data/config/behavior/default/settings.db");
	return cfg_settings_db;
     }
   if (!strcmp(type, "actions"))
     {
	sprintf(cfg_actions_db, 
		PACKAGE_DATA_DIR"/data/config/behavior/default/actions.db");
	return cfg_actions_db;
     }
   if (!strcmp(type, "borders"))
     {
	sprintf(cfg_borders, 
		PACKAGE_DATA_DIR"/data/config/appearance/default/borders/");
	return cfg_borders;
     }
   if (!strcmp(type, "images"))
     {
	sprintf(cfg_borders, 
		PACKAGE_DATA_DIR"/data/images/");
	return cfg_borders;
     }
   if (!strcmp(type, "fonts"))
     {
	sprintf(cfg_borders, 
		PACKAGE_DATA_DIR"/data/fonts/");
	return cfg_borders;
     }
   return "";
}
