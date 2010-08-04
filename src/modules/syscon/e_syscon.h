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
