/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define NEWD(str, typ) \
     { eddc.name = str; eddc.size = sizeof(typ); }

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

EAPI int
e_app_cache_init(void)
{
   Eet_Data_Descriptor_Class eddc;
   
   eddc.version = EET_DATA_DESCRIPTOR_CLASS_VERSION;
   eddc.func.mem_alloc = NULL;
   eddc.func.mem_free = NULL;
   eddc.func.str_alloc = (char *(*)(const char *)) evas_stringshare_add;
   eddc.func.str_free = (void (*)(const char *)) evas_stringshare_del;
   eddc.func.list_next = (void *(*)(void *)) evas_list_next;
   eddc.func.list_append = (void *(*)(void *l, void *d)) evas_list_append;
   eddc.func.list_data = (void *(*)(void *)) evas_list_data;
   eddc.func.list_free = (void *(*)(void *)) evas_list_free;
   eddc.func.hash_foreach = 
      (void  (*) (void *, int (*) (void *, const char *, void *, void *), void *)) 
      evas_hash_foreach;
   eddc.func.hash_add = (void *(*) (void *, const char *, void *)) evas_hash_add;
   eddc.func.hash_free = (void  (*) (void *)) evas_hash_free;
   
   NEWD("E_App_Cache", E_App_Cache);
   _e_app_cache_edd = eet_data_descriptor2_new(&eddc);
   
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
   NEWI("il", is_link, EET_T_UCHAR);
   NEWI("id", is_dir, EET_T_UCHAR);
   return 1;
}

EAPI int
e_app_cache_shutdown(void)
{
   FREED(_e_app_cache_edd);
   return 1;
}

EAPI E_App_Cache *
e_app_cache_load(const char *path)
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
   if (ac)
     {
	Evas_List *l;
	
	for (l = ac->subapps; l; l = l->next)
	  {
	     E_App_Cache *ac2;
	     
	     ac2 = l->data;
	     ac->subapps_hash = evas_hash_add(ac->subapps_hash, ac2->file, ac2);
	  }
     }
   return ac;
}

EAPI E_App_Cache *
e_app_cache_generate(E_App *a)
{
   E_App_Cache *ac;
   Evas_List *l;
   char buf[PATH_MAX];
   
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
	     ac2->is_dir = ecore_file_is_dir(a2->path);
	     snprintf(buf, sizeof(buf), "%s/%s", a->path, ecore_file_get_file(a2->path));
	     if (a2->orig) ac2->is_link = 1;
	     if ((!ac2->is_link) && (!ac2->is_dir))
	       ac2->file_mod_time = ecore_file_mod_time(buf);
	     ac->subapps = evas_list_append(ac->subapps, ac2);
	     ac->subapps_hash = evas_hash_direct_add(ac->subapps_hash, ac2->file, ac2);
	  }
     }
   return ac;
}

EAPI void
e_app_cache_free(E_App_Cache *ac)
{
   if (!ac) return;
   if (ac->name) evas_stringshare_del(ac->name);
   if (ac->generic) evas_stringshare_del(ac->generic);
   if (ac->comment) evas_stringshare_del(ac->comment);
   if (ac->exe) evas_stringshare_del(ac->exe);
   if (ac->file) evas_stringshare_del(ac->file);
   if (ac->win_name) evas_stringshare_del(ac->win_name);
   if (ac->win_class) evas_stringshare_del(ac->win_class);
   if (ac->win_title) evas_stringshare_del(ac->win_title);
   if (ac->win_role) evas_stringshare_del(ac->win_role);
   if (ac->icon_class) evas_stringshare_del(ac->icon_class);
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

EAPI int
e_app_cache_save(E_App_Cache *ac, const char *path)
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
#define IF_DUP(x) if (a->x) ac->x = evas_stringshare_add(a->x)
   IF_DUP(name);
   IF_DUP(generic);
   IF_DUP(comment);
   IF_DUP(exe);
   ac->file = evas_stringshare_add(ecore_file_get_file(a->path));
   IF_DUP(win_name);
   IF_DUP(win_class);
   IF_DUP(win_title);
   IF_DUP(win_role);
   IF_DUP(icon_class);
   ac->startup_notify = a->startup_notify;
   ac->wait_exit = a->wait_exit;
}
