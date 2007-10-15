/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_FM_MIME_H
#define E_FM_MIME_H

typedef struct _E_Fm2_Mime_Handler E_Fm2_Mime_Handler;

struct _E_Fm2_Mime_Handler 
{
   const char *label, *icon_group;
   void (*action_func) (Evas_Object *obj, const char *path, void *data);
   int (*test_func) (Evas_Object *obj, const char *path, void *data);
   void *action_data;
   void *test_data;
};

EAPI const char *e_fm_mime_filename_get(const char *fname);
EAPI const char *e_fm_mime_icon_get(const char *mime);
EAPI void e_fm_mime_icon_cache_flush(void);

EAPI E_Fm2_Mime_Handler *e_fm2_mime_handler_new(const char *label, const char *icon_group, void (*action_func) (Evas_Object *obj, const char *path, void *data), void *action_data, int (test_func) (Evas_Object *obj, const char *path, void *data), void *test_data);
EAPI void e_fm2_mime_handler_free(E_Fm2_Mime_Handler *handler);
EAPI Evas_Bool e_fm2_mime_handler_mime_add(E_Fm2_Mime_Handler *handler, const char *mime);
EAPI Evas_Bool e_fm2_mime_handler_glob_add(E_Fm2_Mime_Handler *handler, const char *glob);
EAPI Evas_Bool e_fm2_mime_handler_call(E_Fm2_Mime_Handler *handler, Evas_Object *obj, const char *path);
EAPI Evas_Bool e_fm2_mime_handler_test(E_Fm2_Mime_Handler *handler, Evas_Object *obj, const char *path);
EAPI void e_fm2_mime_handler_mime_handlers_call_all(Evas_Object *obj, const char *path, const char *mime);
EAPI void e_fm2_mime_handler_glob_handlers_call_all(Evas_Object *obj, const char *path, const char *glob);
EAPI void e_fm2_mime_handler_mime_del(E_Fm2_Mime_Handler *handler, const char *mime);
EAPI void e_fm2_mime_handler_glob_del(E_Fm2_Mime_Handler *handler, const char *glob);
EAPI Evas_List *e_fm2_mime_handler_mime_handlers_get(const char *mime);
EAPI Evas_List *e_fm2_mime_handler_glob_handlers_get(const char *glob);

#endif
#endif
