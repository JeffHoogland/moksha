/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define NEWD(str, typ) \
   eet_data_descriptor_new(str, sizeof(typ), \
			      (void *(*) (void *))evas_list_next, \
			      (void *(*) (void *, void *))evas_list_append, \
			      (void *(*) (void *))evas_list_data, \
			      (void *(*) (void *))evas_list_free, \
			      (void  (*) (void *, int (*) (void *, const char *, void *, void *), void *))evas_hash_foreach, \
			      (void *(*) (void *, const char *, void *))evas_hash_add, \
			      (void  (*) (void *))evas_hash_free)

#define FREED(eed) \
   if (eed) \
       { \
	  eet_data_descriptor_free((eed)); \
	  (eed) = NULL; \
       }
#define NEWI(str, it, type) \
   EET_DATA_DESCRIPTOR_ADD_BASIC(_e_app_cache_edd, E_App_Cache, str, it, type)
#define NEWL(str, it, type) \
   EET_DATA_DESCRIPTOR_ADD_LIST(_e_app_cache_edd, E_App_Cache, str, it, type)

static void _e_eapp_cache_fill(E_App_Cache *ac, E_App *a);

static Eet_Data_Descriptor *_e_app_cache_edd = NULL;

int
e_app_cache_init(void)
{
   _e_app_cache_edd = NEWD("E_App_Cache", E_App_Cache);
   NEWI("nm", name, EET_T_STRING);
   NEWI("gn", generic, EET_T_STRING);
   NEWI("cm", comment, EET_T_STRING);
   NEWI("ex", exe, EET_T_STRING);
   NEWI("fl", file, EET_T_STRING);
   NEWI("fm", file_mod_time, EET_T_ULONG_LONG);
   NEWI("wn", win_name, EET_T_STRING);
   NEWI("wc", win_class, EET_T_STRING);
   NEWI("wt", win_title, EET_T_STRING);
   NEWI("wr", win_role, EET_T_STRING);
   NEWI("ic", icon_class, EET_T_STRING);
   NEWL("ap", subapps, _e_app_cache_edd);
   NEWI("sn", startup_notify, EET_T_UCHAR);
   NEWI("wx", wait_exit, EET_T_UCHAR);
   return 1;
}

int
e_app_cache_shutdown(void)
{
   FREED(_e_app_cache_edd);
   return 1;
}

E_App_Cache *
e_app_cache_load(char *path)
{
   Eet_File *ef;
   char buf[PATH_MAX];
   E_App_Cache *ac;
   
   if (!path) return NULL;
   snprintf(buf, sizeof(buf), "%s/.eap.cache.cfg", path);
   ef = eet_open(buf, EET_FILE_MODE_READ);
   if (!ef) return NULL;
   ac = eet_data_read(ef, _e_app_cache_edd, "cache");
   eet_close(ef);
   return ac;
}

E_App_Cache *
e_app_cache_generate(E_App *a)
{
   E_App_Cache *ac;
   Evas_List *l;
   
   if (!a) return NULL;
   ac = calloc(1, sizeof(E_App_Cache));
   if (!ac) return NULL;
   _e_eapp_cache_fill(ac, a);
   for (l = a->subapps; l; l = l->next)
     {
	E_App *a2;
	E_App_Cache *ac2;
	
	a2 = l->data;
	ac2 = calloc(1, sizeof(E_App_Cache));
	if (ac2)
	  {
	     _e_eapp_cache_fill(ac2, a2);
	     ac->subapps = evas_list_append(ac->subapps, ac2);
	     ac->subapps_hash = evas_hash_add(ac->subapps_hash, ac2->file, ac2);
	  }
     }
   return ac;
}

E_App_Cache *
e_app_cache_path_generate(char *path)
{
   E_App_Cache *ac;
   E_App *a;
   char buf[PATH_MAX];
   Ecore_List *files;

   if (!path) return NULL;
   ac = calloc(1, sizeof(E_App_Cache));
   if (!ac) return NULL;
   a = e_app_raw_new();
   a->path = strdup(path);
   _e_eapp_cache_fill(ac, a);
   if (ecore_file_is_dir(a->path))
     {
	snprintf(buf, sizeof(buf), "%s/.directory.eap", path);
	if (ecore_file_exists(buf))
	  e_app_fields_fill(a, buf);
	else
	  a->name = strdup(ecore_file_get_file(a->path));
	
	files = e_app_dir_file_list_get(a);
	if (files)
	  {
	     char *s = NULL;
	     
	     while ((s = ecore_list_next(files)))
	       {
		  E_App *a2;
		  E_App_Cache *ac2;
		  
		  ac2 = calloc(1, sizeof(E_App_Cache));
		  if (ac2)
		    {
		       a2 = e_app_raw_new();
		       if (a2)
			 {
			    snprintf(buf, sizeof(buf), "%s/%s", a->path, s);
			    a2->path = strdup(buf);
			    if (ecore_file_is_dir(a2->path))
			      {
				 snprintf(buf, sizeof(buf), "%s/.directory.eap", a2->path);
				 e_app_fields_fill(a2, buf);
			      }
			    else
			      e_app_fields_fill(a2, a2->path);
			    _e_eapp_cache_fill(ac2, a2);
			    e_app_fields_empty(a2);
			    free(a2);
			 }
		       ac->subapps = evas_list_append(ac->subapps, ac2);
		       ac->subapps_hash = evas_hash_add(ac->subapps_hash, ac2->file, ac2);
		    }
	       }
	     ecore_list_destroy(files);
	  }
     }
   e_app_fields_empty(a);
   free(a);
   
   return ac;
}

void
e_app_cache_free(E_App_Cache *ac)
{
   if (!ac) return;
   E_FREE(ac->name);
   E_FREE(ac->generic);
   E_FREE(ac->comment);
   E_FREE(ac->exe);
   E_FREE(ac->file);
   E_FREE(ac->win_name);
   E_FREE(ac->win_class);
   E_FREE(ac->win_title);
   E_FREE(ac->win_role);
   E_FREE(ac->icon_class);
   while (ac->subapps)
     {
	E_App_Cache *ac2;
	
	ac2 = ac->subapps->data;
	ac->subapps = evas_list_remove_list(ac->subapps, ac->subapps);
	e_app_cache_free(ac2);
     }
   evas_hash_free(ac->subapps_hash);
   free(ac);
}



int
e_app_cache_save(E_App_Cache *ac, char *path)
{
   Eet_File *ef;
   char buf[4096];
   int ret;
   
   if ((!ac) || (!path)) return 0;
   snprintf(buf, sizeof(buf), "%s/.eap.cache.cfg", path);
   ef = eet_open(buf, EET_FILE_MODE_WRITE);
   if (!ef) return 0;
   ret = eet_data_write(ef, _e_app_cache_edd, "cache", ac, 1);
   eet_close(ef);
   return ret;
}



static void
_e_eapp_cache_fill(E_App_Cache *ac, E_App *a)
{
#define IF_DUP(x) if (a->x) ac->x = strdup(a->x)
   IF_DUP(name);
   IF_DUP(generic);
   IF_DUP(comment);
   IF_DUP(exe);
   ac->file = strdup(ecore_file_get_file(a->path));
   ac->file_mod_time = ecore_file_mod_time(a->path);
   IF_DUP(win_name);
   IF_DUP(win_class);
   IF_DUP(win_title);
   IF_DUP(win_role);
   IF_DUP(icon_class);
   ac->startup_notify = a->startup_notify;
   ac->wait_exit = a->wait_exit;
}

