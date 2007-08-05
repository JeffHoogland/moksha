/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_WINLIST_H
#define E_WINLIST_H

EAPI int e_winlist_init(void);
EAPI int e_winlist_shutdown(void);

EAPI int  e_winlist_show(E_Zone *zone);
EAPI void e_winlist_hide(void);
EAPI void e_winlist_next(void);
EAPI void e_winlist_prev(void);
EAPI void e_winlist_modifiers_set(int mod);

#endif
#endif
