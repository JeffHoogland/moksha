#ifdef E_TYPEDEFS

#else
#ifndef E_ALERT_H
#define E_ALERT_H

EINTERN int e_alert_init(const char *disp);
EINTERN int e_alert_shutdown(void);

void e_alert_show(const char *text);

#endif
#endif
