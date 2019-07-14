#include "e.h"
#include "e_mod_main.h"

/* NOTE: for OpenBSD, as we cannot set the frequency but
 *       only its percent, we store percents 25-50-75-100
 *       in s->frequencies instead of available frequencies.
 */

#if defined(__FreeBSD__) || defined(__DragonFly__) || defined (__OpenBSD__)
# include <sys/sysctl.h>
#endif

/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void             _gc_shutdown(E_Gadcon_Client *gcc);
static void             _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char      *_gc_label(const E_Gadcon_Client_Class *client_class);
static Evas_Object     *_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas);
static const char      *_gc_id_new(const E_Gadcon_Client_Class *client_class);
/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION, "cpufreq",
   {
      _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, NULL
   },
   E_GADCON_CLIENT_STYLE_PLAIN
};

/* actual module specifics */
typedef struct _Instance Instance;

struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_cpu;
};

static void      _button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void      _menu_cb_post(void *data, E_Menu *m);
static void      _cpufreq_set_frequency(int frequency);
static Cpu_Status *_cpufreq_status_new(void);
static void      _cpufreq_status_free(Cpu_Status *s);
static void      _cpufreq_status_check_available(Cpu_Status *s);
static int       _cpufreq_status_check_current(Cpu_Status *s);
static int       _cpufreq_cb_sort(const void *item1, const void *item2);
static void      _cpufreq_face_update_available(Instance *inst);
static void      _cpufreq_face_update_current(Instance *inst);
static void      _cpufreq_face_cb_set_frequency(void *data, Evas_Object *o, const char *emission, const char *source);
static void      _cpufreq_face_cb_set_governor(void *data, Evas_Object *o, const char *emission, const char *source);
static Eina_Bool _cpufreq_event_cb_powersave(void *data __UNUSED__, int type, void *event);

static void      _cpufreq_menu_fast(void *data, E_Menu *m, E_Menu_Item *mi);
static void      _cpufreq_menu_medium(void *data, E_Menu *m, E_Menu_Item *mi);
static void      _cpufreq_menu_normal(void *data, E_Menu *m, E_Menu_Item *mi);
static void      _cpufreq_menu_slow(void *data, E_Menu *m, E_Menu_Item *mi);
static void      _cpufreq_menu_very_slow(void *data, E_Menu *m, E_Menu_Item *mi);
static void      _cpufreq_menu_restore_governor(void *data, E_Menu *m, E_Menu_Item *mi);
static void      _cpufreq_menu_auto_powersave(void *data, E_Menu *m, E_Menu_Item *mi);
static void      _cpufreq_menu_governor(void *data, E_Menu *m, E_Menu_Item *mi);
static void      _cpufreq_menu_powersave_governor(void *data, E_Menu *m, E_Menu_Item *mi);
static void      _cpufreq_menu_frequency(void *data, E_Menu *m, E_Menu_Item *mi);
static void      _cpufreq_menu_pstate_min(void *data, E_Menu *m, E_Menu_Item *mi);
static void      _cpufreq_menu_pstate_max(void *data, E_Menu *m, E_Menu_Item *mi);

static E_Config_DD *conf_edd = NULL;

Config *cpufreq_config = NULL;

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;

   inst = E_NEW(Instance, 1);

   o = edje_object_add(gc->evas);
   e_theme_edje_object_set(o, "base/theme/modules/cpufreq",
                           "e/modules/cpufreq/main");
   edje_object_signal_callback_add(o, "e,action,governor,next", "*",
                                   _cpufreq_face_cb_set_governor, NULL);
   edje_object_signal_callback_add(o, "e,action,frequency,increase", "*",
                                   _cpufreq_face_cb_set_frequency, NULL);
   edje_object_signal_callback_add(o, "e,action,frequency,decrease", "*",
                                   _cpufreq_face_cb_set_frequency, NULL);

   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;

   inst->gcc = gcc;
   inst->o_cpu = o;

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
                                  _button_cb_mouse_down, inst);
   cpufreq_config->instances =
     eina_list_append(cpufreq_config->instances, inst);

   _cpufreq_face_update_available(inst);

   cpufreq_config->handler =
     ecore_event_handler_add(E_EVENT_POWERSAVE_UPDATE,
                             _cpufreq_event_cb_powersave, NULL);
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

   inst = gcc->data;
   cpufreq_config->instances =
     eina_list_remove(cpufreq_config->instances, inst);
   evas_object_del(inst->o_cpu);
   free(inst);

   if (cpufreq_config->handler)
     ecore_event_handler_del(cpufreq_config->handler);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__)
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("Cpufreq");
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;
   char buf[PATH_MAX];

   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-cpufreq.edj",
            e_module_dir_get(cpufreq_config->module));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   static char idbuff[32];

   snprintf(idbuff, sizeof(idbuff), "%s.%d", _gadcon_class.name,
            eina_list_count(cpufreq_config->instances));
   return idbuff;
}

static void
_cpufreq_cb_menu_configure(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   if (!cpufreq_config) return;
   if (cpufreq_config->config_dialog) return;
   e_int_config_cpufreq_module(NULL, NULL);
}

static void
_button_cb_mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Instance *inst;
   Evas_Event_Mouse_Down *ev;

   inst = data;
   ev = event_info;

   if (ev->button == 1)
     {
        E_Menu *mg, *mo;
        E_Menu_Item *mi;
        Eina_List *l;
        int cx, cy;
        char buf[256];

        if (cpufreq_config->menu_poll) return;
        mo = e_menu_new();
        cpufreq_config->menu_poll = mo;

        mi = e_menu_item_new(mo);
        e_menu_item_label_set(mi, _("Fast (4 ticks)"));
        e_menu_item_radio_set(mi, 1);
        e_menu_item_radio_group_set(mi, 1);
        if (cpufreq_config->poll_interval <= 4) e_menu_item_toggle_set(mi, 1);
        e_menu_item_callback_set(mi, _cpufreq_menu_fast, NULL);

        mi = e_menu_item_new(mo);
        e_menu_item_label_set(mi, _("Medium (8 ticks)"));
        e_menu_item_radio_set(mi, 1);
        e_menu_item_radio_group_set(mi, 1);
        if (cpufreq_config->poll_interval > 4) e_menu_item_toggle_set(mi, 1);
        e_menu_item_callback_set(mi, _cpufreq_menu_medium, NULL);

        mi = e_menu_item_new(mo);
        e_menu_item_label_set(mi, _("Normal (32 ticks)"));
        e_menu_item_radio_set(mi, 1);
        e_menu_item_radio_group_set(mi, 1);
        if (cpufreq_config->poll_interval >= 32) e_menu_item_toggle_set(mi, 1);
        e_menu_item_callback_set(mi, _cpufreq_menu_normal, NULL);

        mi = e_menu_item_new(mo);
        e_menu_item_label_set(mi, _("Slow (64 ticks)"));
        e_menu_item_radio_set(mi, 1);
        e_menu_item_radio_group_set(mi, 1);
        if (cpufreq_config->poll_interval >= 64) e_menu_item_toggle_set(mi, 1);
        e_menu_item_callback_set(mi, _cpufreq_menu_slow, NULL);

        mi = e_menu_item_new(mo);
        e_menu_item_label_set(mi, _("Very Slow (256 ticks)"));
        e_menu_item_radio_set(mi, 1);
        e_menu_item_radio_group_set(mi, 1);
        if (cpufreq_config->poll_interval >= 128)
          e_menu_item_toggle_set(mi, 1);
        e_menu_item_callback_set(mi, _cpufreq_menu_very_slow, NULL);

        if (cpufreq_config->status->governors)
          {
             mo = e_menu_new();
             cpufreq_config->menu_governor = mo;

             for (l = cpufreq_config->status->governors; l; l = l->next)
               {
                  mi = e_menu_item_new(mo);
                  if (!strcmp(l->data, "userspace"))
                    e_menu_item_label_set(mi, _("Manual"));
                  else if (!strcmp(l->data, "ondemand"))
                    e_menu_item_label_set(mi, _("Automatic"));
                  else if (!strcmp(l->data, "conservative"))
                    e_menu_item_label_set(mi, _("Lower Power Automatic"));
                  else if (!strcmp(l->data, "interactive"))
                    e_menu_item_label_set(mi, _("Automatic Interactive"));
                  else if (!strcmp(l->data, "powersave"))
                    e_menu_item_label_set(mi, _("Minimum Speed"));
                  else if (!strcmp(l->data, "performance"))
                    e_menu_item_label_set(mi, _("Maximum Speed"));
                  else
                    e_menu_item_label_set(mi, l->data);
                  e_menu_item_radio_set(mi, 1);
                  e_menu_item_radio_group_set(mi, 1);
                  if (!strcmp(cpufreq_config->status->cur_governor, l->data))
                    e_menu_item_toggle_set(mi, 1);
                  e_menu_item_callback_set(mi, _cpufreq_menu_governor, l->data);
               }

             e_menu_item_separator_set(e_menu_item_new(mo), 1);

             mi = e_menu_item_new(mo);
             e_menu_item_label_set(mi, _("Restore CPU Power Policy"));
             e_menu_item_check_set(mi, 1);
             e_menu_item_toggle_set(mi, cpufreq_config->restore_governor);
             e_menu_item_callback_set(mi, _cpufreq_menu_restore_governor, NULL);

             mo = e_menu_new();
             cpufreq_config->menu_powersave = mo;

             for (l = cpufreq_config->status->governors; l; l = l->next)
               {
                  if (!strcmp(l->data, "userspace"))
                    continue;

                  mi = e_menu_item_new(mo);
                  if (!strcmp(l->data, "ondemand"))
                    e_menu_item_label_set(mi, _("Automatic"));
                  else if (!strcmp(l->data, "conservative"))
                    e_menu_item_label_set(mi, _("Lower Power Automatic"));
                  else if (!strcmp(l->data, "interactive"))
                    e_menu_item_label_set(mi, _("Automatic Interactive"));
                  else if (!strcmp(l->data, "powersave"))
                    e_menu_item_label_set(mi, _("Minimum Speed"));
                  else if (!strcmp(l->data, "performance"))
                    e_menu_item_label_set(mi, _("Maximum Speed"));
                  else
                    e_menu_item_label_set(mi, l->data);

                  e_menu_item_radio_set(mi, 1);
                  e_menu_item_radio_group_set(mi, 1);
                  if (cpufreq_config->powersave_governor
                      && !strcmp(cpufreq_config->powersave_governor, l->data))
                    e_menu_item_toggle_set(mi, 1);
                  e_menu_item_callback_set(mi, _cpufreq_menu_powersave_governor, l->data);
               }

             e_menu_item_separator_set(e_menu_item_new(mo), 1);

             mi = e_menu_item_new(mo);
             e_menu_item_label_set(mi, _("Automatic powersaving"));
             e_menu_item_check_set(mi, 1);
             e_menu_item_toggle_set(mi, cpufreq_config->auto_powersave);
             e_menu_item_callback_set(mi, _cpufreq_menu_auto_powersave, NULL);
          }

        if ((cpufreq_config->status->frequencies) &&
            (cpufreq_config->status->can_set_frequency) &&
            (!cpufreq_config->status->pstate))
          {
             mo = e_menu_new();
             cpufreq_config->menu_frequency = mo;

             for (l = cpufreq_config->status->frequencies; l; l = l->next)
               {
                  int frequency;

                  frequency = (long)l->data;
                  mi = e_menu_item_new(mo);

#ifdef __OpenBSD__
                  snprintf(buf, sizeof(buf), "%i %%", frequency);
#else
                  if (frequency < 1000000)
                    snprintf(buf, sizeof(buf), _("%i MHz"), frequency / 1000);
                  else
                    snprintf(buf, sizeof(buf), _("%'.1f GHz"),
                             frequency / 1000000.);
#endif

                  e_menu_item_label_set(mi, buf);
                  e_menu_item_radio_set(mi, 1);
                  e_menu_item_radio_group_set(mi, 1);

#ifdef __OpenBSD__
                  if (cpufreq_config->status->cur_percent == frequency)
                    e_menu_item_toggle_set(mi, 1);
#else
                  if (cpufreq_config->status->cur_frequency == frequency)
                    e_menu_item_toggle_set(mi, 1);
#endif
                  e_menu_item_callback_set(mi, _cpufreq_menu_frequency, l->data);
               }
          }

        if (cpufreq_config->status->pstate)
          {
             int set;
             
             mo = e_menu_new();
             cpufreq_config->menu_pstate1 = mo;

             set = 0;
#define VALMIN(_n) \
             mi = e_menu_item_new(mo); \
             e_menu_item_label_set(mi, #_n); \
             e_menu_item_radio_set(mi, 1); \
             e_menu_item_radio_group_set(mi, 1); \
             if ((!set) && (cpufreq_config->status->pstate_min <= _n)) \
               { set = 1; e_menu_item_toggle_set(mi, 1); } \
             e_menu_item_callback_set(mi, _cpufreq_menu_pstate_min, (void *)_n)
             VALMIN(0);
             VALMIN(25);
             VALMIN(50);
             VALMIN(75);
             VALMIN(100);
             
             mo = e_menu_new();
             cpufreq_config->menu_pstate2 = mo;
             
             set = 0;
#define VALMAX(_n) \
             mi = e_menu_item_new(mo); \
             e_menu_item_label_set(mi, #_n); \
             e_menu_item_radio_set(mi, 1); \
             e_menu_item_radio_group_set(mi, 1); \
             if ((!set) && (cpufreq_config->status->pstate_max <= _n)) \
               { set = 1; e_menu_item_toggle_set(mi, 1); } \
             e_menu_item_callback_set(mi, _cpufreq_menu_pstate_max, (void *)_n)
             VALMAX(5);
             VALMAX(25);
             VALMAX(50);
             VALMAX(75);
             VALMAX(100);
          }
        
        mg = e_menu_new();
        mi = e_menu_item_new(mg);
        e_menu_item_label_set(mi, _("Time Between Updates"));
        e_menu_item_submenu_set(mi, cpufreq_config->menu_poll);

        if (cpufreq_config->menu_governor)
          {
             mi = e_menu_item_new(mg);
             e_menu_item_label_set(mi, _("Set CPU Power Policy"));
             e_menu_item_submenu_set(mi, cpufreq_config->menu_governor);
          }

        if (cpufreq_config->menu_frequency)
          {
             mi = e_menu_item_new(mg);
             e_menu_item_label_set(mi, _("Set CPU Speed"));
             e_menu_item_submenu_set(mi, cpufreq_config->menu_frequency);
          }
        if (cpufreq_config->menu_powersave)
          {
             mi = e_menu_item_new(mg);
             e_menu_item_label_set(mi, _("Powersaving behavior"));
             e_menu_item_submenu_set(mi, cpufreq_config->menu_powersave);
          }
        if (cpufreq_config->menu_pstate1)
          {
             mi = e_menu_item_new(mg);
             e_menu_item_label_set(mi, _("Power State Min"));
             e_menu_item_submenu_set(mi, cpufreq_config->menu_pstate1);
          }
        if (cpufreq_config->menu_pstate2)
          {
             mi = e_menu_item_new(mg);
             e_menu_item_label_set(mi, _("Power State Max"));
             e_menu_item_submenu_set(mi, cpufreq_config->menu_pstate2);
          }

        e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon,
                                          &cx, &cy, NULL, NULL);
        cpufreq_config->menu = mg;
        e_menu_post_deactivate_callback_set(mg, _menu_cb_post, inst);

        e_gadcon_locked_set(inst->gcc->gadcon, 1);

        e_menu_activate_mouse(mg,
                              e_util_zone_current_get(e_manager_current_get()),
                              cx + ev->output.x, cy + ev->output.y, 1, 1,
                              E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
        evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
                                 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
   else if (ev->button == 3)
     {
        E_Menu *m;
        E_Menu_Item *mi;
        int cx, cy;

        m = e_menu_new();
        
        mi = e_menu_item_new(m);
        e_menu_item_label_set(mi, _("Settings"));
        e_util_menu_item_theme_icon_set(mi, "configure");
        e_menu_item_callback_set(mi, _cpufreq_cb_menu_configure, NULL);
        
        m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);

        e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon,
                                          &cx, &cy, NULL, NULL);

        e_menu_activate_mouse(m,
                              e_util_zone_current_get(e_manager_current_get()),
                              cx + ev->output.x, cy + ev->output.y, 1, 1,
                              E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
        evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
                                 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
}

static void
_menu_cb_post(void *data, E_Menu *m __UNUSED__)
{
   Instance *inst = data;

   if (inst)
     e_gadcon_locked_set(inst->gcc->gadcon, 0);

   if (!cpufreq_config->menu) return;
   e_object_del(E_OBJECT(cpufreq_config->menu));
   cpufreq_config->menu = NULL;
   if (cpufreq_config->menu_poll)
     e_object_del(E_OBJECT(cpufreq_config->menu_poll));
   cpufreq_config->menu_poll = NULL;
   if (cpufreq_config->menu_governor)
     e_object_del(E_OBJECT(cpufreq_config->menu_governor));
   cpufreq_config->menu_governor = NULL;
   if (cpufreq_config->menu_frequency)
     e_object_del(E_OBJECT(cpufreq_config->menu_frequency));
   cpufreq_config->menu_frequency = NULL;
   if (cpufreq_config->menu_powersave)
     e_object_del(E_OBJECT(cpufreq_config->menu_powersave));
   if (cpufreq_config->menu_pstate1)
     e_object_del(E_OBJECT(cpufreq_config->menu_pstate1));
   if (cpufreq_config->menu_pstate2)
     e_object_del(E_OBJECT(cpufreq_config->menu_pstate2));
   cpufreq_config->menu_powersave = NULL;
}

void
_cpufreq_set_governor(const char *governor)
{
   char buf[4096];
   int ret;
   struct stat st;

   if (stat(cpufreq_config->set_exe_path, &st) < 0) return;

   snprintf(buf, sizeof(buf),
            "%s %s %s", cpufreq_config->set_exe_path, "governor", governor);
   ret = system(buf);
   if (ret != 0)
     {
        E_Dialog *dia;

        if (!(dia = e_dialog_new(NULL, "E", "_e_mod_cpufreq_error_setfreq")))
          return;
        e_dialog_title_set(dia, "Enlightenment Cpufreq Module");
        e_dialog_icon_set(dia, "enlightenment", 64);
        e_dialog_text_set(dia, _("There was an error trying to set the<ps/>"
                                 "cpu frequency governor via the module's<ps/>"
                                 "setfreq utility."));
        e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
        e_win_centered_set(dia->win, 1);
        e_dialog_show(dia);
     }
}

static void
_cpufreq_set_frequency(int frequency)
{
   char buf[4096];
   int ret;

#if defined(__FreeBSD__) || defined(__DragonFly__)
   frequency /= 1000;
#endif
   if (!cpufreq_config->status->can_set_frequency)
     {
        E_Dialog *dia;

        if (!(dia = e_dialog_new(NULL, "E", "_e_mod_cpufreq_error_setfreq")))
          return;
        e_dialog_title_set(dia, "Enlightenment Cpufreq Module");
        e_dialog_icon_set(dia, "enlightenment", 64);
        e_dialog_text_set(dia, _("Your kernel does not support setting the<ps/>"
                                 "CPU frequency at all. You may be missing<ps/>"
                                 "Kernel modules or features, or your CPU<ps/>"
                                 "simply does not support this feature."));
        e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
        e_win_centered_set(dia->win, 1);
        e_dialog_show(dia);
        return;
     }

#ifndef __OpenBSD__
   /* OpenBSD doesn't have governors */
   _cpufreq_set_governor("userspace");
#endif

   snprintf(buf, sizeof(buf),
            "%s %s %i", cpufreq_config->set_exe_path, "frequency", frequency);
   ret = system(buf);
   if (ret != 0)
     {
        E_Dialog *dia;

        if (!(dia = e_dialog_new(NULL, "E", "_e_mod_cpufreq_error_setfreq")))
          return;
        e_dialog_title_set(dia, "Enlightenment Cpufreq Module");
        e_dialog_icon_set(dia, "enlightenment", 64);
        e_dialog_text_set(dia, _("There was an error trying to set the<ps/>"
                                 "cpu frequency setting via the module's<ps/>"
                                 "setfreq utility."));
        e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
        e_win_centered_set(dia->win, 1);
        e_dialog_show(dia);
     }
}

void
_cpufreq_set_pstate(int min, int max)
{
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
   (void) min;
   (void) max;
#else
   char buf[4096];
   snprintf(buf, sizeof(buf),
            "%s %s %i %i %i", cpufreq_config->set_exe_path, "pstate", min, max, cpufreq_config->status->pstate_turbo);
   int ret = system(buf);
   if (ret != 0)
     {
        E_Dialog *dia;

        if (!(dia = e_dialog_new(NULL, "E", "_e_mod_cpufreq_error_setfreq")))
          return;
        e_dialog_title_set(dia, "Enlightenment Cpufreq Module");
        e_dialog_icon_set(dia, "enlightenment", 64);
        e_dialog_text_set(dia, _("There was an error trying to set the<ps/>"
                                 "cpu power state setting via the module's<ps/>"
                                 "setfreq utility."));
        e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
        e_win_centered_set(dia->win, 1);
        e_dialog_show(dia);
     }
#endif
}

static Cpu_Status *
_cpufreq_status_new(void)
{
   Cpu_Status *s;

   s = E_NEW(Cpu_Status, 1);
   if (!s) return NULL;
   s->active = -1;
   return s;
}

static void
_cpufreq_status_free(Cpu_Status *s)
{
   Eina_List *l;

   if (s->frequencies) eina_list_free(s->frequencies);
   if (s->governors)
     {
        for (l = s->governors; l; l = l->next)
          free(l->data);
        eina_list_free(s->governors);
     }
   free(s->cur_governor);
   if (s->orig_governor) eina_stringshare_del(s->orig_governor);
   free(s);
}

static int
_cpufreq_cb_sort(const void *item1, const void *item2)
{
   int a, b;

   a = (long)item1;
   b = (long)item2;
   if (a < b) return -1;
   else if (a > b)
     return 1;
   return 0;
}

static void
_cpufreq_status_check_available(Cpu_Status *s)
{
   // FIXME: this assumes all cores accept the same freqs/ might be wrong
#if !defined(__OpenBSD__)
   char buf[4096];
   Eina_List *l;
#endif

#if defined (__OpenBSD__)
   int p;

   if (s->frequencies)
     {
        eina_list_free(s->frequencies);
        s->frequencies = NULL;
     }

   /* storing percents */
   p = 100;
   s->frequencies = eina_list_append(s->frequencies, (void *)(long int)p);
   p = 75;
   s->frequencies = eina_list_append(s->frequencies, (void *)(long int)p);
   p = 50;
   s->frequencies = eina_list_append(s->frequencies, (void *)(long int)p);
   p = 25;
   s->frequencies = eina_list_append(s->frequencies, (void *)(long int)p);
#elif defined (__FreeBSD__) || defined(__DragonFly__)
   int freq;
   size_t len = sizeof(buf);
   char *pos, *q;

   /* read freq_levels sysctl and store it in freq */
   if (sysctlbyname("dev.cpu.0.freq_levels", buf, &len, NULL, 0) == 0)
     {
        /* sysctl returns 0 on success */
        if (s->frequencies)
          {
             eina_list_free(s->frequencies);
             s->frequencies = NULL;
          }

        /* parse freqs and store the frequencies in s->frequencies */
        pos = buf;
        while (pos)
          {
             q = strchr(pos, '/');
             if (!q) break;

             *q = '\0';
             freq = atoi(pos);
             freq *= 1000;
             s->frequencies = eina_list_append(s->frequencies, (void *)(long)freq);

             pos = q + 1;
             pos = strchr(pos, ' ');
          }
     }

   /* sort is not necessary because freq_levels is already sorted */
   /* freebsd doesn't have governors */
   if (s->governors)
     {
        for (l = s->governors; l; l = l->next)
          free(l->data);
        eina_list_free(s->governors);
        s->governors = NULL;
     }
#else
   FILE *f;

   f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies", "r");
   if (f)
     {
        char *freq;

        if (s->frequencies)
          {
             eina_list_free(s->frequencies);
             s->frequencies = NULL;
          }

        if (fgets(buf, sizeof(buf), f) == NULL)
          {
             fclose(f);
             return;
          }
        fclose(f);

        freq = strtok(buf, " ");
        do
          {
             if (atoi(freq) != 0)
               {
                  s->frequencies = eina_list_append(s->frequencies,
                                                    (void *)(long)atoi(freq));
               }
             freq = strtok(NULL, " ");
          }
        while (freq);

        s->frequencies = eina_list_sort(s->frequencies,
                                        eina_list_count(s->frequencies),
                                        _cpufreq_cb_sort);
     }
   else
     do
       {
#define CPUFREQ_SYSFSDIR "/sys/devices/system/cpu/cpu0/cpufreq"
          f = fopen(CPUFREQ_SYSFSDIR "/scaling_cur_freq", "r");
          if (!f) break;
          fclose(f);

          f = fopen(CPUFREQ_SYSFSDIR "/scaling_driver", "r");
          if (!f) break;
          if (fgets(buf, sizeof(buf), f) == NULL)
            {
               fclose(f);
               break;
            }
          fclose(f);
          if (strcmp(buf, "intel_pstate\n")) break;

          if (s->frequencies)
            {
               eina_list_free(s->frequencies);
               s->frequencies = NULL;
            }
#define CPUFREQ_ADDF(filename) \
          f = fopen(CPUFREQ_SYSFSDIR filename, "r"); \
          if (f) \
            { \
               if (fgets(buf, sizeof(buf), f) != NULL) \
                 s->frequencies = eina_list_append(s->frequencies, \
                                                   (void *)(long)(atoi(buf))); \
               fclose(f); \
            }
          CPUFREQ_ADDF("/cpuinfo_min_freq");
          CPUFREQ_ADDF("/cpuinfo_max_freq");
       }
     while (0);

   f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors", "r");
   if (f)
     {
        char *gov;
        int len;
        
        if (s->governors)
          {
             for (l = s->governors; l; l = l->next)
               free(l->data);
             eina_list_free(s->governors);
             s->governors = NULL;
          }

        if (fgets(buf, sizeof(buf), f) == NULL)
          {
             fclose(f);
             return;
          }
        fclose(f);
        len = strlen(buf);
        if (len > 0)
          {
             gov = buf + len - 1;
             while ((gov > buf) && (isspace(*gov)))
               {
                  *gov = 0;
                  gov--;
               }
          }
        gov = strtok(buf, " ");
        do
          {
             while ((*gov) && (isspace(*gov)))
               gov++;
             if (strlen(gov) != 0)
               s->governors = eina_list_append(s->governors, strdup(gov));
             gov = strtok(NULL, " ");
          }
        while (gov);

        s->governors =
          eina_list_sort(s->governors, eina_list_count(s->governors),
                         (int (*)(const void *, const void *))strcmp);
     }
#endif
}

static int
_cpufreq_status_check_current(Cpu_Status *s)
{
   int ret = 0;
   int frequency = 0;

#if defined (__OpenBSD__)
   size_t len = sizeof(frequency);
   int percent, mib[] = {CTL_HW, HW_CPUSPEED};
   s->active = 0;

   _cpufreq_status_check_available(s);

   if (sysctl(mib, 2, &frequency, &len, NULL, 0) == 0)
     {
        frequency *= 1000;
        if (frequency != s->cur_frequency) ret = 1;
        s->cur_frequency = frequency;
        s->active = 1;
     }

   mib[1] = HW_SETPERF;

   if (sysctl(mib, 2, &percent, &len, NULL, 0) == 0)
     {
        s->cur_percent = percent;
     }

   s->can_set_frequency = 1;
   s->cur_governor = NULL;

#elif defined (__FreeBSD__) || defined(__DragonFly__)
   size_t len = sizeof(frequency);
   s->active = 0;

   /* frequency is stored in dev.cpu.0.freq */
   if (sysctlbyname("dev.cpu.0.freq", &frequency, &len, NULL, 0) == 0)
     {
        frequency *= 1000;
        if (frequency != s->cur_frequency) ret = 1;
        s->cur_frequency = frequency;
        s->active = 1;
     }

   /* hardcoded for testing */
   s->can_set_frequency = 1;
   s->cur_governor = NULL;

#else
   char buf[4096];
   FILE *f;
   int frequency_min = 0x7fffffff;
   int frequency_max = 0;
   int freqtot = 0;
   int i;

   s->active = 0;

   _cpufreq_status_check_available(s);
   // average out frequencies of all cores
   for (i = 0; i < 64; i++)
     {
        snprintf(buf, sizeof(buf), "/sys/devices/system/cpu/cpu%i/cpufreq/scaling_cur_freq", i);
        f = fopen(buf, "r");
        if (f)
          {
             if (fgets(buf, sizeof(buf), f) == NULL)
               {
                  fclose(f);
                  continue;
               }
             fclose(f);

             frequency = atoi(buf);
             if (frequency > frequency_max) frequency_max = frequency;
             if (frequency < frequency_min) frequency_min = frequency;
             freqtot += frequency;
             s->active = 1;
          }
        else
          break;
     }
   if (i < 1) i = 1;
   frequency = freqtot / i;
   if (frequency != s->cur_frequency) ret = 1;
   if (frequency_min != s->cur_min_frequency) ret = 1;
   if (frequency_max != s->cur_max_frequency) ret = 1;
   s->cur_frequency = frequency;
   s->cur_min_frequency = frequency_min;
   s->cur_max_frequency = frequency_max;

//  printf("%i | %i %i\n", frequency, frequency_min, frequency_max);

   // FIXME: this assumes all cores are on the same governor
   f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed", "r");
   if (f)
     {
        s->can_set_frequency = 1;
        fclose(f);
     }
   else
     {
        s->can_set_frequency = 0;
     }

   f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "r");
   if (f)
     {
        char *p;

        if (fgets(buf, sizeof(buf), f) == NULL)
          {
             fclose(f);
             return ret;
          }
        fclose(f);

        for (p = buf; (*p != 0) && (isalnum(*p)); p++) ;
        *p = 0;

        if ((!s->cur_governor) || (strcmp(buf, s->cur_governor)))
          {
             ret = 1;

             free(s->cur_governor);
             s->cur_governor = strdup(buf);

             for (i = strlen(s->cur_governor) - 1; i >= 0; i--)
               {
                  if (isspace(s->cur_governor[i]))
                    s->cur_governor[i] = 0;
                  else
                    break;
               }
          }
     }
   f = fopen("/sys/devices/system/cpu/intel_pstate/min_perf_pct", "r");
   if (f)
     {
        if (fgets(buf, sizeof(buf), f) != NULL)
          {
             s->pstate_min = atoi(buf);
             s->pstate = 1;
          }
        fclose(f);
     }
   f = fopen("/sys/devices/system/cpu/intel_pstate/max_perf_pct", "r");
   if (f)
     {
        if (fgets(buf, sizeof(buf), f) != NULL)
          {
             s->pstate_max = atoi(buf);
             s->pstate = 1;
          }
        fclose(f);
     }
   f = fopen("/sys/devices/system/cpu/intel_pstate/no_turbo", "r");
   if (f)
     {
        if (fgets(buf, sizeof(buf), f) != NULL)
          {
             s->pstate_turbo = atoi(buf);
             if (s->pstate_turbo) s->pstate_turbo = 0;
             else s->pstate_turbo = 1;
             s->pstate = 1;
          }
        fclose(f);
     }
#endif
   return ret;
}

static void
_cpufreq_face_update_available(Instance *inst)
{
   Edje_Message_Int_Set *frequency_msg;
   Edje_Message_String_Set *governor_msg;
   Eina_List *l;
   int i;
   unsigned int count;

   if (cpufreq_config->status->frequencies)
     {
        count = eina_list_count(cpufreq_config->status->frequencies);
        frequency_msg = malloc(sizeof(Edje_Message_Int_Set) + (count - 1) * sizeof(int));
        EINA_SAFETY_ON_NULL_RETURN(frequency_msg);
        frequency_msg->count = count;
        for (l = cpufreq_config->status->frequencies, i = 0; l; l = l->next, i++)
          frequency_msg->val[i] = (long)l->data;
        edje_object_message_send(inst->o_cpu, EDJE_MESSAGE_INT_SET, 1, frequency_msg);
        free(frequency_msg);
     }

   if (cpufreq_config->status->governors)
     {
        count = eina_list_count(cpufreq_config->status->governors);
        governor_msg = malloc(sizeof(Edje_Message_String_Set) + (count - 1) * sizeof(char *));
        governor_msg->count = count;
        for (l = cpufreq_config->status->governors, i = 0; l; l = l->next, i++)
          governor_msg->str[i] = (char *)l->data;
        edje_object_message_send(inst->o_cpu, EDJE_MESSAGE_STRING_SET, 2, governor_msg);
        free(governor_msg);
     }
#if defined(__OpenBSD__)
   _cpufreq_face_update_current(inst);
#endif

}

static void
_cpufreq_face_update_current(Instance *inst)
{
   Edje_Message_Int_Set *frequency_msg;
   Edje_Message_String governor_msg;

   frequency_msg = malloc(sizeof(Edje_Message_Int_Set) + (sizeof(int) * 4));
   EINA_SAFETY_ON_NULL_RETURN(frequency_msg);
   frequency_msg->count = 5;
   frequency_msg->val[0] = cpufreq_config->status->cur_frequency;
   frequency_msg->val[1] = cpufreq_config->status->can_set_frequency;
   frequency_msg->val[2] = cpufreq_config->status->cur_min_frequency;
   frequency_msg->val[3] = cpufreq_config->status->cur_max_frequency;
   frequency_msg->val[4] = 0; // pad
   edje_object_message_send(inst->o_cpu, EDJE_MESSAGE_INT_SET, 3,
                            frequency_msg);
   free(frequency_msg);

   /* BSD crashes here without the if-condition
    * since it has no governors (yet) */
   if (cpufreq_config->status->cur_governor)
     {
        governor_msg.str = cpufreq_config->status->cur_governor;
        edje_object_message_send(inst->o_cpu, EDJE_MESSAGE_STRING, 4,
                                 &governor_msg);
     }
}

static void
_cpufreq_face_cb_set_frequency(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *emission, const char *src __UNUSED__)
{
   Eina_List *l;
   int next_frequency = 0;

   for (l = cpufreq_config->status->frequencies; l; l = l->next)
     {
        if (cpufreq_config->status->cur_frequency == (long)l->data)
          {
             if (!strcmp(emission, "e,action,frequency,increase"))
               {
                  if (l->next) next_frequency = (long)l->next->data;
                  break;
               }
             else if (!strcmp(emission, "e,action,frequency,decrease"))
               {
                  if (l->prev) next_frequency = (long)l->prev->data;
                  break;
               }
             else
               break;
          }
     }
   if (next_frequency != 0) _cpufreq_set_frequency(next_frequency);
}

static void
_cpufreq_face_cb_set_governor(void *data __UNUSED__, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *src __UNUSED__)
{
   Eina_List *l;
   char *next_governor = NULL;

   for (l = cpufreq_config->status->governors; l; l = l->next)
     {
        if (!strcmp(l->data, cpufreq_config->status->cur_governor))
          {
             if (l->next)
               next_governor = l->next->data;
             else
               next_governor = cpufreq_config->status->governors->data;
             break;
          }
     }
   if (next_governor) _cpufreq_set_governor(next_governor);
}

static Eina_Bool
_cpufreq_event_cb_powersave(void *data __UNUSED__, int type, void *event)
{
   E_Event_Powersave_Update *ev;
   Eina_List *l;
   Eina_Bool has_powersave = EINA_FALSE;
   Eina_Bool has_conservative = EINA_FALSE;

   if (type != E_EVENT_POWERSAVE_UPDATE) return ECORE_CALLBACK_PASS_ON;
   if (!cpufreq_config->auto_powersave) return ECORE_CALLBACK_PASS_ON;

   ev = event;
   if (!cpufreq_config->status->orig_governor)
     cpufreq_config->status->orig_governor = eina_stringshare_add(cpufreq_config->status->cur_governor);

   for (l = cpufreq_config->status->governors; l; l = l->next)
     {
        if (!strcmp(l->data, "conservative"))
          has_conservative = EINA_TRUE;
        else if (!strcmp(l->data, "powersave"))
          has_powersave = EINA_TRUE;
        else if (!strcmp(l->data, "interactive"))
          has_powersave = EINA_TRUE;
     }

   switch (ev->mode)
     {
      case E_POWERSAVE_MODE_NONE:
      case E_POWERSAVE_MODE_LOW:
        _cpufreq_set_governor(cpufreq_config->status->orig_governor);
        eina_stringshare_del(cpufreq_config->status->orig_governor);
        cpufreq_config->status->orig_governor = NULL;
        break;

      case E_POWERSAVE_MODE_MEDIUM:
      case E_POWERSAVE_MODE_HIGH:
        if ((cpufreq_config->powersave_governor) || (has_conservative))
          {
             if (cpufreq_config->powersave_governor)
               _cpufreq_set_governor(cpufreq_config->powersave_governor);
             else
               _cpufreq_set_governor("conservative");
             break;
          }
        EINA_FALLTHROUGH;
        /* no break */

      case E_POWERSAVE_MODE_EXTREME:
      case E_POWERSAVE_MODE_FREEZE:
        if (has_powersave)
          _cpufreq_set_governor("powersave");
        break;

      default:
        break;
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void
_cpufreq_menu_fast(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   cpufreq_config->poll_interval = 4;
   _cpufreq_poll_interval_update();
}

static void
_cpufreq_menu_medium(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   cpufreq_config->poll_interval = 8;
   _cpufreq_poll_interval_update();
}

static void
_cpufreq_menu_normal(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   cpufreq_config->poll_interval = 32;
   _cpufreq_poll_interval_update();
}

static void
_cpufreq_menu_slow(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   cpufreq_config->poll_interval = 64;
   _cpufreq_poll_interval_update();
}

static void
_cpufreq_menu_very_slow(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   cpufreq_config->poll_interval = 256;
   _cpufreq_poll_interval_update();
}

static void
_cpufreq_menu_restore_governor(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi)
{
   cpufreq_config->restore_governor = e_menu_item_toggle_get(mi);
   if ((!cpufreq_config->governor) ||
       (strcmp(cpufreq_config->status->cur_governor, cpufreq_config->governor)))
     {
        eina_stringshare_replace(&cpufreq_config->governor,
                                 cpufreq_config->status->cur_governor);
     }
   e_config_save_queue();
}

static void
_cpufreq_menu_auto_powersave(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi)
{
   cpufreq_config->auto_powersave = e_menu_item_toggle_get(mi);
   e_config_save_queue();
}

static void
_cpufreq_menu_governor(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   const char *governor;

   governor = data;
   if (governor)
     {
        _cpufreq_set_governor(governor);
        eina_stringshare_replace(&cpufreq_config->governor, governor);
     }
   e_config_save_queue();
}

static void
_cpufreq_menu_powersave_governor(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   const char *governor;

   governor = data;
   if (governor)
     eina_stringshare_replace(&cpufreq_config->powersave_governor, governor);
   e_config_save_queue();
}

static void
_cpufreq_menu_frequency(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   int frequency;

   frequency = (long)data;
   if (frequency > 0) _cpufreq_set_frequency(frequency);
}

static void
_cpufreq_menu_pstate_min(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   int min = (long)data;
   cpufreq_config->pstate_min = min + 1;
   if (cpufreq_config->pstate_max < cpufreq_config->pstate_min)
     cpufreq_config->pstate_max = cpufreq_config->pstate_min;
   _cpufreq_set_pstate(cpufreq_config->pstate_min - 1,
                       cpufreq_config->pstate_max - 1);
   e_config_save_queue();
}

static void
_cpufreq_menu_pstate_max(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   int max = (long)data;
   cpufreq_config->pstate_max = max + 1;
   if (cpufreq_config->pstate_min > cpufreq_config->pstate_max)
     cpufreq_config->pstate_min = cpufreq_config->pstate_max;
   _cpufreq_set_pstate(cpufreq_config->pstate_min - 1, 
                       cpufreq_config->pstate_max - 1);
   e_config_save_queue();
}

typedef struct _Thread_Config Thread_Config;

struct _Thread_Config
{
   int interval;
   E_Powersave_Sleeper *sleeper;
};

static void
_cpufreq_cb_frequency_check_done(void *data, Ecore_Thread *th __UNUSED__)
{
   Thread_Config *thc = data;
   e_powersave_sleeper_free(thc->sleeper);
   free(thc);
}

static void
_cpufreq_cb_frequency_check_main(void *data, Ecore_Thread *th)
{
   Thread_Config *thc = data;
   for (;;)
     {
        Cpu_Status *status;

        if (ecore_thread_check(th)) break;
        status = _cpufreq_status_new();
        if (_cpufreq_status_check_current(status))
          ecore_thread_feedback(th, status);
        else
          _cpufreq_status_free(status);
        if (ecore_thread_check(th)) break;
        e_powersave_sleeper_sleep(thc->sleeper, thc->interval);
     }
}

static void
_cpufreq_cb_frequency_check_notify(void *data __UNUSED__,
                                   Ecore_Thread *th __UNUSED__,
                                   void *msg)
{
   Cpu_Status *status = msg;
   Instance *inst;
   Eina_List *l;
   int active;
   static Eina_Bool init_set = EINA_FALSE;
   Eina_Bool freq_changed = EINA_FALSE;

   if (!cpufreq_config)
     {
        _cpufreq_status_free(status);
        return;
     }
   active = cpufreq_config->status->active;
   if ((cpufreq_config->status) &&
       (
#ifdef __OpenBSD__
        (status->cur_percent       != cpufreq_config->status->cur_percent      ) ||
#endif
        (status->cur_frequency     != cpufreq_config->status->cur_frequency    ) ||
        (status->cur_min_frequency != cpufreq_config->status->cur_min_frequency) ||
        (status->cur_max_frequency != cpufreq_config->status->cur_max_frequency) ||
        (status->can_set_frequency != cpufreq_config->status->can_set_frequency)))
     freq_changed = EINA_TRUE;
   _cpufreq_status_free(cpufreq_config->status);
   cpufreq_config->status = status;
   if (freq_changed)
     {
        for (l = cpufreq_config->instances; l; l = l->next)
          {
             inst = l->data;
             _cpufreq_face_update_current(inst);
          }
     }
   if (active != cpufreq_config->status->active)
     {
        for (l = cpufreq_config->instances; l; l = l->next)
          {
             inst = l->data;
             if (cpufreq_config->status->active == 0)
               edje_object_signal_emit(inst->o_cpu, "e,state,disabled", "e");
             else if (cpufreq_config->status->active == 1)
               edje_object_signal_emit(inst->o_cpu, "e,state,enabled", "e");
          }
     }
   if (!init_set)
     {
        _cpufreq_set_pstate(cpufreq_config->pstate_min - 1,
                            cpufreq_config->pstate_max - 1);
        init_set = EINA_TRUE;
     }
}

void
_cpufreq_poll_interval_update(void)
{
   Thread_Config *thc;

   if (cpufreq_config->frequency_check_thread)
     {
        ecore_thread_cancel(cpufreq_config->frequency_check_thread);
        cpufreq_config->frequency_check_thread = NULL;
     }
   thc = malloc(sizeof(Thread_Config));
   if (thc)
     {
        thc->interval = cpufreq_config->poll_interval;
        thc->sleeper = e_powersave_sleeper_new();
        cpufreq_config->frequency_check_thread =
          ecore_thread_feedback_run(_cpufreq_cb_frequency_check_main,
                                    _cpufreq_cb_frequency_check_notify,
                                    _cpufreq_cb_frequency_check_done,
                                    _cpufreq_cb_frequency_check_done,
                                    thc, EINA_TRUE);
     }
   e_config_save_queue();
}

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION, "Cpufreq"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   struct stat st;
   char buf[PATH_MAX];
   Eina_List *l;

   conf_edd = E_CONFIG_DD_NEW("Cpufreq_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, config_version, INT);
   E_CONFIG_VAL(D, T, poll_interval, INT);
   E_CONFIG_VAL(D, T, restore_governor, INT);
   E_CONFIG_VAL(D, T, auto_powersave, INT);
   E_CONFIG_VAL(D, T, powersave_governor, STR);
   E_CONFIG_VAL(D, T, governor, STR);
   E_CONFIG_VAL(D, T, pstate_min, INT);
   E_CONFIG_VAL(D, T, pstate_max, INT);

   cpufreq_config = e_config_domain_load("module.cpufreq", conf_edd);
   if ((cpufreq_config) &&
       (cpufreq_config->config_version != CPUFREQ_CONFIG_VERSION))
     E_FREE(cpufreq_config);

   if (!cpufreq_config)
     {
        cpufreq_config = E_NEW(Config, 1);
        cpufreq_config->config_version = CPUFREQ_CONFIG_VERSION;
        cpufreq_config->poll_interval = 32;
        cpufreq_config->restore_governor = 0;
        cpufreq_config->auto_powersave = 1;
        cpufreq_config->powersave_governor = NULL;
        cpufreq_config->governor = NULL;
        cpufreq_config->pstate_min = 1;
        cpufreq_config->pstate_max = 101;
     }
   else
     {
        if (cpufreq_config->pstate_min == 0) cpufreq_config->pstate_min = 1;
        if (cpufreq_config->pstate_max == 0) cpufreq_config->pstate_max = 101;
     }
   E_CONFIG_LIMIT(cpufreq_config->poll_interval, 1, 1024);

   snprintf(buf, sizeof(buf), "%s/%s/freqset",
            e_module_dir_get(m), MODULE_ARCH);
   cpufreq_config->set_exe_path = strdup(buf);
   
   if (stat(buf, &st) < 0)
     {
        e_util_dialog_show(_("Cpufreq Error"),
                           _("The freqset binary in the cpufreq module<ps/>"
                             "directory cannot be found (stat failed)"));
     }
   else if ((st.st_uid != 0) ||
            ((st.st_mode & (S_ISUID)) != (S_ISUID)) ||
            ((st.st_mode & (S_IXOTH)) != (S_IXOTH)))
     {
        e_util_dialog_show(_("Cpufreq Permissions Error"),
                           _("The freqset binary in the cpufreq module<ps/>"
                             "is not owned by root or does not have the<ps/>"
                             "setuid bit set. Please ensure this is the<ps/>"
                             "case. For example:<ps/>"
                             "<ps/>"
                             "sudo chown root %s<ps/>"
                             "sudo chmod u+s,a+x %s<ps/>"),
                           buf, buf);
     }
   cpufreq_config->status = _cpufreq_status_new();

   _cpufreq_status_check_available(cpufreq_config->status);
   _cpufreq_poll_interval_update();

   if ((cpufreq_config->restore_governor) && (cpufreq_config->governor))
     {
        /* If the governor is available, restore it */
        for (l = cpufreq_config->status->governors; l; l = l->next)
          {
             if (!strcmp(l->data, cpufreq_config->governor))
               {
                  _cpufreq_set_governor(cpufreq_config->governor);
                  break;
               }
          }
     }

   cpufreq_config->module = m;

   e_gadcon_provider_register(&_gadcon_class);
   snprintf(buf, sizeof(buf), "%s/e-module-cpufreq.edj", e_module_dir_get(m));
   e_configure_registry_category_add("advanced", 80, _("Advanced"), NULL,
                                     "preferences-advanced");
   e_configure_registry_item_add("advanced/cpufreq", 120, _("CPU Frequency"),
                                 NULL, buf, e_int_config_cpufreq_module);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   e_configure_registry_item_del("advanced/cpufreq");
   e_configure_registry_category_del("advanced");
   
   e_gadcon_provider_unregister(&_gadcon_class);

   if (cpufreq_config->frequency_check_thread)
     {
        ecore_thread_cancel(cpufreq_config->frequency_check_thread);
        cpufreq_config->frequency_check_thread = NULL;
     }
   if (cpufreq_config->menu)
     {
        e_menu_post_deactivate_callback_set(cpufreq_config->menu, NULL, NULL);
        e_object_del(E_OBJECT(cpufreq_config->menu));
        cpufreq_config->menu = NULL;
     }
   if (cpufreq_config->menu_poll)
     {
        e_menu_post_deactivate_callback_set(cpufreq_config->menu_poll, NULL, NULL);
        e_object_del(E_OBJECT(cpufreq_config->menu_poll));
        cpufreq_config->menu_poll = NULL;
     }
   if (cpufreq_config->menu_governor)
     {
        e_menu_post_deactivate_callback_set(cpufreq_config->menu_governor, NULL, NULL);
        e_object_del(E_OBJECT(cpufreq_config->menu_governor));
        cpufreq_config->menu_governor = NULL;
     }
   if (cpufreq_config->menu_frequency)
     {
        e_menu_post_deactivate_callback_set(cpufreq_config->menu_frequency, NULL, NULL);
        e_object_del(E_OBJECT(cpufreq_config->menu_frequency));
        cpufreq_config->menu_frequency = NULL;
     }
   if (cpufreq_config->menu_powersave)
     {
        e_menu_post_deactivate_callback_set(cpufreq_config->menu_powersave, NULL, NULL);
        e_object_del(E_OBJECT(cpufreq_config->menu_powersave));
        cpufreq_config->menu_powersave = NULL;
     }
   if (cpufreq_config->governor)
     eina_stringshare_del(cpufreq_config->governor);
   if (cpufreq_config->status) _cpufreq_status_free(cpufreq_config->status);
   E_FREE(cpufreq_config->set_exe_path);
   
   if (cpufreq_config->config_dialog)
     e_object_del(E_OBJECT(cpufreq_config->config_dialog));
   
   free(cpufreq_config);
   cpufreq_config = NULL;
   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   e_config_domain_save("module.cpufreq", conf_edd, cpufreq_config);
   return 1;
}
