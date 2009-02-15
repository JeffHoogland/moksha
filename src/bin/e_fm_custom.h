/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
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
      unsigned char     order_file;
      unsigned char     show_hidden_files;
      unsigned char     in_use;
   } prop;
};

struct _E_Fm2_Custom_File
{
   struct {
      int            x, y, w, h;
      int            res_w, res_h;
      double         scale;
      unsigned char  valid;
   } geom;
   struct {
      int            type;
      const char    *icon;
      unsigned char  valid;
   } icon;
   const char       *label;
   E_Fm2_Custom_Dir *dir;
   /* FIXME: this will have more added */
};

EAPI int                   e_fm2_custom_file_init(void);
EAPI void                  e_fm2_custom_file_shutdown(void);
EAPI E_Fm2_Custom_File    *e_fm2_custom_file_get(const char *path);
EAPI void                  e_fm2_custom_file_set(const char *path, const E_Fm2_Custom_File *cf);
EAPI void                  e_fm2_custom_file_del(const char *path);
EAPI void                  e_fm2_custom_file_rename(const char *path, const char *new_path);
EAPI void                  e_fm2_custom_file_flush(void);

EAPI E_Fm2_Custom_File    *e_fm2_custom_file_dup(const E_Fm2_Custom_File *cf);

#endif
#endif
