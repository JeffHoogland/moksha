/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_THUMB_H
#define E_THUMB_H


EAPI int                   e_thumb_init(void);
EAPI int                   e_thumb_shutdown(void);

EAPI Evas_Object          *e_thumb_icon_add(Evas *evas);
EAPI void                  e_thumb_icon_file_set(Evas_Object *obj, const char *file, const char *key);
EAPI void                  e_thumb_icon_size_set(Evas_Object *obj, int w, int h);
EAPI void                  e_thumb_icon_begin(Evas_Object *obj);
EAPI void                  e_thumb_icon_end(Evas_Object *obj);
EAPI void		   e_thumb_icon_rethumb(Evas_Object *obj);

EAPI void                  e_thumb_client_data(Ecore_Ipc_Event_Client_Data *e);
EAPI void                  e_thumb_client_del(Ecore_Ipc_Event_Client_Del *e);

#endif
#endif
