/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Fm2_Custom_File E_Fm2_Custom_File;

#else
#ifndef E_FM_CUSTOM_H
#define E_FM_CUSTOM_H

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
   /* FIXME: this will have more added */
};

EAPI int                   e_fm2_custom_file_init(void);
EAPI void                  e_fm2_custom_file_shutdown(void);
EAPI E_Fm2_Custom_File    *e_fm2_custom_file_get(const char *path);
EAPI void                  e_fm2_custom_file_set(const char *path, E_Fm2_Custom_File *cf);
EAPI void                  e_fm2_custom_file_del(const char *path);
EAPI void                  e_fm2_custom_file_rename(const char *path, const char *new_path);
EAPI void                  e_fm2_custom_file_flush(void);

#endif
#endif
