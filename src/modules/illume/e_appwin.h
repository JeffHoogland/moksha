#ifndef E_APPWIN_H
#define E_APPWIN_H

typedef struct _E_Appwin E_Appwin;
typedef struct _E_Event_Appwin_Simple E_Event_Appwin_Del;

#define E_APPWIN_TYPE 0xE1b0983

struct _E_Appwin
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
      void (*func) (void *data, E_Appwin *ess, E_Border *bd);
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

struct _E_Event_Appwin_Simple
{
   E_Appwin *appwin;
};

EAPI int e_appwin_init(void);
EAPI int e_appwin_shutdown(void);
EAPI E_Appwin *e_appwin_new(E_Zone *zone, const char *themedir);
EAPI void e_appwin_show(E_Appwin *esw);
EAPI void e_appwin_hide(E_Appwin *esw);
EAPI void e_appwin_border_select_callback_set(E_Appwin *esw, void (*func) (void *data, E_Appwin *ess, E_Border *bd), const void *data);

extern EAPI int E_EVENT_APPWIN_DEL;

#endif
