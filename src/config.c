#include "config.h"
#include "util.h"

static char cfg_root[] = "";

#define E_CONF(_key, _var, _args...) \
{ \
  if (!strcmp(type, _key)) \
    { \
      if ((_var)[0]) return (_var); \
      sprintf((_var), ## _args); \
      return (_var); \
    } \
}

static char cfg_grabs_db[4096] = "";
static char cfg_settings_db[4096] = "";
static char cfg_actions_db[4096] = "";
static char cfg_borders_db[4096] = "";
static char cfg_apps_menu_db[4096] = "";
static char cfg_menus_dir[4096] = "";
static char cfg_entries_dir[4096] = "";
static char cfg_selections_dir[4096] = "";
static char cfg_scrollbars_dir[4096] = "";
static char cfg_guides_dir[4096] = "";
static char cfg_user_dir[4096] = "";
static char cfg_images_dir[4096] = "";
static char cfg_cursors_dir[4096] = "";
static char cfg_backgrounds_dir[4096] = "";
static char cfg_fonts_dir[4096] = "";

char *
e_config_get(char *type)
{
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
   /*	  "%sbehavior/default/grabs.db", e_config_user_dir());         */
   /* notice it would use the user config location instead             */
   /* but for now i'm keeping it as is for development "ease"          */
   E_CONF("grabs", cfg_grabs_db, 
	  "%s/behavior/grabs.db", e_config_user_dir());
   E_CONF("settings", cfg_settings_db,
	  "%s/behavior/settings.db", e_config_user_dir());
   E_CONF("actions", cfg_actions_db,
	  "%s/behavior/actions.db", e_config_user_dir());
   E_CONF("apps_menu", cfg_apps_menu_db,
	  "%s/behavior/apps_menu.db", e_config_user_dir());
   E_CONF("borders", cfg_borders_db,
	  PACKAGE_DATA_DIR"/data/config/appearance/default/borders/");
   E_CONF("menus", cfg_menus_dir,
	  PACKAGE_DATA_DIR"/data/config/appearance/default/menus/");
   E_CONF("entries", cfg_entries_dir,
	  PACKAGE_DATA_DIR"/data/config/appearance/default/entries/");
   E_CONF("selections", cfg_selections_dir,
	  PACKAGE_DATA_DIR"/data/config/appearance/default/selections/");
   E_CONF("scrollbars", cfg_scrollbars_dir,
	  PACKAGE_DATA_DIR"/data/config/appearance/default/scrollbars/");
   E_CONF("guides", cfg_guides_dir,
	  PACKAGE_DATA_DIR"/data/config/appearance/default/guides/");
   E_CONF("images", cfg_images_dir,
	  PACKAGE_DATA_DIR"/data/images/");
   E_CONF("cursors", cfg_cursors_dir,
	  PACKAGE_DATA_DIR"/data/cursors/");
   E_CONF("backgrounds", cfg_backgrounds_dir,
	  PACKAGE_DATA_DIR"/data/backgrounds/");
   E_CONF("fonts", cfg_fonts_dir,
	  PACKAGE_DATA_DIR"/data/fonts/");
   return "";
}

void
e_config_init(void)
{
   char buf[4096];

#if 1 /* for now don't do this. i think a cp -r will be needed later anyway */
   if (!e_file_is_dir(e_config_user_dir())) e_file_mkdir(e_config_user_dir());
   sprintf(buf, "%sappearance",           e_config_user_dir());
   if (!e_file_is_dir(buf)) e_file_mkdir(buf);
   sprintf(buf, "%sappearance/borders",   e_config_user_dir());
   if (!e_file_is_dir(buf)) e_file_mkdir(buf);
   sprintf(buf, "%sbehavior",             e_config_user_dir());
   if (!e_file_is_dir(buf)) e_file_mkdir(buf);
   sprintf(buf, "%sbehavior/grabs.db",    e_config_user_dir());
   if (!e_file_exists(buf))
     e_file_cp(PACKAGE_DATA_DIR"/data/config/behavior/default/grabs.db", buf);
   sprintf(buf, "%sbehavior/settings.db", e_config_user_dir());
   if (!e_file_exists(buf))
     e_file_cp(PACKAGE_DATA_DIR"/data/config/behavior/default/settings.db", buf);
   sprintf(buf, "%sbehavior/actions.db",  e_config_user_dir());
   if (!e_file_exists(buf))
     e_file_cp(PACKAGE_DATA_DIR"/data/config/behavior/default/actions.db", buf);
   sprintf(buf, "%sbehavior/apps_menu.db",  e_config_user_dir());
   if (!e_file_exists(buf))
     e_file_cp(PACKAGE_DATA_DIR"/data/config/behavior/default/apps_menu.db", buf);
   sprintf(buf, "%sappearance/borders/border.bits.db",  e_config_user_dir());
#endif
#if 0   
   ts();
#endif
}

void
e_config_set_user_dir(char *dir)
{
   strcpy(cfg_root, dir);
   /* reset the cached dir paths */
   cfg_grabs_db[0]    = 0;
   cfg_settings_db[0] = 0;
   cfg_actions_db[0]  = 0;
   cfg_borders_db[0]  = 0;
   cfg_apps_menu_db[0]= 0;
   cfg_menus_dir[0]   = 0;
   cfg_entries_dir[0] = 0;
   cfg_selections_dir[0] = 0;
   cfg_scrollbars_dir[0] = 0;
   cfg_guides_dir[0] = 0;
   cfg_user_dir[0]    = 0;
   cfg_images_dir[0]  = 0;
   cfg_cursors_dir[0] = 0;
   cfg_backgrounds_dir[0]  = 0;
   cfg_fonts_dir[0]   = 0;
   /* init again - if the user hasnt got all the data */
   e_config_init();
}

char *
e_config_user_dir(void)
{
   if (cfg_user_dir[0]) return cfg_user_dir;
   if (cfg_root[0]) return cfg_root;
#if 1 /* disabled for now - use system ones only */
   sprintf(cfg_user_dir, "%s/.e/", e_file_home());
#else   
   sprintf(cfg_user_dir, PACKAGE_DATA_DIR"/data/config/");
#endif   
   return cfg_user_dir;
}

void
e_config_type_add_node(E_Config_Base_Type *base, char *prefix, 
		       E_Config_Datatype type, E_Config_Base_Type *list_type,
		       int offset)
{
   E_Config_Node *cfg_node;
   
   cfg_node = NEW(E_Config_Node, 1);
   ZERO(cfg_node, E_Config_Node, 1);
   
   cfg_node->prefix   = strdup(prefix);
   cfg_node->type     = type;
   cfg_node->sub_type = list_type;
   cfg_node->offset   = offset;
   
   base->nodes = evas_list_append(base->nodes, cfg_node);
}

E_Config_Base_Type *
e_config_type_new(void)
{
   E_Config_Base_Type *t;
   
   t = NEW(E_Config_Base_Type, 1);
   ZERO(t, E_Config_Base_Type, 1);
   return t;
}

void *
e_config_load(char *file, char *prefix, E_Config_Base_Type *type)
{
   E_DB_File *db;
   char buf[4096];
   Evas_List l;
   char *data;
   
   if (!e_file_exists(file)) return NULL;
   db = e_db_open_read(file);
   if (!db) return NULL;
   data = NEW(char, type->size);
   ZERO(data, char , type->size);
   for (l = type->nodes; l; l = l->next)
     {
	E_Config_Node *node;
	
	node = l->data;
	
	switch (node->type)
	  {
	   case E_CFG_TYPE_INT:
	       {
		  int val;
		  
		  val = 0;
		  sprintf(buf, "%s/%s", prefix, node->prefix);
		  e_db_int_get(db, buf, &val);
		  (*((int *)(&(data[node->offset])))) = val;
	       }
	     break;
	   case E_CFG_TYPE_STR:
	       {
		  char * val;
		  
		  sprintf(buf, "%s/%s", prefix, node->prefix);
		  val = e_db_str_get(db, buf);
		  (*((char **)(&(data[node->offset])))) = val;
	       }
	     break;
	   case E_CFG_TYPE_FLOAT:
	       {
		  float val;
		  
		  val = 0;
		  sprintf(buf, "%s/%s", prefix, node->prefix);
		  e_db_float_get(db, buf, &val);
		  (*((float *)(&(data[node->offset])))) = val;
	       }
	     break;
	   case E_CFG_TYPE_LIST:
	       {
		  Evas_List l2;
		  int i, count;
		  
		  l2 = NULL;
		  sprintf(buf, "%s/%s/count", prefix, node->prefix);
		  count = 0;		  
		  e_db_int_get(db, buf, &count);
		  for (i = 0; i < count; i++)
		    {
		       void *data2;
		       
		       sprintf(buf, "%s/%s/%i", prefix, node->prefix, i);
		       data2 = e_config_load(file, buf, node->sub_type);
		       l2 = evas_list_append(l2, data2);
		    }
		  (*((Evas_List *)(&(data[node->offset])))) = l2;
	       }
	     break;
	   default:
	     break;
	  }
     }
   e_db_close(db);
   return data;
}

#if 0
typedef struct _list_base List_Base;
typedef struct _list_element List_Element;

struct _list_base
{
   Evas_List elements;
};

struct _list_element
{
   char *name;
   int   size;
   float perc;
};

/* eg: */
void ts(void)
{   
   E_Config_Base_Type *cf_list;
   E_Config_Base_Type *cf_element;
   
   cf_element = e_config_type_new();
   E_CONFIG_NODE(cf_element, "name", E_CFG_TYPE_STR, NULL,   List_Element, name); 
   E_CONFIG_NODE(cf_element, "size", E_CFG_TYPE_INT, NULL,   List_Element, size); 
   E_CONFIG_NODE(cf_element, "perc", E_CFG_TYPE_FLOAT, NULL, List_Element, perc); 
   
   cf_list = e_config_type_new();
   E_CONFIG_NODE(cf_list, "list", E_CFG_TYPE_LIST, cf_element, List_Base, elements);
   
     {
	List_Base *cfg_data;
	
	cfg_data = e_config_load("test.db", "", cf_list);
	if (!cfg_data)
	  {
	     printf("no load!\n");
	  }
	else
	  {
	     Evas_List l;
	     
	     for (l = cfg_data->elements; l; l = l->next)
	       {
		  List_Element *cfg_element;
		  
		  printf("element\n");
		  cfg_element = l->data;
		  printf("... name %s\n", cfg_element->name);
		  printf("... size %i\n", cfg_element->size);
		  printf("... perc %3.3f\n", cfg_element->perc);
	       }
	  }
	exit(0);
     }
}

#endif
