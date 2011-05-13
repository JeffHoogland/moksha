#include "e.h"

/* local subsystem functions */

/* externally accessible functions */
EINTERN int
e_env_init(void)
{
   Eina_List *l;
   E_Config_Env_Var *evr;
   
   EINA_LIST_FOREACH(e_config->env_vars, l, evr)
     {
        if (evr->unset)
           e_env_unset(evr->var);
        else
           e_env_set(evr->var, evr->val);
     }
   return 1;
}

EINTERN int
e_env_shutdown(void)
{
   Eina_List *l;
   E_Config_Env_Var *evr;
   
   EINA_LIST_FOREACH(e_config->env_vars, l, evr)
     {
        e_env_unset(evr->var);
     }
   return 1;
}

EAPI void
e_env_set(const char *var, const char *val)
{
   e_util_env_set(var, val);
}

EAPI void
e_env_unset(const char *var)
{
   e_util_env_set(var, NULL);
}

/* local subsystem functions */
