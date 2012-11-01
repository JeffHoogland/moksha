#ifdef E_TYPEDEFS

typedef enum _E_Alert_Op_Type
{
   E_ALERT_OP_RESTART = 0,
   E_ALERT_OP_EXIT
} E_Alert_Op_Type;

#else
#ifndef E_ALERT_H
#define E_ALERT_H

EINTERN int e_alert_init(void);
EINTERN int e_alert_shutdown(void);

EAPI void e_alert_show(void);

#endif
#endif
