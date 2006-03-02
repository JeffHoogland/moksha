/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
typedef struct _E_Theme_Result E_Theme_Result;

struct _E_Theme_Result
{
   const char *file;
   const char *cache;
};

static Evas_Bool _e_theme_mappings_free_cb(Evas_Hash *hash, const char *key, void *data, void *fdata);
static void      _e_theme_category_register(const char *category);
static Evas_List *_e_theme_collection_item_register(Evas_List *list, const char *name);
static Evas_List *_e_theme_collection_items_find(const char *base, const char *collname);


/* local subsystem globals */
static Evas_Hash *mappings = NULL;
static Evas_Hash *group_cache = NULL;

static Evas_List *categories = NULL;
static Evas_List *transitions = NULL;
static Evas_List *borders = NULL;

/* externally accessible functions */

EAPI int
e_theme_init(void)
{
   Evas_List *l;
   
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
   transitions = _e_theme_collection_items_find("base/theme/transitions", "transitions");
   borders = _e_theme_collection_items_find("base/theme/borders", "widgets/border");

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
 *  e_theme_file_set("base/theme/about", "default.edj");
 *  e_theme_file_set("base/theme/borders", "default.edj");
 *  e_theme_file_set("base/theme/background", "default.edj");
 *  e_theme_file_set("base/theme/configure", "default.edj");
 *  e_theme_file_set("base/theme/dialog", "default.edj");
 *  e_theme_file_set("base/theme/menus", "default.edj");
 *  e_theme_file_set("base/theme/error", "default.edj");
 *  e_theme_file_set("base/theme/gadman", "default.edj");
 *  e_theme_file_set("base/theme/dnd", "default.edj");
 *  e_theme_file_set("base/theme/icons", "default.edj");
 *  e_theme_file_set("base/theme/pointer", "default.edj");
 *  e_theme_file_set("base/theme/transitions", "default.edj");
 *  e_theme_file_set("base/theme/widgets", "default.edj");
 *  e_theme_file_set("base/theme/winlist", "default.edj");
 *  e_theme_file_set("base/theme/modules", "default.edj");
 *  e_theme_file_set("base/theme/modules/pager", "default.edj");
 *  e_theme_file_set("base/theme/modules/ibar", "default.edj");
 *  e_theme_file_set("base/theme/modules/ibox", "default.edj");
 *  e_theme_file_set("base/theme/modules/clock", "default.edj");
 *  e_theme_file_set("base/theme/modules/battery", "default.edj");
 *  e_theme_file_set("base/theme/modules/cpufreq", "default.edj");
 *  e_theme_file_set("base/theme/modules/start", "default.edj");
 *  e_theme_file_set("base/theme/modules/temperature", "default.edj");
 */

EAPI int
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
	evas_stringshare_del(categories->data);
	categories = evas_list_remove_list(categories, categories);
     }
   while (transitions)
     {
	evas_stringshare_del(transitions->data);
	transitions = evas_list_remove_list(transitions, transitions);
     }
   while (borders)
     {
	evas_stringshare_del(borders->data);
	borders = evas_list_remove_list(borders, borders);
     }
   return 1;
}

EAPI int
e_theme_edje_object_set(Evas_Object *o, const char *category, const char *group)
{
   E_Theme_Result *res;
   char buf[256];
   char *p;

   /* find category -> edje mapping */
   _e_theme_category_register(category);
   res = evas_hash_find(mappings, category);
   if (res)
     {
	const char *str;
	
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

EAPI const char *
e_theme_edje_file_get(const char *category, const char *group)
{
   E_Theme_Result *res;
   char buf[4096];
   char *p;

   /* find category -> edje mapping */
   _e_theme_category_register(category);
   res = evas_hash_find(mappings, category);
   if (res)
     {
	const char *str;
	
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

EAPI void
e_theme_file_set(const char *category, const char *file)
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
	if (res->file) evas_stringshare_del(res->file);
	if (res->cache) evas_stringshare_del(res->cache);
	free(res);
     }
   res = calloc(1, sizeof(E_Theme_Result));
   res->file = evas_stringshare_add(file);
   mappings = evas_hash_add(mappings, category, res);
}

EAPI int
e_theme_config_set(const char *category, const char *file)
{
   E_Config_Theme *ect;
   Evas_List *next;

   /* Don't accept unused categories */
#if 0
   if (!e_theme_category_find(category)) return 0;
#endif

   /* search for the category */
   for (next = e_config->themes; next; next = next->next)
     {
	ect = evas_list_data(next);
	if (!strcmp(ect->category, category))
	  {
	     if (ect->file) evas_stringshare_del(ect->file);
	     ect->file = evas_stringshare_add(file);
	     return 1;
	  }
     }

   /* the text class doesnt exist */
   ect = E_NEW(E_Config_Theme, 1);
   ect->category = evas_stringshare_add(category);
   ect->file = evas_stringshare_add(file);
   
   e_config->themes = evas_list_append(e_config->themes, ect);
   return 1;
}

/*
 * returns a pointer to the data, return null if nothing if found.
 */
EAPI E_Config_Theme *
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

EAPI int
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
	     if (ect->category) evas_stringshare_del(ect->category);
	     if (ect->file) evas_stringshare_del(ect->file);
	     free(ect);
	     return 1;
	  }
    }
   return 1;
}

EAPI Evas_List *
e_theme_config_list(void)
{
   return e_config->themes;
}

EAPI int
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

EAPI Evas_List *
e_theme_category_list(void)
{
   return categories;
}

EAPI int
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

EAPI Evas_List *
e_theme_transition_list(void)
{
   return transitions;
}

EAPI int
e_theme_border_find(const char *border)
{
   Evas_List *l;

   for (l = borders; l; l = l->next)
     {
	if (!strcmp(border, l->data))
	  return 1;
     }
   return 0;
}

EAPI Evas_List *
e_theme_border_list(void)
{
   return borders;
}

/* local subsystem functions */

static Evas_Bool
_e_theme_mappings_free_cb(Evas_Hash *hash, const char *key, void *data, void *fdata)
{
   E_Theme_Result *res;
   
   res = data;
   if (res->file) evas_stringshare_del(res->file);
   if (res->cache) evas_stringshare_del(res->cache);
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

   categories = evas_list_append(categories, evas_stringshare_add(category));
}

static Evas_List *
_e_theme_collection_item_register(Evas_List *list, const char *name)
{
   Evas_List *l;

   for (l = list; l; l = l->next)
     {
	if (!strcmp(name, l->data)) return list;
     }
   list = evas_list_append(list, evas_stringshare_add(name));
   return list;
}

static Evas_List *
_e_theme_collection_items_find(const char *base, const char *collname)
{
   Evas_List *list = NULL;
   E_Theme_Result *res;
   char *category, *p, *p2;
   int collname_len;
   
   collname_len = strlen(collname);
   category = alloca(strlen(base) + 1);
   strcpy(category, base);
   do
     {
	res = evas_hash_find(mappings, category);
	if (res)
	  {
	     const char *str;
	     
	     /* if found check cached path */
	     str = res->cache;
	     if (!str)
	       {
		  /* no cached path */
		  str = res->file;
		  /* if its not an absolute path find it */
		  if (str[0] != '/') str = e_path_find(path_themes, str);
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
			    if (!strncmp(l->data, collname, collname_len))
			      {
				 char *trans;
				 
				 trans = strdup(l->data);
				 p = trans + collname_len + 1;
				 if (*p)
				   {
				      p2 = strchr(p, '/');
				      if (p2) *p2 = 0;
				      list = _e_theme_collection_item_register(list, p);
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
   return list;
}
