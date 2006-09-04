#include "e.h"

//#define DEBUG 1

//extern int menu_count, item_count;

static int _e_menu_make_apps(const void *data, Ecore_Desktop_Tree * tree, int element, int level);
static void _e_menu_dump_each_hash_node(void *value, void *user_data);
static int _e_search_list(Ecore_List *list, const char *search);

EAPI void
e_fdo_menu_to_order(void)
{
   int i;
   /* NEW STRATEGY -
    *
    *   if there is no applications.menu
    *     try debian-menu.menu instead.
    *   if all else fails
    *     run debians funky menu generator shit.
    */
   char *menu_file;
   
   /* Should rm -fr ~/.e/e/applications/menu/all to make sure we start from a clean slate. */
   /* Find the main menu file. */
   menu_file = ecore_desktop_paths_file_find(ecore_desktop_paths_menus, 
						  "applications.menu", -1, NULL, NULL);
   if (!menu_file)
      {
         /* Try various quirks of various systems. */
         menu_file = ecore_desktop_paths_file_find(ecore_desktop_paths_menus, 
						  "debian-menu.menu", -1, NULL, NULL);
      }

   if (menu_file)
      {
         char *path;
	     
	 path = ecore_file_get_dir(menu_file);
	 if (path)
	    {
	       Ecore_Desktop_Tree *menus;
		  
	       /* convert the xml into menus */
	       menus = ecore_desktop_menu_get(menu_file);
	       if (menus)
	          {
		     /* create the .eap and order files from the menu */
		     ecore_desktop_tree_foreach(menus, 0, _e_menu_make_apps, path);
// FIXME: Can't free this just yet, causes major memory corruption.
//		     ecore_desktop_tree_del(menus);
		  }
	       free(path);
	       
	    }
	 free(menu_file);
      }
}

static int
_e_menu_make_apps(const void *data, Ecore_Desktop_Tree * tree, int element, int level)
{
   if (tree->elements[element].type == ECORE_DESKTOP_TREE_ELEMENT_TYPE_STRING)
     {
        if (strncmp((char *)tree->elements[element].element, "<MENU ", 6) == 0)
          {
             char *path;
             Ecore_Hash *apps;

#ifdef DEBUG
             char *name;

             name = (char *)tree->elements[element].element;
             printf("MAKING MENU - %s \t\t%s\n", path, name);
#endif
             path = (char *)tree->elements[element + 1].element;
//             pool = (Ecore_Hash *) tree->elements[element + 2].element;
             apps = (Ecore_Hash *) tree->elements[element + 4].element;
//             menu_count++;
             ecore_hash_for_each_node(apps, _e_menu_dump_each_hash_node, &path[11]);
          }
     }
   return 0;
}

static void
_e_menu_dump_each_hash_node(void *value, void *user_data)
{
   Ecore_Hash_Node *node;
   Ecore_Desktop *desktop;
   const char *file, *path;

   path = (char *)user_data;
   node = (Ecore_Hash_Node *) value;
   file = (char *)node->value;
#ifdef DEBUG
   printf("MAKING MENU ITEM %s -> %s\n", path, file);
#endif
//   item_count++;
   desktop = ecore_desktop_get(file, NULL);
   /* Check If We Process */
   if ( (!desktop->hidden) && (!desktop->no_display) && ((desktop->type == NULL) || (strcmp(desktop->type, "Application") == 0)) )
      {
         char order_path[PATH_MAX];
         int length;
         char *buff;
         char buffer[PATH_MAX], path2[PATH_MAX];
         FILE *f;
         Ecore_List *list = NULL;

         snprintf(path2, sizeof(path2), "%s/.e/e/applications/all/%s", ecore_desktop_home_get(), ecore_file_get_file(file));
         if (!ecore_file_exists(path2))
            ecore_file_symlink(file, path2);
         file = ecore_file_get_file(file);
         snprintf(order_path, sizeof(order_path), "%s/.e/e/applications/menu/all/%s", ecore_desktop_home_get(), path);
         if (!ecore_file_exists(order_path))
            {
               const char *cat;

               ecore_file_mkpath(order_path);
// FIXME: If we create a dir, we should add it to the parents .order file.
	       cat = ecore_file_get_file(order_path);
	       if (cat)
	          {
// FIXME: What to do about .directory.eap's when .desktop takes over?
//                   create_dir_eap(order_path, cat);
		  }
            }

          snprintf(path2, sizeof(path2), "%s/.order", order_path);

#ifdef DEBUG
          fprintf(stderr, "Modifying Order File %s\n", path2);
#endif

          list = ecore_list_new();
	  ecore_list_set_free_cb(list, free);

          /* Stat .order; Create If Not Found */
          if (!ecore_file_exists(path2))
            {
               FILE *f2;

#ifdef DEBUG
               fprintf(stderr, "Creating Order File %s\n", path2);
#endif

               f2 = fopen(path2, "w");
               if (!f2)
               {
                  fprintf(stderr, "ERROR: Cannot Create Order File %s\n", path2);
                  exit(-1);
               }
               fclose(f2);
               /* If We Had To Create This Order Then Just Add The file */
               if (!ecore_list_append(list, strdup(file)))
                 {
                    fprintf(stderr, "ERROR: Ecore List Append Failed !!\n");
                    return;
                 }
            }
          else
            {
               /* Open .order File For Parsing */
               f = fopen(path2, "r");
               if (!f)
                 {
                    fprintf(stderr, "ERROR: Cannot Open Order File %s \n", path2);
                    exit(-1);
                 }

               /* Read All Entries From Existing Order File, Store In List For Sorting */
               while (fgets(buffer, sizeof(buffer), f) != NULL)
                 {
                    /* Strip New Line Char */
                    if (buffer[(length = strlen(buffer) - 1)] == '\n')
                       buffer[length] = '\0';
                    if (!_e_search_list(list, buffer))
                      {
                         if (!ecore_list_append(list, strdup(buffer)))
                           {
                              fprintf(stderr, "ERROR: Ecore List Append Failed !!\n");
                              return;
                           }
                      }
                 }
               fclose(f);
               buffer[0] = (char)0;

               /* Add This file To List Of Existing ? */
               if (!_e_search_list(list, file))
                 {
                    if (!ecore_list_append(list, strdup(file)))
                      {
                         fprintf(stderr, "ERROR: Ecore List Append Failed !!\n");
                         return;
                      }
                 }
            }

#ifdef DEBUG
          fprintf(stderr, "Rewriting Order %s\n", path2);
#endif

          f = fopen(path2, "w");
          if (!f)
            {
               fprintf(stderr, "ERROR: Cannot Open Order File %s \n", path2);
               if (list)
                  ecore_list_destroy(list);
               return;
            }

          ecore_list_goto_first(list);
          while ((buff = ecore_list_next(list)) != NULL)
            {
               snprintf(buffer, sizeof(buffer), "%s\n", buff);
               if (buffer != NULL)
                  fwrite(buffer, sizeof(char), strlen(buffer), f);
            }
          fclose(f);

          if (list)
             ecore_list_destroy(list);
     }
}


static int
_e_search_list(Ecore_List *list, const char *search)
{
   char *tmp;

   if (!search) return 0;
   if (!list) return 0;
   ecore_list_goto_first(list);
   while ((tmp = (char *)ecore_list_next(list)) != NULL)
     {
        if (!strcmp(tmp, search))
           return 1;
     }
   return 0;
}
