#include "debug.h"
#include "config.h"
#include "file.h"
#include "util.h"

static char cfg_root[] = "";

static char cfg_grabs_db[PATH_MAX] = "";
static char cfg_settings_db[PATH_MAX] = "";
static char cfg_actions_db[PATH_MAX] = "";
static char cfg_borders_db[PATH_MAX] = "";
static char cfg_apps_menu_db[PATH_MAX] = "";
static char cfg_menus_dir[PATH_MAX] = "";
static char cfg_entries_dir[PATH_MAX] = "";
static char cfg_selections_dir[PATH_MAX] = "";
static char cfg_scrollbars_dir[PATH_MAX] = "";
static char cfg_guides_dir[PATH_MAX] = "";
static char cfg_user_dir[PATH_MAX] = "";
static char cfg_images_dir[PATH_MAX] = "";
static char cfg_cursors_dir[PATH_MAX] = "";
static char cfg_backgrounds_dir[PATH_MAX] = "";
static char cfg_fonts_dir[PATH_MAX] = "";

char *
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
   /*	  "%sbehavior/default/grabs.db", e_config_user_dir());         */
   /* notice it would use the user config location instead             */
   /* but for now i'm keeping it as is for development "ease"          */

#define E_CONF(_key, _var, _args...) \
{ \
  if (!strcmp(type, _key)) \
    { \
      if ((_var)[0]) D_RETURN_(_var); \
      sprintf((_var), ## _args); \
      D_RETURN_(_var); \
    } \
}

   E_CONF("grabs", cfg_grabs_db, 
	  "%s/behavior/grabs.db", e_config_user_dir());
   E_CONF("settings", cfg_settings_db,
	  "%s/behavior/settings.db", e_config_user_dir());
   E_CONF("actions", cfg_actions_db,
	  "%s/behavior/actions.db", e_config_user_dir());
   E_CONF("apps_menu", cfg_apps_menu_db,
	  "%s/behavior/apps_menu.db", e_config_user_dir());
   E_CONF("borders", cfg_borders_db,
	  PACKAGE_DATA_DIR"/data/borders/");
   E_CONF("menus", cfg_menus_dir,
	  PACKAGE_DATA_DIR"/data/menus/");
   E_CONF("entries", cfg_entries_dir,
	  PACKAGE_DATA_DIR"/data/entries/");
   E_CONF("selections", cfg_selections_dir,
	  PACKAGE_DATA_DIR"/data/selections/");
   E_CONF("scrollbars", cfg_scrollbars_dir,
	  PACKAGE_DATA_DIR"/data/scrollbars/");
   E_CONF("guides", cfg_guides_dir,
	  PACKAGE_DATA_DIR"/data/guides/");
   E_CONF("images", cfg_images_dir,
	  PACKAGE_DATA_DIR"/data/images/");
   E_CONF("cursors", cfg_cursors_dir,
	  PACKAGE_DATA_DIR"/data/cursors/");
   E_CONF("backgrounds", cfg_backgrounds_dir,
	  PACKAGE_DATA_DIR"/data/backgrounds/");
   E_CONF("fonts", cfg_fonts_dir,
	  PACKAGE_DATA_DIR"/data/fonts/");

   D_RETURN_("");
}

void
e_config_init(void)
{
   char buf[PATH_MAX];

   D_ENTER;

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

   D_RETURN;
}

void
e_config_set_user_dir(char *dir)
{
   D_ENTER;

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

   D_RETURN;
}

char *
e_config_user_dir(void)
{
   D_ENTER;

   if (cfg_user_dir[0]) D_RETURN_(cfg_user_dir);
   if (cfg_root[0]) D_RETURN_(cfg_root);
#if 1 /* disabled for now - use system ones only */
   sprintf(cfg_user_dir, "%s/.e/", e_util_get_user_home());
#else   
   sprintf(cfg_user_dir, PACKAGE_DATA_DIR"/data/config/");
#endif   

   D_RETURN_(cfg_user_dir);
}

typedef struct _e_config_file_entry E_Config_File_Entry;

struct _e_config_file_entry
{
   char   *name;
   struct {
      char   *path;
      time_t  last_mod;
   } user, system;
   Evas_List hash[256];
};

void
e_config_add_change_cb(char *file, void (*func) (void *_data), void *data)
{
}

void
e_config_del_change_cb(char *file, void (*func) (void *_data))
{
}

int
e_config_val_int_get(char *file, char *key, int def)
{
}

float
e_config_val_float_get(char *file, char *key, float def)
{
}

char *
e_config_val_str_get(char *file, char *key, char *def)
{
}

char *
e_config_val_key_get(char *file, char *key, char *def)
{
}




void
e_config_type_add_node(E_Config_Base_Type *base, char *prefix, 
		       E_Config_Datatype type, E_Config_Base_Type *list_type,
		       int offset,
		       int def_int,
		       float def_float,
		       char *def_str)
{
   E_Config_Node *cfg_node;
   
   D_ENTER;

   cfg_node = NEW(E_Config_Node, 1);
   ZERO(cfg_node, E_Config_Node, 1);
   
   cfg_node->prefix    = strdup(prefix);
   cfg_node->type      = type;
   cfg_node->sub_type  = list_type;
   cfg_node->offset    = offset;
   cfg_node->def_int   = def_int;
   cfg_node->def_float = def_float;
   if (cfg_node->def_str) 
     {
	e_strdup(cfg_node->def_str, def_str);
     }
   base->nodes = evas_list_append(base->nodes, cfg_node);

   D_RETURN;
}

E_Config_Base_Type *
e_config_type_new(void)
{
   E_Config_Base_Type *t;
   
   D_ENTER;

   t = NEW(E_Config_Base_Type, 1);
   ZERO(t, E_Config_Base_Type, 1);

   D_RETURN_(t);
}

void *
e_config_load(char *file, char *prefix, E_Config_Base_Type *type)
{
   E_DB_File *db;
   char buf[PATH_MAX];
   Evas_List l;
   char *data;
   
   D_ENTER;

   if (!e_file_exists(file)) D_RETURN_(NULL);
   db = e_db_open_read(file);

   if (!db)
     D_RETURN_(NULL);

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
		  if (e_db_int_get(db, buf, &val))
		    (*((int *)(&(data[node->offset])))) = val;
		  else
		    (*((int *)(&(data[node->offset])))) = node->def_int;
	       }
	     break;
	   case E_CFG_TYPE_STR:
	       {
		  char *val;
		  
		  sprintf(buf, "%s/%s", prefix, node->prefix);
		  if ((val = e_db_str_get(db, buf)))
		    (*((char **)(&(data[node->offset])))) = val;
		  else
		    e_strdup((*((char **)(&(data[node->offset])))), node->def_str);
	       }
	     break;
	   case E_CFG_TYPE_FLOAT:
	       {
		  float val;
		  
		  val = 0;
		  sprintf(buf, "%s/%s", prefix, node->prefix);
		  if (e_db_float_get(db, buf, &val))
		    (*((float *)(&(data[node->offset])))) = val;
		  else
		    (*((float *)(&(data[node->offset])))) = node->def_float;
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
	   case E_CFG_TYPE_KEY:
	       {
		  sprintf(buf, "%s/%s", prefix, node->prefix);
		  (*((char **)(&(data[node->offset])))) = strdup(buf);
	       }
	     break;
	   default:
	     break;
	  }
     }
   e_db_close(db);

   D_RETURN_(data);
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
   /* define the different config types and structs to the config engine */
   E_Config_Base_Type *cf_list;
   E_Config_Base_Type *cf_element;
   
   D_ENTER;

   cf_element = e_config_type_new();
   E_CONFIG_NODE(cf_element, "name", E_CFG_TYPE_STR, NULL,   List_Element, name, 0, 0, "DEFAULT_NAME"); 
   E_CONFIG_NODE(cf_element, "size", E_CFG_TYPE_INT, NULL,   List_Element, size, 777, 0, NULL); 
   E_CONFIG_NODE(cf_element, "perc", E_CFG_TYPE_FLOAT, NULL, List_Element, perc, 0, 3.1415, NULL); 
   
   cf_list = e_config_type_new();
   E_CONFIG_NODE(cf_list, "list", E_CFG_TYPE_LIST, cf_element, List_Base, elements, 0, 0, NULL);

   /* now test it */
     {
	List_Base *cfg_data;
	
	/* load the base data type from the base of the test db file */
	cfg_data = e_config_load("test.db", "", cf_list);
	/* no data file? */
	if (!cfg_data)
	  {
	     D("no load!\n");
	  }
	/* got data */
	else
	  {
	     Evas_List l;
	     
	     for (l = cfg_data->elements; l; l = l->next)
	       {
		  List_Element *cfg_element;
		  
		  D("element\n");
		  cfg_element = l->data;
		  D("... name %s\n", cfg_element->name);
		  D("... size %i\n", cfg_element->size);
		  D("... perc %3.3f\n", cfg_element->perc);
	       }
	  }
	exit(0);
     }

   D_RETURN;
}

#endif
