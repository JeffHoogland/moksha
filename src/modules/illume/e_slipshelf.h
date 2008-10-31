#ifndef E_SLIPSHELF_H
#define E_SLIPSHELF_H

typedef struct _E_Slipshelf E_Slipshelf;
typedef struct _E_Event_Slipshelf_Simple E_Event_Slipshelf_Add;
typedef struct _E_Event_Slipshelf_Simple E_Event_Slipshelf_Del;
typedef struct _E_Event_Slipshelf_Simple E_Event_Slipshelf_Change;

#define E_SLIPSHELF_TYPE 0xE1b0771

typedef enum _E_Slipshelf_Action
{
   E_SLIPSHELF_ACTION_HOME,
   E_SLIPSHELF_ACTION_CLOSE,
   E_SLIPSHELF_ACTION_APPS,
   E_SLIPSHELF_ACTION_KEYBOARD,
   E_SLIPSHELF_ACTION_APP_NEXT,
   E_SLIPSHELF_ACTION_APP_PREV
} E_Slipshelf_Action;

struct _E_Slipshelf
{
   E_Object             e_obj_inherit;
   E_Zone              *zone;
   E_Popup             *popup;
   Ecore_X_Window       clickwin;
   Evas_Object         *base_obj;
   Evas_Object         *control_obj;
   Evas_Object         *scrollframe_obj;
   E_Border            *bsel;
   Evas_Object         *vis_obj;
   Evas_Object         *swallow1_obj;
   Evas_Object         *swallow2_obj;
   Eina_List           *handlers;
   E_Border            *focused_border;
   E_Gadcon            *gadcon;
   E_Gadcon            *gadcon_extra;
   Ecore_Timer         *slide_down_timer;
   const char          *themedir;
   const char          *default_title;
   struct {
      struct {
	 void (*func) (const void *data, E_Slipshelf *ess, E_Slipshelf_Action action);
	 const void *data;
	 unsigned char enabled : 1;
      } home, close, apps, keyboard, app_next, app_prev;
   } action;
   int                  main_size;
   int                  extra_size;
   int                  hidden;
   Ecore_Animator      *animator;
   int                  adjust_start;
   int                  adjust_target;
   int                  adjust;
   struct {
      int               w, h;
   } control;
   struct {
      void (*func) (void *data, E_Slipshelf *ess, E_Border *bd);
      const void *data;
   } callback_border_select, callback_border_home;
   
   double               start;
   double               len;
   unsigned char        out : 1;
};

struct _E_Event_Slipshelf_Simple
{
   E_Slipshelf *slipshelf;
};

EAPI int e_slipshelf_init(void);
EAPI int e_slipshelf_shutdown(void);
EAPI E_Slipshelf *e_slipshelf_new(E_Zone *zone, const char *themedir);
EAPI void e_slipshelf_action_enabled_set(E_Slipshelf *ess, E_Slipshelf_Action action, Evas_Bool enabled);
EAPI Evas_Bool e_slipshelf_action_enabled_get(E_Slipshelf *ess, E_Slipshelf_Action action);
EAPI void e_slipshelf_action_callback_set(E_Slipshelf *ess, E_Slipshelf_Action action, void (*func) (const void *data, E_Slipshelf *ess, E_Slipshelf_Action action), const void *data);
EAPI void e_slipshelf_safe_app_region_get(E_Zone *zone, int *x, int *y, int *w, int *h);
EAPI void e_slipshelf_default_title_set(E_Slipshelf *ess, const char *title);

EAPI void e_slipshelf_border_select_callback_set(E_Slipshelf *ess, void (*func) (void *data, E_Slipshelf *ess, E_Border *bd), const void *data);
EAPI void e_slipshelf_border_home_callback_set(E_Slipshelf *ess, void (*func) (void *data, E_Slipshelf *ess, E_Border *bd), const void *data);


extern EAPI int E_EVENT_SLIPSHELF_ADD;
extern EAPI int E_EVENT_SLIPSHELF_DEL;
extern EAPI int E_EVENT_SLIPSHELF_CHANGE;

#endif
