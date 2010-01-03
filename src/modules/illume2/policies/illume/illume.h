#ifndef _ILLUME_H
# define _ILLUME_H

EAPI extern E_Illume_Layout_Api e_layapi;

EAPI void *e_layapi_init(E_Illume_Layout_Policy *p);
EAPI int e_layapi_shutdown(E_Illume_Layout_Policy *p);
EAPI void e_layapi_config(E_Container *con, const char *params);

#endif
