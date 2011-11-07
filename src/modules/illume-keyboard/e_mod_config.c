#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_kbd_int.h"

/* local function prototypes */
static void *_il_kbd_config_create(E_Config_Dialog *cfd);
static void _il_kbd_config_free(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_il_kbd_config_ui(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _il_kbd_config_changed(void *data, Evas_Object *obj, void *event);
static Eina_Bool _il_kbd_config_change_timeout(void *data);

EAPI Il_Kbd_Config *il_kbd_cfg = NULL;
static E_Config_DD *conf_edd = NULL;
Ecore_Timer *_il_kbd_config_change_timer = NULL;
int kbd_external = 0;

/* public functions */
EAPI int 
il_kbd_config_init(E_Module *m) 
{
   char buff[PATH_MAX];

   conf_edd = E_CONFIG_DD_NEW("Illume_Kbd_Cfg", Il_Kbd_Config);
   #undef T
   #undef D
   #define T Il_Kbd_Config
   #define D conf_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, use_internal, INT);
   E_CONFIG_VAL(D, T, run_keyboard, STR);
   E_CONFIG_VAL(D, T, dict, STR);
   E_CONFIG_VAL(D, T, zoom_level, INT);
   E_CONFIG_VAL(D, T, hold_timer, DOUBLE);
   E_CONFIG_VAL(D, T, slide_dim, INT);
   E_CONFIG_VAL(D, T, scale_height, DOUBLE);
   E_CONFIG_VAL(D, T, layout, INT);
   il_kbd_cfg = e_config_domain_load("module.illume-keyboard", conf_edd);
   if ((il_kbd_cfg) && 
       ((il_kbd_cfg->version >> 16) < IL_CONFIG_MAJ)) 
     {
        E_FREE(il_kbd_cfg);
        il_kbd_cfg = NULL;
     }
   if (!il_kbd_cfg) 
     {
        il_kbd_cfg = E_NEW(Il_Kbd_Config, 1);
        il_kbd_cfg->version = 0;
        il_kbd_cfg->use_internal = 1;
        il_kbd_cfg->run_keyboard = NULL;
        il_kbd_cfg->dict = eina_stringshare_add("English_(US).dic");
        il_kbd_cfg->zoom_level = 4;
        il_kbd_cfg->slide_dim = 4;
        il_kbd_cfg->hold_timer = 0.25;
     }
   if (il_kbd_cfg) 
     {
        /* Add new config variables here */
        /* if ((il_kbd_cfg->version & 0xffff) < 1) */
        if ((il_kbd_cfg->version & 0xffff) < 2)
          {
             il_kbd_cfg->zoom_level = 4;
             il_kbd_cfg->slide_dim = 4;
             il_kbd_cfg->hold_timer = 0.25;
             il_kbd_cfg->scale_height = 1.0;
          }
        if ((il_kbd_cfg->version & 0xffff) < IL_CONFIG_MIN)
          {
             il_kbd_cfg->layout = E_KBD_INT_TYPE_ALPHA;
          }

        il_kbd_cfg->version = (IL_CONFIG_MAJ << 16) | IL_CONFIG_MIN;
     }

   il_kbd_cfg->mod_dir = eina_stringshare_add(m->dir);

   snprintf(buff, sizeof(buff), "%s/e-module-illume-keyboard.edj",
            il_kbd_cfg->mod_dir);
   e_configure_registry_category_add("illume", 0, _("Illume"), NULL, 
                                     "enlightenment/display");
   e_configure_registry_generic_item_add("illume/keyboard", 0, _("Keyboard"), 
                                         buff, "icon", il_kbd_config_show);
   return 1;
}

EAPI int 
il_kbd_config_shutdown(void) 
{
   il_kbd_cfg->cfd = NULL;

   e_configure_registry_item_del("illume/keyboard");
   e_configure_registry_category_del("illume");

   if (il_kbd_cfg->mod_dir) eina_stringshare_del(il_kbd_cfg->mod_dir);
   if (il_kbd_cfg->run_keyboard) eina_stringshare_del(il_kbd_cfg->run_keyboard);
   if (il_kbd_cfg->dict) eina_stringshare_del(il_kbd_cfg->dict);

   E_FREE(il_kbd_cfg);
   il_kbd_cfg = NULL;

   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int 
il_kbd_config_save(void) 
{
   e_config_domain_save("module.illume-keyboard", conf_edd, il_kbd_cfg);
   return 1;
}

EAPI void 
il_kbd_config_show(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "_config_illume_keyboard_settings")) return;

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _il_kbd_config_create;
   v->free_cfdata = _il_kbd_config_free;
   v->basic.create_widgets = _il_kbd_config_ui;
   v->basic_only = 1;
   v->normal_win = 1;
   v->scroll = 1;

   cfd = e_config_dialog_new(con, _("Keyboard Settings"), "E", 
                             "_config_illume_keyboard_settings", 
                             "enlightenment/keyboard_settings", 0, v, NULL);
   e_dialog_resizable_set(cfd->dia, 1);
   il_kbd_cfg->cfd = cfd;
}

/* local function prototypes */
static void *
_il_kbd_config_create(E_Config_Dialog *cfd __UNUSED__) 
{
   return NULL;
}

static void 
_il_kbd_config_free(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata __UNUSED__) 
{
   il_kbd_cfg->cfd = NULL;
}

static Evas_Object *
_il_kbd_config_ui(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata __UNUSED__) 
{
   Evas_Object *list, *of, *ow, *sl, *ol;
   E_Radio_Group *rg;
   Eina_List *l;

   list = e_widget_list_add(evas, 0, 0);
   if (!il_kbd_cfg->run_keyboard) 
     {
        if (il_kbd_cfg->use_internal) kbd_external = 1;
        else kbd_external = 0;
     }
   else 
     {
        Eina_List *kbds;
        Efreet_Desktop *desktop;
        int nn = 0;

        kbd_external = 0;
        kbds = efreet_util_desktop_category_list("Keyboard");
        if (kbds) 
          {
             nn = 2;
             EINA_LIST_FOREACH(kbds, l, desktop) 
               {
                  const char *dname;

                  dname = ecore_file_file_get(desktop->orig_path);
                  if (dname) 
                    {
                       if (!strcmp(il_kbd_cfg->run_keyboard, dname)) 
                         {
                            kbd_external = nn;
                            break;
                         }
                    }
                  nn++;
               }
             EINA_LIST_FREE(kbds, desktop)
               efreet_desktop_free(desktop);
          }
     }

   of = e_widget_framelist_add(evas, _("Keyboards"), 0);
   rg = e_widget_radio_group_new(&(kbd_external));
   ow = e_widget_radio_add(evas, _("None"), 0, rg);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", _il_kbd_config_changed, NULL);
   ow = e_widget_radio_add(evas, _("Default"), 1, rg);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", _il_kbd_config_changed, NULL);
     {
        Eina_List *kbds;
        Efreet_Desktop *desktop;
        int nn = 2;

        kbds = efreet_util_desktop_category_list("Keyboard");
        EINA_LIST_FREE(kbds, desktop) 
          {
             ow = e_widget_radio_add(evas, desktop->name, nn, rg);
             e_widget_framelist_object_append(of, ow);
             evas_object_smart_callback_add(ow, "changed", 
                                            _il_kbd_config_changed, NULL);
             efreet_desktop_free(desktop);
             nn++;
          }
     }
   
   ol = e_widget_label_add(evas, _("Displacement ratio"));
   e_widget_framelist_object_append(of, ol);
   sl = e_widget_slider_add(evas, EINA_TRUE, 0, "1/%.0f", 1.0, 10.0, 1.0, 0,
                            NULL, &(il_kbd_cfg->slide_dim), 150);
   e_widget_framelist_object_append(of, sl);

   ol = e_widget_label_add(evas, _("Delay for zoom popup"));
   e_widget_framelist_object_append(of, ol);
   sl = e_widget_slider_add(evas, EINA_TRUE, 0, "%.2f second(s)", 0.0, 3.0, 0.01, 0,
                            &(il_kbd_cfg->hold_timer), NULL, 150);
   e_widget_framelist_object_append(of, sl);

   ol = e_widget_label_add(evas, _("Zoom level"));
   e_widget_framelist_object_append(of, ol);
   sl = e_widget_slider_add(evas, EINA_TRUE, 0, "%.0f", 1.0, 10.0, 1.0, 0,
                            NULL, &(il_kbd_cfg->zoom_level), 150);
   e_widget_framelist_object_append(of, sl);

   ol = e_widget_label_add(evas, _("Height"));
   e_widget_framelist_object_append(of, ol);
   sl = e_widget_slider_add(evas, EINA_TRUE, 0, "%.2f", 0.2, 2.0, 0.1, 0,
                            &(il_kbd_cfg->scale_height), NULL, 150);
   evas_object_smart_callback_add(sl, "changed", 
                                  _il_kbd_config_changed, NULL);
        
   e_widget_framelist_object_append(of, sl);

   e_widget_list_object_append(list, of, 1, 0, 0.0);

   of = e_widget_framelist_add(evas, _("Layout"), 0);
   rg = e_widget_radio_group_new(&(il_kbd_cfg->layout));
   ow = e_widget_radio_add(evas, _("Default"), E_KBD_INT_TYPE_ALPHA, rg);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", _il_kbd_config_changed, NULL);
   ow = e_widget_radio_add(evas, _("Terminal"), E_KBD_INT_TYPE_TERMINAL, rg);
   e_widget_framelist_object_append(of, ow);
   evas_object_smart_callback_add(ow, "changed", _il_kbd_config_changed, NULL);
   e_widget_list_object_append(list, of, 1, 0, 0.0);
   
   return list;
}

static void 
_il_kbd_config_changed(void *data, Evas_Object *obj __UNUSED__, void *event __UNUSED__) 
{
   if (_il_kbd_config_change_timer) 
     ecore_timer_del(_il_kbd_config_change_timer);
   _il_kbd_config_change_timer = 
     ecore_timer_add(0.5, _il_kbd_config_change_timeout, data);
}

static Eina_Bool
_il_kbd_config_change_timeout(void *data __UNUSED__)
{
   Eina_List *l;

   il_kbd_cfg->use_internal = 0;
   if (il_kbd_cfg->run_keyboard) 
     eina_stringshare_del(il_kbd_cfg->run_keyboard);
   il_kbd_cfg->run_keyboard = NULL;
   if (kbd_external == 0)
     il_kbd_cfg->use_internal = 0;
   else if (kbd_external == 1)
     il_kbd_cfg->use_internal = 1;
   else 
     {
        Eina_List *kbds;
        Efreet_Desktop *desktop;
        int nn;

        kbds = efreet_util_desktop_category_list("Keyboard");
        if (kbds) 
          {
             nn = 2;
             EINA_LIST_FOREACH(kbds, l, desktop) 
               {
                  const char *dname;

                  dname = ecore_file_file_get(desktop->orig_path);
                  if (nn == kbd_external) 
                    {
                       if (dname)
                         il_kbd_cfg->run_keyboard = eina_stringshare_add(dname);
                       break;
                    }
                  nn++;
               }
             EINA_LIST_FREE(kbds, desktop)
               efreet_desktop_free(desktop);
          }
     }

   il_kbd_cfg_update();
   e_config_save_queue();
   _il_kbd_config_change_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}
