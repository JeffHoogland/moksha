/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_SYSCON_H
#define E_SYSCON_H

EAPI int e_syscon_init(void);
EAPI int e_syscon_shutdown(void);

EAPI int  e_syscon_show(E_Zone *zone, const char *defact);
EAPI void e_syscon_hide(void);

#endif
#endif
