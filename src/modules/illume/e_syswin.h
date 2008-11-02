#ifndef E_SYSWIN_H
#define E_SYSWIN_H

typedef struct _E_Syswin E_Syswin;
typedef struct _E_Event_Syswin_Simple E_Event_Syswin_Del;

#define E_SYSWIN_TYPE 0xE1b0993

struct _E_Syswin
{
   E_Object             e_obj_inherit;
   E_Zone              *zone;
   E_Popup             *popup;
   Ecore_X_Window       clickwin;
   Evas_Object         *base_obj;
   Evas_Object         *ilist_obj;
   Eina_List           *handlers;
   E_Border            *focused_border;
   Eina_List           *borders;
   struct {
      void (*func) (void *data, E_Syswin *ess, E_Border *bd);
      const void *data;
   } callback;
   const char          *themedir;
   Ecore_Animator      *animator;
   int                  adjust_start;
   int                  adjust_target;
   int                  adjust;
   double               start;
   double               len;
   unsigned char        out : 1;
};

struct _E_Event_Syswin_Simple
{
   E_Syswin *syswin;
};

EAPI int e_syswin_init(void);
EAPI int e_syswin_shutdown(void);
EAPI E_Syswin *e_syswin_new(E_Zone *zone, const char *themedir);
EAPI void e_syswin_show(E_Syswin *esw);
EAPI void e_syswin_hide(E_Syswin *esw);
EAPI void e_syswin_border_select_callback_set(E_Syswin *esw, void (*func) (void *data, E_Syswin *ess, E_Border *bd), const void *data);

extern EAPI int E_EVENT_SYSWIN_DEL;

#endif
