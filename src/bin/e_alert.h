/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_ALERT_H
#define E_ALERT_H

int e_alert_init(const char *disp);
int e_alert_shutdown(void);

void e_alert_show(const char *text);

#endif
#endif
