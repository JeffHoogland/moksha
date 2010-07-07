/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_WINLIST_H
#define E_WINLIST_H

int e_winlist_init(void);
int e_winlist_shutdown(void);

int  e_winlist_show(E_Zone *zone);
void e_winlist_hide(void);
void e_winlist_next(void);
void e_winlist_prev(void);
void e_winlist_left(E_Zone *zone);
void e_winlist_right(E_Zone *zone);
void e_winlist_down(E_Zone *zone);
void e_winlist_up(E_Zone *zone);
void e_winlist_modifiers_set(int mod);

#endif
#endif
