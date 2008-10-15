/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static Evas_Bool _e_fm2_custom_file_hash_foreach_list(const Evas_Hash *hash, const char *key, void *data, void *fdata);
static Evas_List *_e_fm2_custom_hash_key_base_list(Evas_Hash *hash, const char *str);
//static Evas_Bool _e_fm2_custom_file_hash_foreach_sub_list(Evas_Hash *hash, const char *key, void *data, void *fdata);
//static Evas_List *_e_fm2_custom_hash_key_sub_list(Evas_Hash *hash, const char *str);
static Evas_Bool _e_fm2_custom_file_hash_foreach(const Evas_Hash *hash, const char *key, void *data, void *fdata);
static Evas_Bool _e_fm2_custom_file_hash_foreach_save(const Evas_Hash *hash, const char *key, void *data, void *fdata);
static void _e_fm2_custom_file_info_load(void);
static void _e_fm2_custom_file_info_save(void);
static void _e_fm2_custom_file_info_free(void);
static void _e_fm2_custom_file_cb_defer_save(void *data);

static E_Powersave_Deferred_Action*_e_fm2_flush_defer = NULL;
static Eet_File *_e_fm2_custom_file = NULL;
static Eet_Data_Descriptor *_e_fm2_custom_file_edd = NULL;
static Evas_Hash *_e_fm2_custom_hash = NULL;
static int _e_fm2_custom_writes = 0;


/* FIXME: this uses a flat path key for custom file info. this is fine as
 * long as we only expect a limited number of custom info nodes. if we
 * start to see whole dire trees stored this way things will suck. we need
 * to use a tree struct to store this so deletes and renmes are sane.
 */

/* externally accessible functions */
EAPI int
e_fm2_custom_file_init(void)
{
   Eet_Data_Descriptor_Class eddc;
   
   eddc.version = EET_DATA_DESCRIPTOR_CLASS_VERSION;
   eddc.func.mem_alloc = NULL;
   eddc.func.mem_free = NULL;
   eddc.func.str_alloc = (char *(*)(const char *)) eina_stringshare_add;
   eddc.func.str_free = (void (*)(const char *)) eina_stringshare_del;
   eddc.func.list_next = (void *(*)(void *)) evas_list_next;
   eddc.func.list_append = (void *(*)(void *l, void *d)) evas_list_append;
   eddc.func.list_data = (void *(*)(void *)) evas_list_data;
   eddc.func.list_free = (void *(*)(void *)) evas_list_free;
   eddc.func.hash_foreach =
     (void  (*) (void *, int (*) (void *, const char *, void *, void *), void *))
     evas_hash_foreach;
   eddc.func.hash_add = (void *(*) (void *, const char *, void *)) evas_hash_add;
   eddc.func.hash_free = (void  (*) (void *)) evas_hash_free;
   eddc.name = "e_fm_custom_file";
   eddc.size = sizeof(E_Fm2_Custom_File);

   _e_fm2_custom_file_edd = eet_data_descriptor2_new(&eddc);
#define DAT(x, y, z) EET_DATA_DESCRIPTOR_ADD_BASIC(_e_fm2_custom_file_edd, E_Fm2_Custom_File, x, y, z)   
   DAT("g.x", geom.x, EET_T_INT);
   DAT("g.y", geom.y, EET_T_INT);
   DAT("g.w", geom.w, EET_T_INT);
   DAT("g.h", geom.h, EET_T_INT);
   DAT("g.rw", geom.res_w, EET_T_INT);
   DAT("g.rh", geom.res_h, EET_T_INT);
   DAT("g.s", geom.scale, EET_T_DOUBLE);
   DAT("g.v", geom.valid, EET_T_UCHAR);
   
   DAT("i.t", icon.type, EET_T_INT);
   DAT("i.i", icon.icon, EET_T_STRING);
   DAT("i.v", icon.valid, EET_T_UCHAR);
   
   DAT("l", label, EET_T_STRING);
   
   return 1;
}

EAPI void
e_fm2_custom_file_shutdown(void)
{
   _e_fm2_custom_file_info_save();
   _e_fm2_custom_file_info_free();
   if (_e_fm2_flush_defer) e_powersave_deferred_action_del(_e_fm2_flush_defer);
   _e_fm2_flush_defer = NULL;
   eet_data_descriptor_free(_e_fm2_custom_file_edd);
   _e_fm2_custom_file_edd = NULL;
}

EAPI E_Fm2_Custom_File *
e_fm2_custom_file_get(const char *path)
{
   E_Fm2_Custom_File *cf;
   
   _e_fm2_custom_file_info_load();
   if (!_e_fm2_custom_file) return NULL;
   if (_e_fm2_flush_defer) e_fm2_custom_file_flush();
   cf = evas_hash_find(_e_fm2_custom_hash, path);
   return cf;
}

EAPI void
e_fm2_custom_file_set(const char *path, E_Fm2_Custom_File *cf)
{
   _e_fm2_custom_file_info_load();
   if (!_e_fm2_custom_file) return;
   if (_e_fm2_flush_defer) e_fm2_custom_file_flush();
   if (evas_hash_find(_e_fm2_custom_hash, path) != cf)
     {
	E_Fm2_Custom_File *cf2;
	
	cf2 = calloc(1, sizeof(E_Fm2_Custom_File));
	if (cf2)
	  {
	     memcpy(cf2, cf, sizeof(E_Fm2_Custom_File));
	     if (cf->icon.icon)
	       cf2->icon.icon = eina_stringshare_add(cf->icon.icon);
	     if (cf->label)
	       cf2->label = eina_stringshare_add(cf->label);
	     _e_fm2_custom_hash = evas_hash_add(_e_fm2_custom_hash, path, cf2);
	  }
     }
   _e_fm2_custom_writes = 1;
}

EAPI void
e_fm2_custom_file_del(const char *path)
{
   Evas_List *list, *l;
   E_Fm2_Custom_File *cf2;
	
   _e_fm2_custom_file_info_load();
   if (!_e_fm2_custom_file) return;
   if (_e_fm2_flush_defer) e_fm2_custom_file_flush();
   
   list = _e_fm2_custom_hash_key_base_list(_e_fm2_custom_hash, path);
   if (list)
     {
	for (l = list; l; l = l->next)
	  {
	     cf2 = evas_hash_find(_e_fm2_custom_hash, l->data);
	     if (cf2)
	       {
		  _e_fm2_custom_hash = evas_hash_del(_e_fm2_custom_hash,
						     l->data, cf2);
		  if (cf2->icon.icon) eina_stringshare_del(cf2->icon.icon);
		  if (cf2->label) eina_stringshare_del(cf2->label);
		  free(cf2);
	       }
	  }
	evas_list_free(list);
     }
   _e_fm2_custom_writes = 1;
}

EAPI void
e_fm2_custom_file_rename(const char *path, const char *new_path)
{
   E_Fm2_Custom_File *cf, *cf2;
   Evas_List *list, *l;

   _e_fm2_custom_file_info_load();
   if (!_e_fm2_custom_file) return;
   if (_e_fm2_flush_defer) e_fm2_custom_file_flush();
   cf2 = evas_hash_find(_e_fm2_custom_hash, path);
   if (cf2)
     {
	_e_fm2_custom_hash = evas_hash_del(_e_fm2_custom_hash, path, cf2);
	cf = evas_hash_find(_e_fm2_custom_hash, new_path);
	if (cf)
	  {
	     if (cf->icon.icon) eina_stringshare_del(cf->icon.icon);
	     if (cf->label) eina_stringshare_del(cf->label);
	     free(cf);
	  }
	_e_fm2_custom_hash = evas_hash_add(_e_fm2_custom_hash, new_path, cf2);
     }
   list = _e_fm2_custom_hash_key_base_list(_e_fm2_custom_hash, path);
   if (list)
     {
	for (l = list; l; l = l->next)
	  {
	     cf2 = evas_hash_find(_e_fm2_custom_hash, l->data);
	     if (cf2)
	       {
		  char buf[PATH_MAX];
		  
		  strcpy(buf, new_path);
		  strcat(buf, (char *)l->data + strlen(path));
		  _e_fm2_custom_hash = evas_hash_del(_e_fm2_custom_hash,
						     l->data, cf2);
		  cf = evas_hash_find(_e_fm2_custom_hash, buf);
		  if (cf)
		    {
		       if (cf->icon.icon) eina_stringshare_del(cf->icon.icon);
		       if (cf->label) eina_stringshare_del(cf->label);
		       free(cf);
		    }
		  _e_fm2_custom_hash = evas_hash_add(_e_fm2_custom_hash,
						     buf, cf2);
	       }
	  }
	evas_list_free(list);
     }
   _e_fm2_custom_writes = 1;
}

EAPI void
e_fm2_custom_file_flush(void)
{
   if (!_e_fm2_custom_file) return;
   
   if (_e_fm2_flush_defer)
     e_powersave_deferred_action_del(_e_fm2_flush_defer);
   _e_fm2_flush_defer = 
     e_powersave_deferred_action_add(_e_fm2_custom_file_cb_defer_save, NULL);
}

/**/

struct _E_Custom_List
{
   Evas_List  *l;
   const char *base;
   int         base_len;
};

static Evas_Bool
_e_fm2_custom_file_hash_foreach_list(const Evas_Hash *hash, const char *key, void *data, void *fdata)
{
   struct _E_Custom_List *cl;
   
   cl = fdata;
   if (!strncmp(cl->base, key, cl->base_len))
     cl->l = evas_list_append(cl->l, key);
   return 1;
}
    
static Evas_List *
_e_fm2_custom_hash_key_base_list(Evas_Hash *hash, const char *str)
{
   struct _E_Custom_List cl;
   
   cl.l = NULL;
   cl.base = str;
   cl.base_len = strlen(cl.base);
   evas_hash_foreach(hash,
		     _e_fm2_custom_file_hash_foreach_list, &cl);
   return cl.l;
}

/*
static Evas_Bool
_e_fm2_custom_file_hash_foreach_sub_list(const Evas_Hash *hash, const char *key, void *data, void *fdata)
{
   struct _E_Custom_List *cl;
   
   cl = fdata;
   if (!strncmp(cl->base, key, strlen(key)))
     cl->l = evas_list_append(cl->l, key);
   return 1;
}
*/

/*
static Evas_List *
_e_fm2_custom_hash_key_sub_list(const Evas_Hash *hash, const char *str)
{
   struct _E_Custom_List cl;
   
   cl.l = NULL;
   cl.base = str;
   evas_hash_foreach(hash,
		     _e_fm2_custom_file_hash_foreach_sub_list, &cl);
   return cl.l;
}
*/

static Evas_Bool
_e_fm2_custom_file_hash_foreach(const Evas_Hash *hash, const char *key, void *data, void *fdata)
{
   E_Fm2_Custom_File *cf;
   
   cf = data;
   if (cf->icon.icon) eina_stringshare_del(cf->icon.icon);
   if (cf->label) eina_stringshare_del(cf->label);
   free(cf);
   return 1;
}

static Evas_Bool
_e_fm2_custom_file_hash_foreach_save(const Evas_Hash *hash, const char *key, void *data, void *fdata)
{
   Eet_File *ef;
   E_Fm2_Custom_File *cf;
   
   ef = fdata;
   cf = data;
   eet_data_write(ef, _e_fm2_custom_file_edd, key, cf, 1);
   return 1;
}

static void
_e_fm2_custom_file_info_load(void)
{
   char buf[PATH_MAX];
   
   if (_e_fm2_custom_file) return;
   _e_fm2_custom_writes = 0;
   snprintf(buf, sizeof(buf), "%s/.e/e/fileman/custom.cfg",
	    e_user_homedir_get());
   _e_fm2_custom_file = eet_open(buf, EET_FILE_MODE_READ);
   if (!_e_fm2_custom_file)
     _e_fm2_custom_file = eet_open(buf, EET_FILE_MODE_WRITE);
   if (_e_fm2_custom_file)
     {
	E_Fm2_Custom_File *cf;
	char **list;
	int i, num;
	
	list = eet_list(_e_fm2_custom_file, "*", &num);
	if (list)
	  {
	     for (i = 0; i < num; i++)
	       {
		  cf = eet_data_read(_e_fm2_custom_file,
				     _e_fm2_custom_file_edd, list[i]);
		  if (cf)
		    _e_fm2_custom_hash = evas_hash_add(_e_fm2_custom_hash,
						       list[i], cf);
	       }
	     free(list);
	  }
     }
}

static void
_e_fm2_custom_file_info_save(void)
{
   Eet_File *ef;
   char buf[PATH_MAX], buf2[PATH_MAX];
   int ret;

   if (!_e_fm2_custom_file) return;
   if (!_e_fm2_custom_writes) return;
   snprintf(buf, sizeof(buf), "%s/.e/e/fileman/custom.cfg.tmp",
	    e_user_homedir_get());
   ef = eet_open(buf, EET_FILE_MODE_WRITE);
   if (!ef) return;
   evas_hash_foreach(_e_fm2_custom_hash,
		     _e_fm2_custom_file_hash_foreach_save, ef);
   eet_close(ef);
   snprintf(buf2, sizeof(buf2), "%s/.e/e/fileman/custom.cfg",
	    e_user_homedir_get());
   eet_close(_e_fm2_custom_file);
   _e_fm2_custom_file = NULL;
   ret = rename(buf, buf2);
   if (ret < 0)
     {
	/* FIXME: do we want to trap individual errno
	 and provide a short blurp to the user? */
	perror("rename");
     }
}

static void
_e_fm2_custom_file_info_free(void)
{
   _e_fm2_custom_writes = 0;
   if (_e_fm2_custom_file)
     {
	eet_close(_e_fm2_custom_file);
	_e_fm2_custom_file = NULL;
     }
   if (_e_fm2_custom_hash)
     {
	evas_hash_foreach(_e_fm2_custom_hash,
			  _e_fm2_custom_file_hash_foreach, NULL);
	evas_hash_free(_e_fm2_custom_hash);
	_e_fm2_custom_hash = NULL;
     }
}

static void
_e_fm2_custom_file_cb_defer_save(void *data)
{
   _e_fm2_custom_file_info_save();
   _e_fm2_custom_file_info_free();
   _e_fm2_flush_defer = NULL;
}
