#include "e.h"

static Eina_Bool  _e_fm2_custom_file_hash_foreach_list(const Eina_Hash *hash, const void *key, void *data, void *fdata);
static Eina_List *_e_fm2_custom_hash_key_base_list(Eina_Hash *hash, const char *str);
//static Eina_Bool _e_fm2_custom_file_hash_foreach_sub_list(Eina_Hash *hash, const char *key, void *data, void *fdata);
//static Eina_List *_e_fm2_custom_hash_key_sub_list(Eina_Hash *hash, const char *str);
static Eina_Bool  _e_fm2_custom_file_hash_foreach(const Eina_Hash *hash, const void *key, void *data, void *fdata);
static Eina_Bool  _e_fm2_custom_file_hash_foreach_save(const Eina_Hash *hash, const void *key, void *data, void *fdata);
static void       _e_fm2_custom_file_info_load(void);
static void       _e_fm2_custom_file_info_save(void);
static void       _e_fm2_custom_file_info_free(void);
static void       _e_fm2_custom_file_cb_defer_save(void *data);

static E_Powersave_Deferred_Action *_e_fm2_flush_defer = NULL;
static Eet_File *_e_fm2_custom_file = NULL;
static Eet_Data_Descriptor *_e_fm2_custom_file_edd = NULL;
static Eet_Data_Descriptor *_e_fm2_custom_dir_edd = NULL;
static Eina_Hash *_e_fm2_custom_hash = NULL;
static int _e_fm2_custom_writes = 0;
static int _e_fm2_custom_init = 0;

/* FIXME: this uses a flat path key for custom file info. this is fine as
 * long as we only expect a limited number of custom info nodes. if we
 * start to see whole dire trees stored this way things will suck. we need
 * to use a tree struct to store this so deletes and renmes are sane.
 */

/* externally accessible functions */
EINTERN int
e_fm2_custom_file_init(void)
{
   Eet_Data_Descriptor_Class eddc;

   _e_fm2_custom_init++;
   if (_e_fm2_custom_init > 1) return _e_fm2_custom_init;

   if (!eet_eina_stream_data_descriptor_class_set(&eddc, sizeof (eddc), "e_fm2_custom_dir", sizeof (E_Fm2_Custom_Dir)))
     {
        _e_fm2_custom_init--;
        return 0;
     }

   _e_fm2_custom_hash = eina_hash_string_superfast_new(NULL);

   _e_fm2_custom_dir_edd = eet_data_descriptor_stream_new(&eddc);
#define DAT(y, z) EET_DATA_DESCRIPTOR_ADD_BASIC(_e_fm2_custom_dir_edd, E_Fm2_Custom_Dir, #y, y, z)
   DAT(pos.x, EET_T_DOUBLE);
   DAT(pos.y, EET_T_DOUBLE);
   DAT(prop.icon_size, EET_T_SHORT);
   DAT(prop.view_mode, EET_T_CHAR);
   DAT(prop.order_file, EET_T_UCHAR);
   DAT(prop.show_hidden_files, EET_T_UCHAR);
   DAT(prop.in_use, EET_T_UCHAR);

   DAT(prop.sort.no_case, EET_T_UCHAR);
   DAT(prop.sort.size, EET_T_UCHAR);
   DAT(prop.sort.extension, EET_T_UCHAR);
   DAT(prop.sort.mtime, EET_T_UCHAR);
   DAT(prop.sort.dirs.first, EET_T_UCHAR);
   DAT(prop.sort.dirs.last, EET_T_UCHAR);
#undef DAT
   eddc.size = sizeof (E_Fm2_Custom_File);
   eddc.name = "e_fm_custom_file";

   _e_fm2_custom_file_edd = eet_data_descriptor_stream_new(&eddc);
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

#undef DAT

   EET_DATA_DESCRIPTOR_ADD_SUB(_e_fm2_custom_file_edd, E_Fm2_Custom_File, "dir",
                               dir, _e_fm2_custom_dir_edd);

   return 1;
}

EINTERN void
e_fm2_custom_file_shutdown(void)
{
   _e_fm2_custom_init--;
   if (_e_fm2_custom_init != 0) return;

   _e_fm2_custom_file_info_save();
   _e_fm2_custom_file_info_free();
   if (_e_fm2_flush_defer) e_powersave_deferred_action_del(_e_fm2_flush_defer);
   _e_fm2_flush_defer = NULL;
   eet_data_descriptor_free(_e_fm2_custom_file_edd);
   _e_fm2_custom_file_edd = NULL;
   eet_data_descriptor_free(_e_fm2_custom_dir_edd);
   _e_fm2_custom_dir_edd = NULL;
}

EAPI E_Fm2_Custom_File *
e_fm2_custom_file_get(const char *path)
{
   E_Fm2_Custom_File *cf;

   _e_fm2_custom_file_info_load();
   if (!_e_fm2_custom_file) return NULL;
   if (_e_fm2_flush_defer) e_fm2_custom_file_flush();
   cf = eina_hash_find(_e_fm2_custom_hash, path);
   return cf;
}

static void
_e_fm2_custom_dir_del(E_Fm2_Custom_Dir *dir)
{
   free(dir);
}

static void
_e_fm2_custom_file_del(E_Fm2_Custom_File *cf)
{
   if (!cf) return;

   eina_stringshare_del(cf->icon.icon);
   eina_stringshare_del(cf->label);
   _e_fm2_custom_dir_del(cf->dir);
   free(cf);
}

static E_Fm2_Custom_Dir *
_e_fm2_custom_dir_dup(const E_Fm2_Custom_Dir *dir)
{
   E_Fm2_Custom_Dir *copy;

   if (!dir) return NULL;

   copy = calloc(1, sizeof(*copy));
   if (!copy) return NULL;

   memcpy(copy, dir, sizeof(*copy));
   return copy;
}

EAPI E_Fm2_Custom_File *
e_fm2_custom_file_dup(const E_Fm2_Custom_File *cf)
{
   E_Fm2_Custom_File *copy;

   if (!cf) return NULL;

   copy = calloc(1, sizeof(*copy));
   if (!copy) return NULL;

   memcpy(copy, cf, sizeof(*copy));
   copy->icon.icon = eina_stringshare_add(cf->icon.icon);
   copy->label = eina_stringshare_add(cf->label);
   copy->dir = _e_fm2_custom_dir_dup(cf->dir);
   return copy;
}

EAPI void
e_fm2_custom_file_set(const char *path, const E_Fm2_Custom_File *cf)
{
   E_Fm2_Custom_File *cf1;
   _e_fm2_custom_file_info_load();
   if (!_e_fm2_custom_file) return;
   if (_e_fm2_flush_defer) e_fm2_custom_file_flush();

   cf1 = eina_hash_find(_e_fm2_custom_hash, path);
   if ((cf1 != cf) || ((cf1) && (cf) && (cf1->dir != cf->dir)))
     {
        E_Fm2_Custom_File *cf2 = e_fm2_custom_file_dup(cf);
        if (cf2)
          {
             if (cf1)
               {
                  eina_hash_modify(_e_fm2_custom_hash, path, cf2);
                  _e_fm2_custom_file_del(cf1);
               }
             else
               eina_hash_add(_e_fm2_custom_hash, path, cf2);
          }
     }
   _e_fm2_custom_writes = 1;
}

EAPI void
e_fm2_custom_file_del(const char *path)
{
   Eina_List *list, *l;
   E_Fm2_Custom_File *cf2;
   const void *key;

   _e_fm2_custom_file_info_load();
   if (!_e_fm2_custom_file) return;
   if (_e_fm2_flush_defer) e_fm2_custom_file_flush();

   list = _e_fm2_custom_hash_key_base_list(_e_fm2_custom_hash, path);
   if (list)
     {
        EINA_LIST_FOREACH(list, l, key)
          {
             cf2 = eina_hash_find(_e_fm2_custom_hash, key);
             if (cf2)
               {
                  eina_hash_del(_e_fm2_custom_hash, key, cf2);
                  _e_fm2_custom_file_del(cf2);
               }
          }
        eina_list_free(list);
     }
   _e_fm2_custom_writes = 1;
}

EAPI void
e_fm2_custom_file_rename(const char *path, const char *new_path)
{
   E_Fm2_Custom_File *cf, *cf2;
   Eina_List *list, *l;
   const void *key;

   _e_fm2_custom_file_info_load();
   if (!_e_fm2_custom_file) return;
   if (_e_fm2_flush_defer) e_fm2_custom_file_flush();
   cf2 = eina_hash_find(_e_fm2_custom_hash, path);
   if (cf2)
     {
        eina_hash_del(_e_fm2_custom_hash, path, cf2);
        cf = eina_hash_find(_e_fm2_custom_hash, new_path);
        if (cf)
          _e_fm2_custom_file_del(cf);
        eina_hash_add(_e_fm2_custom_hash, new_path, cf2);
     }
   list = _e_fm2_custom_hash_key_base_list(_e_fm2_custom_hash, path);
   if (list)
     {
        EINA_LIST_FOREACH(list, l, key)
          {
             cf2 = eina_hash_find(_e_fm2_custom_hash, key);
             if (cf2)
               {
                  char buf[PATH_MAX];

                  strcpy(buf, new_path);
                  strcat(buf, (char *)key + strlen(path));
                  eina_hash_del(_e_fm2_custom_hash, key, cf2);
                  cf = eina_hash_find(_e_fm2_custom_hash, buf);
                  if (cf)
                    _e_fm2_custom_file_del(cf);
                  eina_hash_add(_e_fm2_custom_hash, buf, cf2);
               }
          }
        eina_list_free(list);
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
   Eina_List  *l;
   const char *base;
   int         base_len;
};

static Eina_Bool
_e_fm2_custom_file_hash_foreach_list(const Eina_Hash *hash __UNUSED__, const void *key, void *data __UNUSED__, void *fdata)
{
   struct _E_Custom_List *cl;

   cl = fdata;
   if (!strncmp(cl->base, key, cl->base_len))
     cl->l = eina_list_append(cl->l, key);
   return 1;
}

static Eina_List *
_e_fm2_custom_hash_key_base_list(Eina_Hash *hash, const char *str)
{
   struct _E_Custom_List cl;

   cl.l = NULL;
   cl.base = str;
   cl.base_len = strlen(cl.base);
   eina_hash_foreach(hash, _e_fm2_custom_file_hash_foreach_list, &cl);
   return cl.l;
}

/*
   static Eina_Bool
   _e_fm2_custom_file_hash_foreach_sub_list(const Eina_Hash *hash, const void *key, void *data, void *fdata)
   {
   struct _E_Custom_List *cl;

   cl = fdata;
   if (!strncmp(cl->base, key, strlen(key)))
     cl->l = eina_list_append(cl->l, key);
   return 1;
   }
 */

/*
   static Eina_List *
   _e_fm2_custom_hash_key_sub_list(const Eina_Hash *hash, const void *str)
   {
   struct _E_Custom_List cl;

   cl.l = NULL;
   cl.base = str;
   eina_hash_foreach(hash,
                     _e_fm2_custom_file_hash_foreach_sub_list, &cl);
   return cl.l;
   }
 */

static Eina_Bool
_e_fm2_custom_file_hash_foreach(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *data, void *fdata __UNUSED__)
{
   _e_fm2_custom_file_del(data);
   return 1;
}

static Eina_Bool
_e_fm2_custom_file_hash_foreach_save(const Eina_Hash *hash __UNUSED__, const void *key, void *data, void *fdata)
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
   e_user_dir_concat_static(buf, "fileman/custom.cfg");
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
                    eina_hash_add(_e_fm2_custom_hash, list[i], cf);
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
   size_t len;
   int ret;

   if (!_e_fm2_custom_file) return;
   if (!_e_fm2_custom_writes) return;

   len = e_user_dir_concat_static(buf, "fileman/custom.cfg.tmp");
   if (len >= sizeof(buf)) return;
   ef = eet_open(buf, EET_FILE_MODE_WRITE);
   if (!ef) return;
   eina_hash_foreach(_e_fm2_custom_hash,
                     _e_fm2_custom_file_hash_foreach_save, ef);
   eet_close(ef);

   memcpy(buf2, buf, len - (sizeof(".tmp") - 1));
   buf2[len - (sizeof(".tmp") - 1)] = '\0';
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
        eina_hash_foreach(_e_fm2_custom_hash,
                          _e_fm2_custom_file_hash_foreach, NULL);
        eina_hash_free(_e_fm2_custom_hash);
        _e_fm2_custom_hash = eina_hash_string_superfast_new(NULL);
     }
}

static void
_e_fm2_custom_file_cb_defer_save(void *data __UNUSED__)
{
   _e_fm2_custom_file_info_save();
   _e_fm2_custom_file_info_free();
   _e_fm2_flush_defer = NULL;
}

