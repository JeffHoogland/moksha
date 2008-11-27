/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_EXEHIST_H
#define E_EXEHIST_H

EAPI int e_exehist_init(void);
EAPI int e_exehist_shutdown(void);

EAPI void e_exehist_add(const char *launch_method, const char *exe);
EAPI void e_exehist_del(const char *exe);
EAPI void e_exehist_clear(void);
EAPI int e_exehist_popularity_get(const char *exe);
EAPI double e_exehist_newest_run_get(const char *exe);
EAPI Eina_List *e_exehist_list_get(void);
EAPI void e_exehist_mime_desktop_add(const char *mime, Efreet_Desktop *desktop);
EAPI Efreet_Desktop *e_exehist_mime_desktop_get(const char *mime);

#endif
#endif
