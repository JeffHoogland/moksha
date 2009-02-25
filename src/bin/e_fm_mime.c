/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local types */
typedef struct _E_Fm2_Mime_Handler_Tuple E_Fm2_Mime_Handler_Tuple;
struct _E_Fm2_Mime_Handler_Tuple
{
   Eina_List *list;
   const char *str;
};

/* local subsystem functions */
static Eina_Bool _e_fm2_mime_handler_glob_match_foreach(const Eina_Hash *hash __UNUSED__, const void *key, void *data, void *fdata);
static Eina_Bool _e_fm_mime_icon_foreach(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata);

static Eina_Hash *icon_map = NULL;
static Eina_Hash *_mime_handlers = NULL;
static Eina_Hash *_glob_handlers = NULL;

/* externally accessible functions */
EAPI const char *
e_fm_mime_filename_get(const char *fname)
{
   return efreet_mime_globs_type_get(fname);
}

/* returns:
 * NULL == don't know
 * "THUMB" == generate a thumb
 * "e/icons/fileman/mime/..." == theme icon
 * "/path/to/file....edj" = explicit icon edje file
 * "/path/to/file..." = explicit image file to use
 */
EAPI const char *
e_fm_mime_icon_get(const char *mime)
{
   char buf[4096], buf2[4096], *val;
   const char *homedir = NULL;
   Eina_List *l = NULL;
   E_Config_Mime_Icon *mi;

   /* 0.0 clean out hash cache once it has mroe than 512 entries in it */
   if (eina_hash_population(icon_map) > 512) e_fm_mime_icon_cache_flush();

   /* 0. look in mapping cache */
   val = eina_hash_find(icon_map, mime);
   if (val) return val;

   ecore_strlcpy(buf2, mime, sizeof(buf2));
   val = strchr(buf2, '/');
   if (val) *val = 0;

   /* 1. look up in mapping to file or thumb (thumb has flag)*/
   for (l = e_config->mime_icons; l; l = l->next)
     {
	mi = l->data;
	if (e_util_glob_match(mi->mime, mime))
	  {
	     ecore_strlcpy(buf, mi->icon, sizeof(buf));
	     goto ok;
	  }
     }
   
   /* 2. look up in ~/.e/e/icons */
   homedir = e_user_homedir_get();

   snprintf(buf, sizeof(buf), "%s/.e/e/icons/%s.edj", homedir, mime);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/.e/e/icons/%s.svg", homedir, mime);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/.e/e/icons/%s.png", homedir, mime);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/.e/e/icons/%s.edj", homedir, buf2);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/.e/e/icons/%s.svg", homedir, buf2);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/.e/e/icons/%s.png", homedir, buf2);
   if (ecore_file_exists(buf)) goto ok;
   
   /* 3. look up icon in theme */
   snprintf(buf, sizeof(buf), "e/icons/fileman/mime/%s", mime);
   val = (char *)e_theme_edje_file_get("base/theme/fileman", buf);
   if ((val) && (e_util_edje_collection_exists(val, buf))) goto ok;
   snprintf(buf, sizeof(buf), "e/icons/fileman/mime/%s", buf2);
   val = (char *)e_theme_edje_file_get("base/theme/fileman", buf);
   if ((val) && (e_util_edje_collection_exists(val, buf))) goto ok;
   
   /* 4. look up icon in PREFIX/share/enlightent/data/icons */
   snprintf(buf, sizeof(buf), "%s/data/icons/%s.edj", e_prefix_data_get(), mime);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/data/icons/%s.svg", e_prefix_data_get(), mime);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/data/icons/%s.png", e_prefix_data_get(), mime);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/data/icons/%s.edj", e_prefix_data_get(), buf2);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/data/icons/%s.svg", e_prefix_data_get(), buf2);
   if (ecore_file_exists(buf)) goto ok;
   snprintf(buf, sizeof(buf), "%s/data/icons/%s.png", e_prefix_data_get(), buf2);
   if (ecore_file_exists(buf)) goto ok;

   return NULL;

   ok:
   val = (char *)eina_stringshare_add(buf);
   if (!icon_map) icon_map = eina_hash_string_superfast_new(NULL);
   eina_hash_add(icon_map, mime, val);
   return val;
}

EAPI void
e_fm_mime_icon_cache_flush(void)
{
   Eina_List *freelist = NULL;

   eina_hash_foreach(icon_map, _e_fm_mime_icon_foreach, &freelist);
   while (freelist)
     {
	eina_stringshare_del(freelist->data);
	freelist = eina_list_remove_list(freelist, freelist);
     }
   eina_hash_free(icon_map);
   icon_map = NULL;
}

/* create (allocate), set properties, and return a new mime handler */
EAPI E_Fm2_Mime_Handler *
e_fm2_mime_handler_new(const char *label, const char *icon_group,
                      void (*action_func) (Evas_Object *obj, const char *path, void *data),
		      void *action_data,
		      int (test_func) (Evas_Object *obj, const char *path, void *data),
		      void *test_data)
{
   E_Fm2_Mime_Handler *handler;

   if ((!label) || (!action_func)) return NULL;

   handler = E_NEW(E_Fm2_Mime_Handler, 1);
   if (!handler) return NULL;

   handler->label = eina_stringshare_add(label);
   handler->icon_group = icon_group ? eina_stringshare_add(icon_group) : NULL;
   handler->action_func = action_func;
   handler->action_data = action_data;
   handler->test_func = test_func;
   handler->test_data = test_data;

   return handler;
}

EAPI void
e_fm2_mime_handler_free(E_Fm2_Mime_Handler *handler)
{
   if (!handler) return;

   eina_stringshare_del(handler->label);
   if (handler->icon_group) eina_stringshare_del(handler->icon_group);
   E_FREE(handler);
}

/* associate a certain mime type with a handler */
EAPI Eina_Bool
e_fm2_mime_handler_mime_add(E_Fm2_Mime_Handler *handler, const char *mime)
{
   Eina_List *handlers = NULL;

   if ((!handler) || (!mime)) return 0;

   /* if there's an entry for this mime already, then append to its list */
   if ((handlers = eina_hash_find(_mime_handlers, mime)))
     {
	handlers = eina_list_append(handlers, handler);
	eina_hash_modify(_mime_handlers, mime, handlers);
     }
   else
     {
	/* no previous entry for this mime, lets add one */
	handlers = eina_list_append(handlers, handler);
	if (!_mime_handlers) _mime_handlers = eina_hash_string_superfast_new(NULL);
	eina_hash_add(_mime_handlers, mime, handlers);
     }

   return 1;
}

/* associate a certain glob with a handler */
EAPI Eina_Bool
e_fm2_mime_handler_glob_add(E_Fm2_Mime_Handler *handler, const char *glob)
{
   Eina_List *handlers = NULL;

   if ((!handler) || (!glob)) return 0;

   /* if there's an entry for this glob already, then append to its list */
   if ((handlers = eina_hash_find(_glob_handlers, glob)))
     {
	handlers = eina_list_append(handlers, handler);
	eina_hash_modify(_glob_handlers, glob, handlers);
     }
   else
     {
	/* no previous entry for this glob, lets add one */
	handlers = eina_list_append(handlers, handler);
	if (!_glob_handlers) _glob_handlers = eina_hash_string_superfast_new(NULL);
	eina_hash_add(_glob_handlers, glob, handlers);
     }

   return 1;
}

/* delete a certain handler for a certian mime */
EAPI void
e_fm2_mime_handler_mime_del(E_Fm2_Mime_Handler *handler, const char *mime)
{
   Eina_List *handlers = NULL;

   if ((!handler) || (!mime)) return;

   /* if there's an entry for this mime already, then remove from list */
   if ((handlers = eina_hash_find(_mime_handlers, mime)))
     {
	handlers = eina_list_remove(handlers, handler);
	if (handlers)
	  eina_hash_modify(_mime_handlers, mime, handlers);
	else
	  {
	    eina_hash_del(_mime_handlers, mime, handlers);
	    if (!eina_hash_population(_mime_handlers))
	      {
		eina_hash_free(_mime_handlers);
		_mime_handlers = NULL;
	      }
	  }
     }
}

/* delete a certain handler for a certain glob */
EAPI void
e_fm2_mime_handler_glob_del(E_Fm2_Mime_Handler *handler, const char *glob)
{
   Eina_List *handlers = NULL;

   if ((!handler) || (!glob)) return;

   /* if there's an entry for this glob already, then remove from list */
   if ((handlers = eina_hash_find(_glob_handlers, glob)))
     {
	handlers = eina_list_remove(handlers, handler);
	if (handlers)
	  eina_hash_modify(_glob_handlers, glob, handlers);
	else
	  {
	    eina_hash_del(_glob_handlers, glob, handlers);
	    if (!eina_hash_population(_glob_handlers))
	      {
		eina_hash_free(_glob_handlers);
		_glob_handlers = NULL;
	      }
	  }
     }
}

/* get the list of mime handlers for a mime.
 NOTE: the list should be free()'ed */
EAPI Eina_List *
e_fm2_mime_handler_mime_handlers_get(const char *mime)
{
   if ((!mime) || (!_mime_handlers)) return NULL;
   return eina_hash_find(_mime_handlers, mime);
}

/* get the list of glob handlers for a glob.
 NOTE: the list should be free()'ed */
EAPI Eina_List *
e_fm2_mime_handler_glob_handlers_get(const char *glob)
{
   E_Fm2_Mime_Handler_Tuple *tuple = NULL;
   Eina_List *handlers = NULL;

   if ((!glob) || (!_glob_handlers)) return NULL;

   tuple = E_NEW(E_Fm2_Mime_Handler_Tuple, 1);
   tuple->list = NULL;
   tuple->str = glob;
   eina_hash_foreach(_glob_handlers, _e_fm2_mime_handler_glob_match_foreach, tuple);
   handlers = tuple->list;
   E_FREE(tuple);
   return handlers;
}

/* call a certain handler */
EAPI Eina_Bool
e_fm2_mime_handler_call(E_Fm2_Mime_Handler *handler, Evas_Object *obj, const char *path)
{
   if ((!handler) || (!obj) || (!path) || (!handler->action_func))
     return 0;

   if (handler->test_func)
     {
	if (handler->test_func(obj, path, handler->test_data))
	  {
	     handler->action_func(obj, path, handler->action_data);
	     return 1;
	  }
	else
	  return 0;
     }

   handler->action_func(obj, path, handler->action_data);
   return 1;
}

/* call all handlers related to a certain mime */
EAPI void
e_fm2_mime_handler_mime_handlers_call_all(Evas_Object *obj, const char *path, const char *mime)
{
   Eina_List *handlers = NULL;
   Eina_List *l = NULL;

   if ((!obj) || (!path) || (!mime)) return;

   handlers = e_fm2_mime_handler_mime_handlers_get(mime);
   if (!handlers) return;

   for (l = handlers; l; l = l->next)
     {
	E_Fm2_Mime_Handler *handler = NULL;

	handler = l->data;
	if (!handler) continue;
	
	e_fm2_mime_handler_call(handler, obj, path);
     }
}

/* call all handlers related to a certain glob */
EAPI void
e_fm2_mime_handler_glob_handlers_call_all(Evas_Object *obj, const char *path, const char *glob)
{
   Eina_List *handlers = NULL;
   Eina_List *l = NULL;

   if ((!obj) || (!path) || (!glob)) return;

   handlers = e_fm2_mime_handler_glob_handlers_get(glob);
   if (!handlers) return;

   for (l = handlers; l; l = l->next)
     {
	E_Fm2_Mime_Handler *handler = NULL;

	handler = l->data;
	if (!handler) continue;
	
	e_fm2_mime_handler_call(handler, obj, path);
     }
}

/* run a handlers test function */
EAPI Eina_Bool
e_fm2_mime_handler_test(E_Fm2_Mime_Handler *handler, Evas_Object *obj, const char *path)
{
   if ((!handler) || (!obj) || (!path)) return 0;
   if (!handler->test_func) return 1;

   return handler->test_func(obj, path, handler->test_data);
}

/* local subsystem functions */
/* used to loop a glob hash and determine if the glob handler matches the filename */
static Eina_Bool
_e_fm2_mime_handler_glob_match_foreach(const Eina_Hash *hash __UNUSED__, const void *key, void *data, void *fdata)
{
   E_Fm2_Mime_Handler_Tuple *tuple;
   Eina_List *handlers = NULL;
   Eina_List *l = NULL;

   tuple = fdata;
   if (e_util_glob_match(tuple->str, key))
     {
	handlers = data;
	for (l = handlers; l; l = l->next)
	  {
	     if (l->data)
	       tuple->list = eina_list_append(tuple->list, l->data);
	  }
     }

   return 1;
}

static Eina_Bool
_e_fm_mime_icon_foreach(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata)
{
   Eina_List **freelist;

   freelist = fdata;
   *freelist = eina_list_append(*freelist, data);
   return 1;
}
