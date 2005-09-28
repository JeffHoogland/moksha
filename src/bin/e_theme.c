/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
typedef struct _E_Theme_Result E_Theme_Result;

struct _E_Theme_Result
{
   char *file;
   char *cache;
};

static Evas_Bool _e_theme_mappings_free_cb(Evas_Hash *hash, const char *key, void *data, void *fdata);
static void      _e_theme_category_register(const char *category);
static void      _e_theme_transition_register(const char *transition);

/* local subsystem globals */
static Evas_Hash *mappings = NULL;
static Evas_Hash *group_cache = NULL;

static Evas_List *categories = NULL;
static Evas_List *transitions = NULL;

/* externally accessible functions */

int
e_theme_init(void)
{
   Evas_List *l;
   E_Theme_Result *res;
   char *category, *p;
   
   /* this is a fallback that is ALWAYS there - if all fails things will */
   /* always fall back to the default theme. the rest after this are config */
   /* values users can set */
   e_theme_file_set("base", "default.edj");
   
   for (l = e_config->themes; l; l = l->next)
     {
	E_Config_Theme *et;
	char buf[256];
	
	et = l->data;
	snprintf(buf, sizeof(buf), "base/%s", et->category);
	e_theme_file_set(buf, et->file);
     }

   /* Find transitions */
   category = strdup("base/theme/transitions");
   if (category)
     {
	do
	  {
	     res = evas_hash_find(mappings, category);
	     if (res)
	       {
		  char *str;

		  /* if found check cached path */
		  str = res->cache;
		  if (!str)
		    {
		       /* no cached path */
		       str = res->file;
		       /* if its not an absolute path find it */
		       if (str[0] != '/')
			 str = e_path_find(path_themes, str);
		       /* save cached value */
		       if (str) res->cache = str;
		    }
		  if (str)
		    {
		       Evas_List *coll, *l;
		       coll = edje_file_collection_list(str);
		       if (coll)
			 {
			    for (l = coll; l; l = l->next)
			      {
				 if (!strncmp(l->data, "transitions", 11))
				   {
				      char *trans;

				      trans = strdup(l->data);
				      p = strchr(trans, '/');
				      if (p)
					{
					   p++;
					   _e_theme_transition_register(p);
					}
				      free(trans);
				   }
			      }
			    edje_file_collection_list_free(coll);
			 }
		    }
	       }
	     p = strrchr(category, '/');
	     if (p) *p = 0;
	  }
	while (p);
	free(category);
     }

   return 1;
}

/*
 * this is used to sewt the theme for a CATEGORY of e17. "base" is always set
 * to the default theme - because if a selected theme for lest say base/theme
 * does not provide theme elements it can default back to the default theme.
 * 
 * the idea is you can actually set a different theme for different parts of
 * the desktop... :)
 * 
 * other possible categories...
 *  e_theme_file_set("base/theme/borders", "default.edj");
 *  e_theme_file_set("base/theme/background", "default.edj");
 *  e_theme_file_set("base/theme/menus", "default.edj");
 *  e_theme_file_set("base/theme/error", "default.edj");
 *  e_theme_file_set("base/theme/gadman", "default.edj");
 *  e_theme_file_set("base/theme/dnd", "default.edj");
 *  e_theme_file_set("base/theme/icons", "default.edj");
 *  e_theme_file_set("base/theme/modules", "default.edj");
 *  e_theme_file_set("base/theme/modules/pager", "default.edj");
 *  e_theme_file_set("base/theme/modules/ibar", "default.edj");
 *  e_theme_file_set("base/theme/modules/clock", "default.edj");
 *  e_theme_file_set("base/theme/modules/battery", "default.edj");
 *  e_theme_file_set("base/theme/modules/cpufreq", "default.edj");
 *  e_theme_file_set("base/theme/modules/temperature", "default.edj");
 */

int
e_theme_shutdown(void)
{
   if (mappings)
     {
	evas_hash_foreach(mappings, _e_theme_mappings_free_cb, NULL);
	evas_hash_free(mappings);
	mappings = NULL;
     }
   if (group_cache)
     {
	evas_hash_free(group_cache);
	group_cache = NULL;
     }
   while (categories)
     {
	free(categories->data);
	categories = evas_list_remove_list(categories, categories);
     }
   while (transitions)
     {
	free(transitions->data);
	transitions = evas_list_remove_list(transitions, transitions);
     }
   return 1;
}

int
e_theme_edje_object_set(Evas_Object *o, char *category, char *group)
{
   E_Theme_Result *res;
   char buf[256];
   char *p;

   /* find category -> edje mapping */
   _e_theme_category_register(category);
   res = evas_hash_find(mappings, category);
   if (res)
     {
	char *str;
	
	/* if found check cached path */
	str = res->cache;
	if (!str)
	  {
	     /* no cached path */
	     str = res->file;
	     /* if its not an absolute path find it */
	     if (str[0] != '/')
	       str = e_path_find(path_themes, str);
	     /* save cached value */
	     if (str) res->cache = str;
	  }
	if (str)
	  {
	     void *tres;
	     int ok;
	     
	     snprintf(buf, sizeof(buf), "%s/::/%s", str, group);
	     tres = evas_hash_find(group_cache, buf);
	     if (!tres)
	       {
		  ok = edje_object_file_set(o, str, group);
                  /* save in the group cache hash */
		  if (ok)
		    group_cache = evas_hash_add(group_cache, buf, res);
		  else
		    group_cache = evas_hash_add(group_cache, buf, (void *)1);
	       }
	     else if (tres == (void *)1)
	       ok = 0;
	     else
	       ok = 1;
	     if (ok)
	       {
		  if (tres)
		    edje_object_file_set(o, str, group);
		  return 1;
	       }
	  }
     }
   /* no mapping or set failed - fall back */
   strncpy(buf, category, sizeof(buf) - 1);
   buf[sizeof(buf) - 1] = 0;
   /* shorten string up to and not including last / char */
   p = strrchr(buf, '/');
   if (p) *p = 0;
   /* no / anymore - we are already as far back as we can go */
   else return 0;
   /* try this category */
   return e_theme_edje_object_set(o, buf, group);
}

const char *
e_theme_edje_file_get(char *category, char *group)
{
   E_Theme_Result *res;
   char buf[4096];
   char *p;

   /* find category -> edje mapping */
   _e_theme_category_register(category);
   res = evas_hash_find(mappings, category);
   if (res)
     {
	char *str;
	
	/* if found check cached path */
	str = res->cache;
	if (!str)
	  {
	     /* no cached path */
	     str = res->file;
	     /* if its not an absolute path find it */
	     if (str[0] != '/')
	       str = e_path_find(path_themes, str);
	     /* save cached value */
	     if (str) res->cache = str;
	  }
	if (str)
	  {
	     void *tres;
	     Evas_List *coll, *l;
	     int ok;
	     
	     snprintf(buf, sizeof(buf), "%s/::/%s", str, group);
	     tres = evas_hash_find(group_cache, buf);
	     if (!tres)
	       {
		  /* if the group exists - return */
		  coll = edje_file_collection_list(str);
		  ok = 0;
		  for (l = coll; l; l = l->next)
		    {
		       if (!strcmp(l->data, group))
			 {
			    ok = 1;
			    break;
			 }
		    }
		  if (coll) edje_file_collection_list_free(coll);
		  /* save in the group cache hash */
		  if (ok)
		    group_cache = evas_hash_add(group_cache, buf, res);
		  else
		    group_cache = evas_hash_add(group_cache, buf, (void *)1);
	       }
	     else if (tres == (void *)1) /* special pointer "1" == not there */
	       ok = 0;
	     else
	       ok = 1;
	     if (ok) return str;
	  }
     }
   /* no mapping or set failed - fall back */
   strncpy(buf, category, sizeof(buf) - 1);
   buf[sizeof(buf) - 1] = 0;
   /* shorten string up to and not including last / char */
   p = strrchr(buf, '/');
   if (p) *p = 0;
   /* no / anymore - we are already as far back as we can go */
   else return "";
   /* try this category */
   return e_theme_edje_file_get(buf, group);
}

void
e_theme_file_set(char *category, char *file)
{
   E_Theme_Result *res;

   if (group_cache)
     {
	evas_hash_free(group_cache);
	group_cache = NULL;
     }
   _e_theme_category_register(category);
   res = evas_hash_find(mappings, category);
   if (res)
     {
	mappings = evas_hash_del(mappings, category, res);
	E_FREE(res->file);
	E_FREE(res->cache);
	free(res);
     }
   res = calloc(1, sizeof(E_Theme_Result));
   res->file = strdup(file);
   mappings = evas_hash_add(mappings, category, res);
}

int
e_theme_config_set(const char *category, const char *file)
{
   E_Config_Theme *ect;
   Evas_List *next;

   /* Don't accept unused categories */
   if (!e_theme_category_find(category)) return 0;

   /* search for the category */
   for (next = e_config->themes; next; next = next->next)
     {
	ect = evas_list_data(next);
	if (!strcmp(ect->category, category))
	  {
	     E_FREE(ect->file);
	     ect->file = strdup(file);
	     return 1;
	  }
     }

   /* the text class doesnt exist */
   ect = E_NEW(E_Config_Theme, 1);
   ect->category = strdup(category);
   ect->file = strdup(file);
   
   e_config->themes = evas_list_append(e_config->themes, ect);
   return 1;
}

/*
 * returns a pointer to the data, return null if nothing if found.
 */
E_Config_Theme *
e_theme_config_get(const char *category)
{
   E_Config_Theme *ect = NULL;
   Evas_List *next;

   /* search for the category */
   for (next = e_config->themes; next; next = next->next)
     {
	ect = evas_list_data(next);
	if (!strcmp(ect->category, category))
	  {
	     return ect;
	  }
     }
   return NULL;
}

int
e_theme_config_remove(const char *category)
{
   E_Config_Theme *ect;
   Evas_List *next;
   
   /* search for the category */
   for (next = e_config->themes; next; next = next->next)
     {
	ect = evas_list_data(next);
	if (!strcmp(ect->category, category))
	  {
	     e_config->themes = evas_list_remove_list(
					e_config->themes, next);
	     E_FREE(ect->category);
	     E_FREE(ect->file);
	     E_FREE(ect);
	     return 1;
	  }
    }
   return 1;
}

Evas_List *
e_theme_config_list(void)
{
   return e_config->themes;
}

int
e_theme_category_find(const char *category)
{
   Evas_List *l;

   for (l = categories; l; l = l->next)
     {
	if (!strcmp(category, l->data))
	  return 1;
     }
   return 0;
}

Evas_List *
e_theme_category_list(void)
{
   return categories;
}

int
e_theme_transition_find(const char *transition)
{
   Evas_List *l;

   for (l = transitions; l; l = l->next)
     {
	if (!strcmp(transition, l->data))
	  return 1;
     }
   return 0;
}

Evas_List *
e_theme_transition_list(void)
{
   return transitions;
}

/* local subsystem functions */

static Evas_Bool
_e_theme_mappings_free_cb(Evas_Hash *hash, const char *key, void *data, void *fdata)
{
   E_Theme_Result *res;
   
   res = data;
   E_FREE(res->file);
   E_FREE(res->cache);
   free(res);
   return 1;
}

static void
_e_theme_category_register(const char *category)
{
   Evas_List *l;

   for (l = categories; l; l = l->next)
     {
	if (!strcmp(category, l->data))
	  return;
     }

   categories = evas_list_append(categories, strdup(category));
}

static void
_e_theme_transition_register(const char *transition)
{
   Evas_List *l;

   for (l = transitions; l; l = l->next)
     {
	if (!strcmp(transition, l->data))
	  return;
     }

   transitions = evas_list_append(transitions, strdup(transition));
}
