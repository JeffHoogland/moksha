#ifdef E_TYPEDEFS

typedef struct _E_Fm2_Custom_File E_Fm2_Custom_File;
typedef struct _E_Fm2_Custom_Dir E_Fm2_Custom_Dir;

#else
#ifndef E_FM_CUSTOM_H
#define E_FM_CUSTOM_H

struct _E_Fm2_Custom_Dir
{
   struct {
      double x, y;
   } pos;
   struct {
      signed short      icon_size; /* -1 = unset */
      signed char       view_mode; /* -1 = unset */
      Eina_Bool     order_file;
      Eina_Bool     show_hidden_files;
      Eina_Bool     in_use;
      struct
      {
         Eina_Bool no_case;
         Eina_Bool size;
         Eina_Bool extension;
         Eina_Bool mtime;
         struct
         {
            Eina_Bool first;
            Eina_Bool last;
         } dirs;
      } sort;
   } prop;
};

struct _E_Fm2_Custom_File
{
   struct {
      int            x, y, w, h;
      int            res_w, res_h;
      double         scale;
      Eina_Bool  valid;
   } geom;
   struct {
      int            type;
      const char    *icon;
      Eina_Bool  valid;
   } icon;
   const char       *label;
   E_Fm2_Custom_Dir *dir;
   /* FIXME: this will have more added */
};

EINTERN int                   e_fm2_custom_file_init(void);
EINTERN void                  e_fm2_custom_file_shutdown(void);
EAPI E_Fm2_Custom_File    *e_fm2_custom_file_get(const char *path);
EAPI void                  e_fm2_custom_file_set(const char *path, const E_Fm2_Custom_File *cf);
EAPI void                  e_fm2_custom_file_del(const char *path);
EAPI void                  e_fm2_custom_file_rename(const char *path, const char *new_path);
EAPI void                  e_fm2_custom_file_flush(void);

EAPI E_Fm2_Custom_File    *e_fm2_custom_file_dup(const E_Fm2_Custom_File *cf);

#endif
#endif
