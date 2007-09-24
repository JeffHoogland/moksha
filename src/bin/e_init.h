/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_INIT_H
#define E_INIT_H

EAPI int            e_init_init(void);
EAPI int            e_init_shutdown(void);
EAPI void           e_init_show(void);
EAPI void           e_init_hide(void);
EAPI void           e_init_title_set(const char *str);
EAPI void           e_init_version_set(const char *str);
EAPI void           e_init_status_set(const char *str);
EAPI void           e_init_done(void);
EAPI void           e_init_undone(void);
EAPI void           e_init_client_data(Ecore_Ipc_Event_Client_Data *e);
EAPI void           e_init_client_del(Ecore_Ipc_Event_Client_Del *e);
    
#endif
#endif
