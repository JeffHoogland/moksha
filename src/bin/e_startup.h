#ifndef E_STARTUP_H
#define E_STARTUP_H

typedef enum _E_Startup_Mode
{
   E_STARTUP_START,
   E_STARTUP_RESTART
} E_Startup_Mode;

EAPI void e_startup(E_Startup_Mode mode);

#endif
