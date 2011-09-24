#ifdef E_TYPEDEFS

#else
# ifndef E_XSETTINGS_H
#  define E_XSETTINGS_H

EINTERN int e_xsettings_init(void);
EINTERN int e_xsettings_shutdown(void);

EAPI void e_xsettings_config_update(void);

# endif
#endif
