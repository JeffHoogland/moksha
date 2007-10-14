/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_FM_MIME_H
#define E_FM_MIME_H

typedef struct _E_Fm_Mime_Handler E_Fm_Mime_Handler;

struct _E_Fm_Mime_Handler 
{
   const char *label, *icon_group;
   void (*action_func) (Evas_Object *obj, const char *path, void *data);
   int (*test_func) (Evas_Object *obj, const char *path, void *data);
};

EAPI const char *e_fm_mime_filename_get(const char *fname);
EAPI const char *e_fm_mime_icon_get(const char *mime);
EAPI void e_fm_mime_icon_cache_flush(void);

EAPI E_Fm_Mime_Handler *e_fm_mime_handler_new(const char *label, const char *icon_group, void (*action_func) (Evas_Object *obj, const char *path, void *data), int (test_func) (Evas_Object *obj, const char *path, void *data));
EAPI void e_fm_mime_handler_free(E_Fm_Mime_Handler *handler);
EAPI int e_fm_mime_handler_mime_add(E_Fm_Mime_Handler *handler, const char *mime);
EAPI int e_fm_mime_handler_glob_add(E_Fm_Mime_Handler *handler, const char *glob);

#endif
#endif
