#ifndef E_SLIPWIN_H
#define E_SLIPWIN_H

typedef struct _E_Slipwin E_Slipwin;
typedef struct _E_Event_Slipwin_Simple E_Event_Slipwin_Del;

#define E_SLIPWIN_TYPE 0xE1b0971

struct _E_Slipwin
{
   E_Object             e_obj_inherit;
   E_Zone              *zone;
   E_Popup             *popup;
   Ecore_X_Window       clickwin;
   Evas_Object         *base_obj;
   Evas_Object         *scrollframe_obj;
   Evas_Object         *ilist_obj;
   Eina_List           *handlers;
   E_Border            *focused_border;
   Eina_List           *borders;
   struct {
      void (*func) (void *data, E_Slipwin *ess, E_Border *bd);
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

struct _E_Event_Slipwin_Simple
{
   E_Slipwin *slipwin;
};

EAPI int e_slipwin_init(void);
EAPI int e_slipwin_shutdown(void);
EAPI E_Slipwin *e_slipwin_new(E_Zone *zone, const char *themedir);
EAPI void e_slipwin_show(E_Slipwin *esw);
EAPI void e_slipwin_hide(E_Slipwin *esw);
EAPI void e_slipwin_border_select_callback_set(E_Slipwin *esw, void (*func) (void *data, E_Slipwin *ess, E_Border *bd), const void *data);

extern EAPI int E_EVENT_SLIPWIN_DEL;

#endif
