/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_SYSCON_H
#define E_SYSCON_H

int e_syscon_init(void);
int e_syscon_shutdown(void);

int  e_syscon_show(E_Zone *zone, const char *defact);
void e_syscon_hide(void);

#endif
#endif
