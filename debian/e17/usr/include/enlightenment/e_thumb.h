#ifdef E_TYPEDEFS

#else
#ifndef E_THUMB_H
#define E_THUMB_H


EINTERN int                   e_thumb_init(void);
EINTERN int                   e_thumb_shutdown(void);

EAPI Evas_Object          *e_thumb_icon_add(Evas *evas);
EAPI void                  e_thumb_icon_file_set(Evas_Object *obj, const char *file, const char *key);
EAPI void                  e_thumb_icon_size_set(Evas_Object *obj, int w, int h);
EAPI void                  e_thumb_icon_begin(Evas_Object *obj);
EAPI void                  e_thumb_icon_end(Evas_Object *obj);
EAPI void		   e_thumb_icon_rethumb(Evas_Object *obj);
EAPI const char           *e_thumb_sort_id_get(Evas_Object *obj);

EAPI void                  e_thumb_client_data(Ecore_Ipc_Event_Client_Data *e);
EAPI void                  e_thumb_client_del(Ecore_Ipc_Event_Client_Del *e);

#endif
#endif
