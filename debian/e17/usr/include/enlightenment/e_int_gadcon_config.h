#ifdef E_TYPEDEFS
#else
#ifndef E_INT_GADCON_CONFIG_H
#define E_INT_GADCON_CONFIG_H

EAPI void e_int_gadcon_config_shelf   (E_Gadcon *gc);
EAPI void e_int_gadcon_config_toolbar (E_Gadcon *gc);
EAPI void e_int_gadcon_config_hook(E_Gadcon *gc, const char *name, E_Gadcon_Site site);

#endif
#endif
