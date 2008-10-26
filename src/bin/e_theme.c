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
   Evas_Hash  *quickfind;
};

static Evas_Bool _e_theme_mappings_free_cb(const Evas_Hash *hash, const void *key, void *data, void *fdata);
static Evas_Bool _e_theme_mappings_quickfind_free_cb(const Evas_Hash *hash, const void *key, void *data, void *fdata);
static void      _e_theme_category_register(const char *category);
static Eina_List *_e_theme_collection_item_register(Eina_List *list, const char *name);
static Eina_List *_e_theme_collection_items_find(const char *base, const char *collname);


/* local subsystem globals */
static Evas_Hash *mappings = NULL;
static Evas_Hash *group_cache = NULL;

static Eina_List *categories = NULL;
static Eina_List *transitions = NULL;
static Eina_List *borders = NULL;
static Eina_List *shelfs = NULL;
static E_Fm2_Mime_Handler *theme_hdl = NULL;

/* externally accessible functions */

EAPI int
e_theme_init(void)
{
   Eina_List *l = NULL;

   /* Register mime handler */
   theme_hdl = e_fm2_mime_handler_new(_("Set As Theme"), "enlightenment/themes", 
				      e_theme_handler_set, NULL, 
				      e_theme_handler_test, NULL);
   if (theme_hdl) e_fm2_mime_handler_glob_add(theme_hdl, "*.edj");

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
   transitions = _e_theme_collection_items_find("base/theme/transitions", "e/transitions");
   borders = _e_theme_collection_items_find("base/theme/borders", "e/widgets/border");
   shelfs = _e_theme_collection_items_find("base/theme/shelf", "e/shelf");

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
   if (theme_hdl) 
     {
	e_fm2_mime_handler_glob_del(theme_hdl, "*.edj");
	e_fm2_mime_handler_free(theme_hdl);
     }
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
	eina_stringshare_del(categories->data);
	categories = eina_list_remove_list(categories, categories);
     }
   while (transitions)
     {
	eina_stringshare_del(transitions->data);
	transitions = eina_list_remove_list(transitions, transitions);
     }
   while (borders)
     {
	eina_stringshare_del(borders->data);
	borders = eina_list_remove_list(borders, borders);
     }
   while (shelfs)
     {
	eina_stringshare_del(shelfs->data);
	shelfs = eina_list_remove_list(shelfs, shelfs);
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
   const char *q;
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
	     Eina_List *coll, *l;
	     int ok;
	     
	     snprintf(buf, sizeof(buf), "%s/::/%s", str, group);
	     tres = evas_hash_find(group_cache, buf);
	     if (!tres)
	       {
		  /* if the group exists - return */
		  if (!res->quickfind)
		    {
		       /* great a quick find hash of all group entires */
		       coll = edje_file_collection_list(str);
		       for (l = coll; l; l = l->next)
			 {
			    q = eina_stringshare_add(l->data);
			    res->quickfind = evas_hash_direct_add(res->quickfind, q, q);
			 }
		       if (coll) edje_file_collection_list_free(coll);
		    }
		  /* save in the group cache hash */
		  if (evas_hash_find(res->quickfind, group))
		    {
		       group_cache = evas_hash_add(group_cache, buf, res);
		       ok = 1;
		    }
		  else
		    {
		       group_cache = evas_hash_add(group_cache, buf, (void *)1);
		       ok = 0;
		    }
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
	if (res->file) 
	  {
	     e_filereg_deregister(res->file);
	     eina_stringshare_del(res->file);
	  }
	if (res->cache) eina_stringshare_del(res->cache);
	free(res);
     }
   res = calloc(1, sizeof(E_Theme_Result));
   res->file = eina_stringshare_add(file);
   e_filereg_register(res->file);
   mappings = evas_hash_add(mappings, category, res);
}

EAPI int
e_theme_config_set(const char *category, const char *file)
{
   E_Config_Theme *ect;
   Eina_List *next;

   /* Don't accept unused categories */
#if 0
   if (!e_theme_category_find(category)) return 0;
#endif

   /* search for the category */
   for (next = e_config->themes; next; next = next->next)
     {
	ect = eina_list_data_get(next);
	if (!strcmp(ect->category, category))
	  {
	     if (ect->file) eina_stringshare_del(ect->file);
	     ect->file = eina_stringshare_add(file);
	     return 1;
	  }
     }

   /* the text class doesnt exist */
   ect = E_NEW(E_Config_Theme, 1);
   ect->category = eina_stringshare_add(category);
   ect->file = eina_stringshare_add(file);
   
   e_config->themes = eina_list_append(e_config->themes, ect);
   return 1;
}

/*
 * returns a pointer to the data, return null if nothing if found.
 */
EAPI E_Config_Theme *
e_theme_config_get(const char *category)
{
   E_Config_Theme *ect = NULL;
   Eina_List *next;

   /* search for the category */
   for (next = e_config->themes; next; next = next->next)
     {
	ect = eina_list_data_get(next);
	if (!strcmp(ect->category, category))
	  return ect;
     }
   return NULL;
}

EAPI int
e_theme_config_remove(const char *category)
{
   E_Config_Theme *ect;
   Eina_List *next;
   
   /* search for the category */
   for (next = e_config->themes; next; next = next->next)
     {
	ect = eina_list_data_get(next);
	if (!strcmp(ect->category, category))
	  {
	     e_config->themes = eina_list_remove_list(e_config->themes, next);
	     if (ect->category) eina_stringshare_del(ect->category);
	     if (ect->file) eina_stringshare_del(ect->file);
	     free(ect);
	     return 1;
	  }
    }
   return 1;
}

EAPI Eina_List *
e_theme_config_list(void)
{
   return e_config->themes;
}

EAPI int
e_theme_category_find(const char *category)
{
   Eina_List *l;

   for (l = categories; l; l = l->next)
     {
	if (!strcmp(category, l->data))
	  return 1;
     }
   return 0;
}

EAPI Eina_List *
e_theme_category_list(void)
{
   return categories;
}

EAPI int
e_theme_transition_find(const char *transition)
{
   Eina_List *l;

   for (l = transitions; l; l = l->next)
     {
	if (!strcmp(transition, l->data))
	  return 1;
     }
   return 0;
}

EAPI Eina_List *
e_theme_transition_list(void)
{
   return transitions;
}

EAPI int
e_theme_border_find(const char *border)
{
   Eina_List *l;

   for (l = borders; l; l = l->next)
     {
	if (!strcmp(border, l->data))
	  return 1;
     }
   return 0;
}

EAPI Eina_List *
e_theme_border_list(void)
{
   return borders;
}

EAPI int
e_theme_shelf_find(const char *shelf)
{
   Eina_List *l;

   for (l = shelfs; l; l = l->next)
     {
	if (!strcmp(shelf, l->data))
	  return 1;
     }
   return 0;
}

EAPI Eina_List *
e_theme_shelf_list(void)
{
   return shelfs;
}

EAPI void 
e_theme_handler_set(Evas_Object *obj, const char *path, void *data) 
{
   E_Action *a;
   
   if (!path) return;
   e_theme_config_set("theme", path);
   e_config_save_queue();
   a = e_action_find("restart");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
}

EAPI int 
e_theme_handler_test(Evas_Object *obj, const char *path, void *data) 
{
   if (!path) return 0;
   if (!edje_file_group_exists(path, "e/widgets/border/default/border")) 
     return 0;
   return 1;
}

/* local subsystem functions */
static Evas_Bool
_e_theme_mappings_free_cb(const Evas_Hash *hash, const void *key, void *data, void *fdata)
{
   E_Theme_Result *res;
   
   res = data;
   if (res->file) eina_stringshare_del(res->file);
   if (res->cache) eina_stringshare_del(res->cache);
   if (res->quickfind)
     {
	evas_hash_foreach(res->quickfind, _e_theme_mappings_quickfind_free_cb, NULL);
	evas_hash_free(res->quickfind);
     }
   free(res);
   return 1;
}

static Evas_Bool
_e_theme_mappings_quickfind_free_cb(const Evas_Hash *hash, const void *key, void *data, void *fdata)
{
   eina_stringshare_del(key);
   return 1;
}

static void
_e_theme_category_register(const char *category)
{
   Eina_List *l;

   for (l = categories; l; l = l->next)
     {
	if (!strcmp(category, l->data)) return;
     }

   categories = eina_list_append(categories, eina_stringshare_add(category));
}

static Eina_List *
_e_theme_collection_item_register(Eina_List *list, const char *name)
{
   Eina_List *l;

   for (l = list; l; l = l->next)
     {
	if (!strcmp(name, l->data)) return list;
     }
   list = eina_list_append(list, eina_stringshare_add(name));
   return list;
}

static Eina_List *
_e_theme_collection_items_find(const char *base, const char *collname)
{
   Eina_List *list = NULL;
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
		  Eina_List *coll, *l;
		  
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
