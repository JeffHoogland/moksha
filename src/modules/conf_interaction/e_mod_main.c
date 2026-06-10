#include "e.h"
#include "e_mod_main.h"

#define DURATION 1.0

/* actual module specifics */
static E_Module *conf_module = NULL;
static E_Action *act = NULL;

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Settings - Interaction"
};

typedef struct
{
   Ecore_Evas *ee;
   Evas *evas;

   Evas_Object *square1;
   Evas_Object *square2;
   Evas_Object *square3;

   int mx, my;
} App;

static void
set_color(Evas_Object *o, int r, int g, int b, int a)
{
   r = r * a / 255;
   g = g * a / 255;
   b = b * a / 255;

   evas_object_color_set(o, r, g, b, a);
}

static Eina_Bool
animate(void *data, double pos)
{
   App *app = data;

   if (pos >= 1.0)
     {
       ecore_evas_free(app->ee);
       free(app);
       return ECORE_CALLBACK_CANCEL;
     }

   double g = 1.0 - pow(1.0 - pos, 3.0);

   int size = 60 + (int)(g * 120);

   int x = app->mx - size / 2;
   int y = app->my - size / 2;

   ecore_evas_move(app->ee, x, y);
   ecore_evas_resize(app->ee, size, size);

   int cx = size / 2;
   int cy = size / 2;

   Evas_Object *squares[3] = {
       app->square1,
       app->square2,
       app->square3
   };

   for (int i = 0; i < 3; i++)
     {
       double phase = g - (i * 0.15);

       if (phase < 0.0)
         {
           evas_object_hide(squares[i]);
           continue;
         }

       evas_object_show(squares[i]);

       double radius = phase * (size / 2);
       int alpha = (int)(255 * (1.0 - phase));
       if (alpha < 0) alpha = 0;

       int rsize = (int)(radius * 2);

       evas_object_resize(squares[i], rsize, rsize);
       evas_object_move(squares[i],
                        cx - rsize / 2,
                        cy - rsize / 2);

       set_color(squares[i], 255, 140, 0, alpha);
     }

   return ECORE_CALLBACK_RENEW;
}

int find_cur(void)
{
   App *app;
   app = E_NEW(App, 1);

   app->ee = ecore_evas_new(NULL, 0, 0, 1, 1, NULL);
   
   if (!app->ee)
     {
       free(app);
       return 1;
     }
   ecore_evas_pointer_xy_get(app->ee, &app->mx, &app->my);

   ecore_evas_alpha_set(app->ee, EINA_TRUE);
   ecore_evas_borderless_set(app->ee, EINA_TRUE);
   ecore_evas_override_set(app->ee, EINA_TRUE);
   ecore_evas_show(app->ee);

   app->evas = ecore_evas_get(app->ee);
   
   app->square1 = evas_object_rectangle_add(app->evas);
   app->square2 = evas_object_rectangle_add(app->evas);
   app->square3 = evas_object_rectangle_add(app->evas);

   ecore_animator_timeline_add(DURATION, animate, app);

   return 0;
}

static void
_cb_action_go(E_Object *obj __UNUSED__, const char *params __UNUSED__)
{
   find_cur();
}


EAPI void *
e_modapi_init(E_Module *m)
{
   e_configure_registry_category_add("keyboard_and_mouse", 40, _("Input"),
                                     NULL, "preferences-behavior");
   e_configure_registry_item_add("keyboard_and_mouse/interaction", 40, 
                                 _("Touch"), NULL, 
                                 "preferences-interaction", 
                                 e_int_config_interaction);
   e_configure_registry_item_add("keyboard_and_mouse/mouse_settings", 50,
                                 _("Mouse and Touchpad"), NULL,
                                 "preferences-desktop-mouse",
                                 e_int_config_mouse);

   act = e_action_add("find_cursor");
   if (act)
     {
       act->func.go = _cb_action_go;
       e_action_predef_name_set(_("Cursor"), _("Find Cursor"), "find_cursor",  NULL, NULL, 0);
     }

   conf_module = m;
   e_module_delayed_set(m, 1);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   E_Config_Dialog *cfd;

   while ((cfd = e_config_dialog_get("E", "keyboard_and_mouse/mouse_settings")))
      e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "keyboard_and_mouse/interaction"))) 
     e_object_del(E_OBJECT(cfd));

   e_configure_registry_item_del("keyboard_and_mouse/mouse_settings");
   e_configure_registry_item_del("keyboard_and_mouse/interaction");

   e_configure_registry_category_del("keyboard_and_mouse");

   if (act)
     {
       e_action_predef_name_del("Cursor", "Find Cursor");
       e_action_del("find_cursor");
       act = NULL;
     }

   conf_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
