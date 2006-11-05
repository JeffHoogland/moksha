#include "e.h"

//#define DEBUG 1


struct order_data
{
   char order_path[PATH_MAX];
   Ecore_Sheap *sheap;
};


struct category_data
{
   char *path;
   Ecore_Hash *menus;
};

static void  _e_fdo_menu_to_order_make_apps(char *name, char *path, char *directory, Ecore_Hash *apps);
static void  _e_fdo_menu_to_order_dump_each_hash_node(void *value, void *user_data);
static void  _e_fdo_menu_to_order_add_sheap(Ecore_Sheap *sheap, const char *order_path, const char *file);
static void  _e_fdo_menu_to_order_cb_desktop_dir_foreach(void *list_data, void *data);
static void  _e_fdo_menu_to_order_cb_desktop_foreach(void *list_data, void *data);
static char *_e_fdo_menu_to_order_find_category(char *category);
static void  _e_fdo_menu_to_order_dump_each_hash_node2(void *value, void *user_data);

static int menu_count;
static int item_count;

EAPI void
e_fdo_menu_to_order(void)
{
   char dir[PATH_MAX];

   ecore_desktop_instrumentation_reset();
   /* Nuke the old menus. */
   snprintf(dir, sizeof(dir), "%s/.e/e/applications/menu/all/", e_user_homedir_get());
   ecore_file_recursive_rm(dir);
   menu_count = 0;
   item_count = 0;
   ecore_desktop_menu_for_each(_e_fdo_menu_to_order_make_apps);
   ecore_desktop_instrumentation_print();
   /* This is a hueristic to guess if there are not enough apps.  Feel free to tweak it. */
   if ((item_count < 50) || (menu_count > (item_count * 3)))
      {
         struct category_data cat_data;

         /* search out all .desktop files and generate menus ala e17genmenu. */
         ecore_desktop_instrumentation_reset();
         cat_data.menus = ecore_hash_new(ecore_str_hash, ecore_str_compare);
	 if (cat_data.menus)
	    {
	       ecore_hash_set_free_key(cat_data.menus, free);
	       ecore_hash_set_free_value(cat_data.menus, (Ecore_Free_Cb) ecore_hash_destroy);
               ecore_desktop_paths_for_each(ECORE_DESKTOP_PATHS_DESKTOPS, _e_fdo_menu_to_order_cb_desktop_dir_foreach, &cat_data);
               ecore_hash_for_each_node(cat_data.menus, _e_fdo_menu_to_order_dump_each_hash_node2, &cat_data);
	    }
         ecore_desktop_instrumentation_print();
      }
}

static void
_e_fdo_menu_to_order_make_apps(char *name, char *path, char *directory, Ecore_Hash *apps)
{
   struct order_data order_data;

   order_data.sheap = ecore_sheap_new(ecore_str_compare, 100);
   if (order_data.sheap)
      {
         ecore_sheap_set_free_cb(order_data.sheap, free);
         snprintf(order_data.order_path, sizeof(order_data.order_path), "%s/.e/e/applications/menu/all/%s", e_user_homedir_get(), path);
         /* Collect the apps. */
         ecore_hash_for_each_node(apps, _e_fdo_menu_to_order_dump_each_hash_node, &order_data);

         /* Check if we need to create the directory. */
         if ((order_data.sheap->size) && (!ecore_file_exists(order_data.order_path)))
            {
               Ecore_Sheap *sheap;
               const char *temp;

               sheap = ecore_sheap_new(ecore_str_compare, 100);
               temp = ecore_file_get_dir((const char *) order_data.order_path);
	       if ((sheap) && (temp))
	          {
                     ecore_sheap_set_free_cb(sheap, free);
                     ecore_file_mkpath(order_data.order_path);
	             menu_count++;
                     /* If we create a dir, we add it to the parents .order file. */
                     _e_fdo_menu_to_order_add_sheap(sheap, temp, ecore_file_get_file(order_data.order_path));
		  }
	       if (temp)    free((char *) temp);
               if (sheap)   ecore_sheap_destroy(sheap);
            }

         if (ecore_file_exists(order_data.order_path))
	   {
	      if (directory)
	        {
                   char dir[PATH_MAX];

                   snprintf(dir, sizeof(dir), "%s/.directory", order_data.order_path);
                   if ((ecore_file_exists(directory)) && (!ecore_file_exists(dir)))
                      ecore_file_symlink(directory, dir);
	        }
              /* Create the apps. */
              _e_fdo_menu_to_order_add_sheap(order_data.sheap, order_data.order_path, NULL);
              ecore_sheap_destroy(order_data.sheap);
	   }
      }
}

static void
_e_fdo_menu_to_order_dump_each_hash_node(void *value, void *user_data)
{
   struct order_data *order_data;
   Ecore_Hash_Node *node;
   Ecore_Desktop *desktop;
   const char *file, *key;

   order_data = (struct order_data *)user_data;
   node = (Ecore_Hash_Node *) value;
   key = (char *)node->key;
   file = (char *)node->value;
   desktop = ecore_desktop_get(file, NULL);
   /* Check if we process */
   if (!desktop) return;
   if ( (!desktop->hidden) && (!desktop->no_display) && ((desktop->type == NULL) || (strcmp(desktop->type, "Application") == 0)) )
      {
         char path2[PATH_MAX];

#ifdef DEBUG
         printf("MAKING MENU ITEM %s -> %s  (%s)\n", order_data->order_path, file, key);
#endif
         item_count++;
         snprintf(path2, sizeof(path2), "%s/.e/e/applications/all/%s", e_user_homedir_get(), key);
         if (!ecore_file_exists(path2))
            ecore_file_symlink(file, path2);
         ecore_sheap_insert(order_data->sheap, strdup(key));
     }
}

static void
_e_fdo_menu_to_order_add_sheap(Ecore_Sheap *sheap, const char *order_path, const char *file)
{
   char path2[PATH_MAX];
   FILE *f;

   if (file)
      ecore_sheap_insert(sheap, strdup(file));
   snprintf(path2, sizeof(path2), "%s/.order", order_path);
   f = fopen(path2, "r");
   if (f)
      {
        char buffer[PATH_MAX];

        /* Read all entries from existing order file, store in sheap for sorting. */
         while (fgets(buffer, sizeof(buffer), f) != NULL)
            {
               int length;

               /* Strip new line char */
               if (buffer[(length = strlen(buffer) - 1)] == '\n')
                  buffer[length] = '\0';
               ecore_sheap_insert(sheap, strdup(buffer));
            }
         fclose(f);
      }

   f = fopen(path2, "w");
   if (!f)
      fprintf(stderr, "ERROR: Cannot open order file %s \n", path2);
   else
      {
         char *last = NULL;
         int i;

         for (i = 0; i < sheap->size; i++)
            {
               char *item;

               item = ecore_sheap_item(sheap, i);
               if (item)
	          {
		     /* Weed out the dupes. */
		     if ((last) && (strcmp(last, item) == 0))
		        continue;
                     fprintf(f, "%s\n", item);
		     last = item;
		  }
	    }
         fclose(f);
      }
}

static void
_e_fdo_menu_to_order_cb_desktop_dir_foreach(void *list_data, void *data)
{
   char *path = list_data;
   struct category_data *cat_data = data;
   Ecore_List *desktops;

   if(!path)      return;
   if(!cat_data)  return;

   cat_data->path = path;
   desktops = ecore_file_ls(path);
   if(desktops)
      ecore_list_for_each(desktops, _e_fdo_menu_to_order_cb_desktop_foreach, cat_data);
}

static void
_e_fdo_menu_to_order_cb_desktop_foreach(void *list_data, void *data)
{
   const char* filename = list_data;
   struct category_data *cat_data = data;
   char path[PATH_MAX], *ext;
   Ecore_Desktop *desktop = ecore_desktop_get(path, NULL);

   if(!filename)
      return;

   ext = strrchr(filename, '.');
   if ((ext) && (strcmp(ext, ".desktop") == 0))
      {
         snprintf(path, PATH_MAX, "%s/%s", cat_data->path, filename);
         desktop = ecore_desktop_get(path, NULL);
         /* Check if we process */
         if (!desktop) return;
         if ( (!desktop->hidden) && (!desktop->no_display) 
	    && ((desktop->type == NULL) || (strcmp(desktop->type, "Application") == 0)) 
	    && ((desktop->OnlyShowIn == NULL) ||(ecore_hash_get(desktop->OnlyShowIn, "Enlightenment") != NULL))
	    && ((desktop->NotShowIn == NULL) ||(ecore_hash_get(desktop->NotShowIn, "Enlightenment") == NULL)) )
            {
               char *category;
	       Ecore_Hash *menu;

               category = _e_fdo_menu_to_order_find_category(desktop->categories);
	       menu = ecore_hash_get(cat_data->menus, category);
	       if (!menu)
	          {
		     menu = ecore_hash_new(ecore_str_hash, ecore_str_compare);
		     if (menu)
		       {
		          ecore_hash_set_free_key(menu, free);
		          ecore_hash_set_free_value(menu, free);
	                  ecore_hash_set(cat_data->menus, strdup(category), menu);
		       }
	          }

	       if (menu)
                  ecore_hash_set(menu, strdup(filename), strdup(path));
            }
      }
}


// FIXME: There are better ways of dealing with this, just a quick cut'n'paste from e17genmenu for now.

#define CATEGORIES "Accessibility:Accessories:Amusement:AudioVideo:Core:Development:Education:Game:Graphics:Multimedia:Network:Office:Programming:Settings:System:TextEditor:Utility:Video"

static char *
_e_fdo_menu_to_order_find_category(char *category)
{
   char *token, *cat, *categories;

   cat = NULL;
   if (category)
      {
         categories = strdup(CATEGORIES);
         if (categories)
	    {
               token = strtok(categories, ":");
               while (token)
                 {
                    /* Check If this token is in supplied $t */
                    if (strstr(category, token) != NULL)
                      {
                         if (strstr(token, "Development") != NULL)
                           {
                              cat = "Programming";
                           }
                         else if (strstr(token, "Game") != NULL)
                           {
                              cat = "Games";
                           }
                         else if ((strstr(token, "AudioVideo") != NULL) ||
                                  (strstr(token, "Sound") != NULL) || (strstr(token, "Video") != NULL) || (strstr(token, "Multimedia") != NULL))
                           {
                              cat = "Multimedia";
                           }
                         else if (strstr(token, "Net") != NULL)
                           {
                              cat = "Internet";
                           }
                         else if (strstr(token, "Education") != NULL)
                           {
                              cat = "Edutainment";
                           }
                         else if (strstr(token, "Amusement") != NULL)
                           {
                              cat = "Toys";
                           }
                         else if (strstr(token, "System") != NULL)
                           {
                              cat = "System";
                           }
                         else if ((strstr(token, "Shells") != NULL) || (strstr(token, "Utility") != NULL) || (strstr(token, "Tools") != NULL))
                           {
                              cat = "Utilities";
                           }
                         else if ((strstr(token, "Viewers") != NULL) || (strstr(token, "Editors") != NULL) || (strstr(token, "Text") != NULL))
                           {
                              cat = "Editors";
                           }
                         else if (strstr(token, "Graphics") != NULL)
                           {
                              cat = "Graphics";
                           }
                         else if ((strstr(token, "WindowManagers") != NULL) || (strstr(token, "Core") != NULL))
                           {
                              cat = "Core";
                           }
                         else if ((strstr(token, "Settings") != NULL) || (strstr(token, "Accessibility") != NULL))
                           {
                              cat = "Settings";
                           }
                         else if (strstr(token, "Office") != NULL)
                           {
                              cat = "Office";
                           }
                         else
                           {
                              cat = "Core";
                           }
                      }
                    token = strtok(NULL, ":");
                 }
               if (token)
                  free(token);
               free(categories);
	    }
      }
   if (!cat)
      cat = "Core";
   return strdup(cat);
}

static void
_e_fdo_menu_to_order_dump_each_hash_node2(void *value, void *user_data)
{
   Ecore_Hash_Node *node;
   Ecore_Hash *menu;
   char *category;

   node = (Ecore_Hash_Node *) value;
   category = (char *)node->key;
   menu = (Ecore_Hash *)node->value;
   _e_fdo_menu_to_order_make_apps(category, category, "", menu);
}
