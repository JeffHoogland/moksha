#ifndef E_FLAUNCH_H
#define E_FLAUNCH_H

typedef struct _E_Flaunch     E_Flaunch;
typedef struct _E_Flaunch_App E_Flaunch_App;

#define E_FLAUNCH_TYPE 0xE1b7274

struct _E_Flaunch
{
   E_Object             e_obj_inherit;
   int                  height;
   const char          *themedir;
   Evas_Object         *box_obj, *app_box_obj;
   Eina_List           *apps;
   E_Flaunch_App       *start_button;
   E_Zone              *zone;
   void               (*desktop_run_func) (Efreet_Desktop *desktop);
   Ecore_Event_Handler *zone_resize_handler;
   Eina_List           *handlers;
   Ecore_Timer         *repopulate_timer;
};

struct _E_Flaunch_App
{
   E_Flaunch *flaunch;
   Evas_Object *obj;
   struct {
      void (*func) (void *data);
      const void *data;
   } callback;
   const char *desktop;
};

EAPI int e_flaunch_init(void);
EAPI int e_flaunch_shutdown(void);
EAPI E_Flaunch *e_flaunch_new(E_Zone *zone, const char *themedir);
EAPI void e_flaunch_desktop_exec_callback_set(E_Flaunch *fl, void (*func) (Efreet_Desktop *desktop));

#endif
