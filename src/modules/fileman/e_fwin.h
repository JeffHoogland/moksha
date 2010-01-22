/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_FWIN_H
#define E_FWIN_H

int  e_fwin_init          (void);
int  e_fwin_shutdown      (void);
void e_fwin_new           (E_Container *con, const char *dev, const char *path);
void e_fwin_zone_new      (E_Zone *zone, const char *dev, const char *path);
void e_fwin_zone_shutdown (E_Zone *zone);
void e_fwin_all_unsel     (void *data);
void e_fwin_reload_all    (void);
int  e_fwin_zone_find     (E_Zone *zone);

#endif
#endif
