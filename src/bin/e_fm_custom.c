/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static Evas_Bool _e_fm2_custom_file_hash_foreach(Evas_Hash *hash, const char *key, void *data, void *fdata);
static void _e_fm2_custom_file_info_clean(void);
static void _e_fm2_custom_file_info_load(void);
static void _e_fm2_custom_file_info_save(void);
static void _e_fm2_custom_file_info_free(void);
static int _e_fm2_custom_file_cb_timer_save(void *data);

static Ecore_Timer *_e_fm2_flush_timer = NULL;
static Eet_File *_e_fm2_custom_file = NULL;
static Eet_Data_Descriptor *_e_fm2_custom_file_edd = NULL;
static Evas_Hash *_e_fm2_custom_hash = NULL;

/* externally accessible functions */
EAPI int
e_fm2_custom_file_init(void)
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
   if (_e_fm2_flush_timer) ecore_timer_del(_e_fm2_flush_timer);
   _e_fm2_flush_timer = NULL;
   eet_data_descriptor_free(_e_fm2_custom_file_edd);
   _e_fm2_custom_file_edd = NULL;
}

EAPI E_Fm2_Custom_File *
e_fm2_custom_file_get(const char *path)
{
   E_Fm2_Custom_File *cf;
   
   _e_fm2_custom_file_info_load();
   if (!_e_fm2_custom_file) return NULL;
   /* get any custom info for the path in our metadata - if non exists,
    * return NULL. This may mean loading upa chunk of metadata off disk
    * on demand and caching it */
   if (_e_fm2_flush_timer)
     {
	ecore_timer_del(_e_fm2_flush_timer);
	_e_fm2_flush_timer = ecore_timer_add(1.0, _e_fm2_custom_file_cb_timer_save, NULL);
     }
//   printf("FIND CUSTOM %s\n", path);
   cf = evas_hash_find(_e_fm2_custom_hash, path);
   if (cf) return cf;
   cf = eet_data_read(_e_fm2_custom_file, _e_fm2_custom_file_edd, path);
   if (cf)
     _e_fm2_custom_hash = evas_hash_add(_e_fm2_custom_hash, path, cf);
   if (cf)
     {
	printf("CUSTOM FILE for %s:\n"
	       "  type=%i,icon=%s,valid=%i\n",
	       path, cf->icon.type, cf->icon.icon, cf->icon.valid);
     }
   return cf;
}

EAPI void
e_fm2_custom_file_set(const char *path, E_Fm2_Custom_File *cf)
{
   _e_fm2_custom_file_info_load();
   if (!_e_fm2_custom_file) return;
   if (_e_fm2_flush_timer)
     {
	ecore_timer_del(_e_fm2_flush_timer);
	_e_fm2_flush_timer = ecore_timer_add(1.0, _e_fm2_custom_file_cb_timer_save, NULL);
     }
   /* set custom metadata for a file path - save it to the metadata (or queue it) */
   if (evas_hash_find(_e_fm2_custom_hash, path) != cf)
     {
	E_Fm2_Custom_File *cf2;
	
	cf2 = calloc(1, sizeof(E_Fm2_Custom_File));
	if (cf2)
	  {
	     memcpy(cf2, cf, sizeof(E_Fm2_Custom_File));
	     if (cf->icon.icon)
	       cf2->icon.icon = evas_stringshare_add(cf->icon.icon);
	     if (cf->label)
	       cf2->label = evas_stringshare_add(cf->label);
	     _e_fm2_custom_hash = evas_hash_add(_e_fm2_custom_hash, path, cf2);
	     cf = cf2;
	  }
     }
/*   
   printf("SET CUSTOM %s %p %s %i %i\n", 
	  path, cf, cf->icon.icon, cf->icon.type, cf->icon.valid);
   printf("_e_fm2_custom_file = %p, _e_fm2_custom_file_edd=%p\n",
	  _e_fm2_custom_file, _e_fm2_custom_file_edd);
 */
   eet_data_write(_e_fm2_custom_file, _e_fm2_custom_file_edd, path, cf, 1);
}

EAPI void
e_fm2_custom_file_del(const char *path)
{
   E_Fm2_Custom_File *cf;
   
   _e_fm2_custom_file_info_load();
   if (!_e_fm2_custom_file) return;
   /* delete a custom metadata entry for a path - save changes (or queue it) */
   if (_e_fm2_flush_timer)
     {
	ecore_timer_del(_e_fm2_flush_timer);
	_e_fm2_flush_timer = ecore_timer_add(1.0, _e_fm2_custom_file_cb_timer_save, NULL);
     }
/*   printf("DEL CUSTOM %s\n",path);*/
   cf = evas_hash_find(_e_fm2_custom_hash, path);
   if (cf)
     {
	_e_fm2_custom_hash = evas_hash_del(_e_fm2_custom_hash, path, cf);
	if (cf->icon.icon) evas_stringshare_del(cf->icon.icon);
	if (cf->label) evas_stringshare_del(cf->label);
	free(cf);
     }
   eet_delete(_e_fm2_custom_file, path);
}

EAPI void
e_fm2_custom_file_rename(const char *path, const char *new_path)
{
   E_Fm2_Custom_File *cf;
   void *dat;
   int size;
   
   _e_fm2_custom_file_info_load();
   if (!_e_fm2_custom_file) return;
   /* rename file path a to file paht b in the metadata - if the path exists */
   if (_e_fm2_flush_timer)
     {
	ecore_timer_del(_e_fm2_flush_timer);
	_e_fm2_flush_timer = ecore_timer_add(1.0, _e_fm2_custom_file_cb_timer_save, NULL);
     }
   cf = evas_hash_find(_e_fm2_custom_hash, path);
   if (cf)
     {
	_e_fm2_custom_hash = evas_hash_del(_e_fm2_custom_hash, path, cf);
	_e_fm2_custom_hash = evas_hash_add(_e_fm2_custom_hash, new_path, cf);
     }
   dat = eet_read(_e_fm2_custom_file, path, &size);
   if (dat)
     {
	eet_write(_e_fm2_custom_file, new_path, dat, size, 1);
	free(dat);
     }
}

EAPI void
e_fm2_custom_file_flush(void)
{
   if (!_e_fm2_custom_file) return;
   /* free any loaded custom file data, sync changes to disk etc. */
   if (_e_fm2_flush_timer) ecore_timer_del(_e_fm2_flush_timer);
   _e_fm2_flush_timer = ecore_timer_add(1.0, _e_fm2_custom_file_cb_timer_save, NULL);
   _e_fm2_custom_file_info_free();
   evas_hash_foreach(_e_fm2_custom_hash, _e_fm2_custom_file_hash_foreach, NULL);
   evas_hash_free(_e_fm2_custom_hash);
   _e_fm2_custom_hash = NULL;
}

/**/

static Evas_Bool
_e_fm2_custom_file_hash_foreach(Evas_Hash *hash, const char *key, void *data, void *fdata)
{
   E_Fm2_Custom_File *cf;
   
   cf = data;
   if (cf->icon.icon) evas_stringshare_del(cf->icon.icon);
   if (cf->label) evas_stringshare_del(cf->label);
   free(cf);
   return 1;
}

static void
_e_fm2_custom_file_info_clean(void)
{
   char **list;
   int i, num;

   /* FIXME: this could be nasty on interactivity */
   _e_fm2_custom_file_info_load();
   if (!_e_fm2_custom_file) return;
	
   list = eet_list(_e_fm2_custom_file, "*", &num);
   for (i = 0; i < num; i++)
     {
	if (!ecore_file_exists(list[i]))
	  eet_delete(_e_fm2_custom_file, list[i]);
     }
   if (list) free(list);
   
   e_fm2_custom_file_flush();
}

static void
_e_fm2_custom_file_info_load(void)
{
   char buf[PATH_MAX];
   
   if (_e_fm2_custom_file) return;
   snprintf(buf, sizeof(buf), "%s/.e/e/fileman/custom.cfg", e_user_homedir_get());
   _e_fm2_custom_file = eet_open(buf, EET_FILE_MODE_READ_WRITE);
   if (!_e_fm2_custom_file)
     _e_fm2_custom_file = eet_open(buf, EET_FILE_MODE_WRITE);
   if (_e_fm2_custom_file)
     {
	char **list;
	int i, num;
	
	list = eet_list(_e_fm2_custom_file, "*", &num);
	for (i = 0; i < num; i++)
	  {
	     void *dat;
	     int size;
	     
	     dat = eet_read(_e_fm2_custom_file, list[i], &size);
	     if (dat)
	       {
		  eet_write(_e_fm2_custom_file, list[i], dat, size, 1);
		  free(dat);
	       }
	  }
	if (list) free(list);
     }
}

static void
_e_fm2_custom_file_info_save(void)
{
   if (!_e_fm2_custom_file) return;
}

static void
_e_fm2_custom_file_info_free(void)
{
   if (!_e_fm2_custom_file) return;
   eet_close(_e_fm2_custom_file);
   _e_fm2_custom_file = NULL;
}

static int
_e_fm2_custom_file_cb_timer_save(void *data)
{
   _e_fm2_custom_file_info_save();
   _e_fm2_custom_file_info_free();
   _e_fm2_flush_timer = NULL;
   return 0;
}
