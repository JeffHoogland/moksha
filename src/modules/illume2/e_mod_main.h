#ifndef E_MOD_MAIN_H
# define E_MOD_MAIN_H

# ifdef E_ILLUME_DEFAULT_LOG_COLOR
#  undef E_ILLUME_DEFAULT_LOG_COLOR
# endif
# define E_ILLUME_DEFAULT_LOG_COLOR EINA_COLOR_BLUE

extern int _e_illume_log_dom;
# ifdef E_ILLUME_ERR
#  undef E_ILLUME_ERR
# endif
# define E_ILLUME_ERR(...) EINA_LOG_DOM_ERR(_e_illume_log_dom, __VA_ARGS__)

# ifdef E_ILLUME_INF
#  undef E_ILLUME_INF
# endif
# define E_ILLUME_INF(...) EINA_LOG_DOM_INFO(_e_illume_log_dom, __VA_ARGS__)

# ifdef E_ILLUME_CRIT
#  undef E_ILLUME_CRIT
# endif
# define E_ILLUME_CRIT(...) EINA_LOG_DOM_INFO(_e_illume_log_dom, __VA_ARGS__)

# ifdef E_ILLUME_WARN
#  undef E_ILLUME_WARN
# endif
# define E_ILLUME_WARN(...) EINA_LOG_DOM_WARN(_e_illume_log_dom, __VA_ARGS__)

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

#endif
