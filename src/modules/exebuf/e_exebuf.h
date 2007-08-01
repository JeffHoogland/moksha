/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_EXEBUF_H
#define E_EXEBUF_H

EAPI int e_exebuf_init(void);
EAPI int e_exebuf_shutdown(void);

EAPI int  e_exebuf_show(E_Zone *zone);
EAPI void e_exebuf_hide(void);

#endif
#endif
