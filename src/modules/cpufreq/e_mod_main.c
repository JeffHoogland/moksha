/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

#ifdef __FreeBSD__   
#include <sys/types.h>
#include <sys/sysctl.h>
#endif  

/***************************************************************************/
/**/
/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *client_class);
/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
     "cpufreq",
     {
        _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL
     },
   E_GADCON_CLIENT_STYLE_PLAIN
};
/**/
/***************************************************************************/

/***************************************************************************/
/**/
/* actual module specifics */

typedef struct _Instance Instance;

struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_cpu;
};

static void _button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _menu_cb_post(void *data, E_Menu *m);
static void _cpufreq_set_governor(const char *governor);
static void _cpufreq_set_frequency(int frequency);
static int _cpufreq_cb_check(void *data);
static Status *_cpufreq_status_new();
static void _cpufreq_status_free(Status *s);
static int _cpufreq_status_check_available(Status *s);
static int _cpufreq_status_check_current(Status *s);
static int _cpufreq_cb_sort(const void *item1, const void *item2);
static void _cpufreq_face_update_available(Instance *inst);
static void _cpufreq_face_update_current(Instance *inst);
static void _cpufreq_face_cb_set_frequency(void *data, Evas_Object *o, const char *emission, const char *source);
static void _cpufreq_face_cb_set_governor(void *data, Evas_Object *o, const char *emission, const char *source);
static int _cpufreq_event_cb_powersave(void *data __UNUSED__, int type, void *event);

static void _cpufreq_menu_fast(void *data, E_Menu *m, E_Menu_Item *mi);
static void _cpufreq_menu_medium(void *data, E_Menu *m, E_Menu_Item *mi);
static void _cpufreq_menu_normal(void *data, E_Menu *m, E_Menu_Item *mi);
static void _cpufreq_menu_slow(void *data, E_Menu *m, E_Menu_Item *mi);
static void _cpufreq_menu_very_slow(void *data, E_Menu *m, E_Menu_Item *mi);
static void _cpufreq_menu_restore_governor(void *data, E_Menu *m, E_Menu_Item *mi);
static void _cpufreq_menu_auto_powersave(void *data, E_Menu *m, E_Menu_Item *mi);
static void _cpufreq_menu_governor(void *data, E_Menu *m, E_Menu_Item *mi);
static void _cpufreq_menu_powersave_governor(void *data, E_Menu *m, E_Menu_Item *mi);
static void _cpufreq_menu_frequency(void *data, E_Menu *m, E_Menu_Item *mi);

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
   edje_object_signal_callback_add(o, "e,action,governor,next", "*", _cpufreq_face_cb_set_governor, NULL);
   edje_object_signal_callback_add(o, "e,action,frequency,increase", "*", _cpufreq_face_cb_set_frequency, NULL);
   edje_object_signal_callback_add(o, "e,action,frequency,decrease", "*", _cpufreq_face_cb_set_frequency, NULL);
   
   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;
   
   inst->gcc = gcc;
   inst->o_cpu = o;
   
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
				  _button_cb_mouse_down, inst);
   cpufreq_config->instances = eina_list_append(cpufreq_config->instances, inst);
   if (cpufreq_config->status) _cpufreq_status_free(cpufreq_config->status);
   cpufreq_config->status = _cpufreq_status_new();
   _cpufreq_cb_check(NULL);
   _cpufreq_face_update_available(inst);

   cpufreq_config->handler = ecore_event_handler_add(E_EVENT_POWERSAVE_UPDATE,
	_cpufreq_event_cb_powersave, NULL);
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;
   
   inst = gcc->data;
   cpufreq_config->instances = eina_list_remove(cpufreq_config->instances, inst);
   evas_object_del(inst->o_cpu);
   free(inst);

   if (cpufreq_config->handler) ecore_event_handler_del(cpufreq_config->handler);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   Instance *inst;
   
   inst = gcc->data;
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static char *
_gc_label(E_Gadcon_Client_Class *client_class)
{
   return _("Cpufreq");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas)
{
   Evas_Object *o;
   char buf[4096];
   
   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-cpufreq.edj",
	    e_module_dir_get(cpufreq_config->module));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class)
{
   return _gadcon_class.name;
}

/**/
/***************************************************************************/

/***************************************************************************/
/**/
static void
_button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Instance *inst;
   Evas_Event_Mouse_Down *ev;
   
   inst = data;
   ev = event_info;
   if ((ev->button == 3) && (!cpufreq_config->menu))
     {
	E_Menu *mn;
	E_Menu_Item *mi;
	int cx, cy;
	Eina_List *l;
	char buf[256];
	
	mn = e_menu_new();
	cpufreq_config->menu_poll = mn;
	
	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Fast (4 ticks)"));
	e_menu_item_radio_set(mi, 1);
	e_menu_item_radio_group_set(mi, 1);
	if (cpufreq_config->poll_interval <= 4) e_menu_item_toggle_set(mi, 1);
	e_menu_item_callback_set(mi, _cpufreq_menu_fast, NULL);

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Medium (8 ticks)"));
	e_menu_item_radio_set(mi, 1);
	e_menu_item_radio_group_set(mi, 1);
	if (cpufreq_config->poll_interval > 4) e_menu_item_toggle_set(mi, 1);
	e_menu_item_callback_set(mi, _cpufreq_menu_medium, NULL);

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Normal (32 ticks)"));
	e_menu_item_radio_set(mi, 1);
	e_menu_item_radio_group_set(mi, 1);
	if (cpufreq_config->poll_interval >= 32) e_menu_item_toggle_set(mi, 1);
	e_menu_item_callback_set(mi, _cpufreq_menu_normal, NULL);

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Slow (64 ticks)"));
	e_menu_item_radio_set(mi, 1);
	e_menu_item_radio_group_set(mi, 1);
	if (cpufreq_config->poll_interval >= 64) e_menu_item_toggle_set(mi, 1);
	e_menu_item_callback_set(mi, _cpufreq_menu_slow, NULL);

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Very Slow (256 ticks)"));
	e_menu_item_radio_set(mi, 1);
	e_menu_item_radio_group_set(mi, 1);
	if (cpufreq_config->poll_interval >= 128) e_menu_item_toggle_set(mi, 1);
	e_menu_item_callback_set(mi, _cpufreq_menu_very_slow, NULL);

	if (cpufreq_config->status->governors)
	  {
	     mn = e_menu_new();
	     cpufreq_config->menu_governor = mn;
	     
	     for (l = cpufreq_config->status->governors; l; l = l->next)
	       {
		  mi = e_menu_item_new(mn);
		  if (!strcmp(l->data, "userspace"))
		    e_menu_item_label_set(mi, _("Manual"));
		  else if (!strcmp(l->data, "ondemand"))
		    e_menu_item_label_set(mi, _("Automatic"));
		  else if (!strcmp(l->data, "conservative"))
		    e_menu_item_label_set(mi, _("Lower Power Automatic"));
		  else if (!strcmp(l->data, "powersave"))
		    e_menu_item_label_set(mi, _("Minimum Speed"));
		  else if (!strcmp(l->data, "performance"))
		    e_menu_item_label_set(mi, _("Maximum Speed"));
		  e_menu_item_radio_set(mi, 1);
		  e_menu_item_radio_group_set(mi, 1);
		  if (!strcmp(cpufreq_config->status->cur_governor, l->data))
		    e_menu_item_toggle_set(mi, 1);
		  e_menu_item_callback_set(mi, _cpufreq_menu_governor, l->data);
	       }

	     e_menu_item_separator_set(e_menu_item_new(mn), 1);

	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("Restore CPU Power Policy"));
	     e_menu_item_check_set(mi, 1);
	     e_menu_item_toggle_set(mi, cpufreq_config->restore_governor);
	     e_menu_item_callback_set(mi, _cpufreq_menu_restore_governor, NULL);

	     mn = e_menu_new();
	     cpufreq_config->menu_powersave = mn;

	     e_menu_title_set(mn, _("Powersaving policy"));

	     for (l = cpufreq_config->status->governors; l; l = l->next)
	       {
		  if (!strcmp(l->data, "userspace"))
		    continue;

		  mi = e_menu_item_new(mn);
		  if (!strcmp(l->data, "ondemand"))
		    e_menu_item_label_set(mi, _("Automatic"));
		  else if (!strcmp(l->data, "conservative"))
		    e_menu_item_label_set(mi, _("Lower Power Automatic"));
		  else if (!strcmp(l->data, "powersave"))
		    e_menu_item_label_set(mi, _("Minimum Speed"));
		  else if (!strcmp(l->data, "performance"))
		    e_menu_item_label_set(mi, _("Maximum Speed"));

		  e_menu_item_radio_set(mi, 1);
		  e_menu_item_radio_group_set(mi, 1);
		  if (cpufreq_config->powersave_governor
		       	&& !strcmp(cpufreq_config->powersave_governor, l->data))
		    e_menu_item_toggle_set(mi, 1);
		  e_menu_item_callback_set(mi, _cpufreq_menu_powersave_governor, l->data);
	       }

	     e_menu_item_separator_set(e_menu_item_new(mn), 1);

	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("Automatic powersaving"));
	     e_menu_item_check_set(mi, 1);
	     e_menu_item_toggle_set(mi, cpufreq_config->auto_powersave);
	     e_menu_item_callback_set(mi, _cpufreq_menu_auto_powersave, NULL);
	  }

	if ((cpufreq_config->status->frequencies) &&
	    (cpufreq_config->status->can_set_frequency))
	  {
	     mn = e_menu_new();
	     cpufreq_config->menu_frequency = mn;
	
	     for (l = cpufreq_config->status->frequencies; l; l = l->next)
	       {
		  int frequency;
		  
		  frequency = (int)l->data;
		  mi = e_menu_item_new(mn);
		  if (frequency < 1000000)
		    snprintf(buf, sizeof(buf), _("%i MHz"), frequency / 1000);
		  else
		    snprintf(buf, sizeof(buf), _("%i.%i GHz"), 
			     frequency / 1000000, (frequency % 1000000) / 100000);
		  buf[sizeof(buf) - 1] = 0;
		  e_menu_item_label_set(mi, buf);
		  e_menu_item_radio_set(mi, 1);
		  e_menu_item_radio_group_set(mi, 1);
		  if (cpufreq_config->status->cur_frequency == frequency)
		    e_menu_item_toggle_set(mi, 1);
		  e_menu_item_callback_set(mi, _cpufreq_menu_frequency, l->data);
	       }
	  }
	
	mn = e_menu_new();
	cpufreq_config->menu = mn;
	e_menu_post_deactivate_callback_set(mn, _menu_cb_post, inst);
	
	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Time Between Updates"));
	e_menu_item_submenu_set(mi, cpufreq_config->menu_poll);

	if (cpufreq_config->menu_governor)
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("Set CPU Power Policy"));
	     e_menu_item_submenu_set(mi, cpufreq_config->menu_governor);
	  }
	
	if (cpufreq_config->menu_frequency)
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("Set CPU Speed"));
	     e_menu_item_submenu_set(mi, cpufreq_config->menu_frequency);
	  }
	if (cpufreq_config->menu_powersave)
	  {
	     mi = e_menu_item_new(mn);
	     e_menu_item_label_set(mi, _("Powersaving behavior"));
	     e_menu_item_submenu_set(mi, cpufreq_config->menu_powersave);
	  }
	
        e_gadcon_client_util_menu_items_append(inst->gcc, mn, 0);
	
	e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon,
					  &cx, &cy, NULL, NULL);
	e_menu_activate_mouse(mn,
			      e_util_zone_current_get(e_manager_current_get()),
			      cx + ev->output.x, cy + ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
     }

}

static void
_menu_cb_post(void *data, E_Menu *m)
{
   if (!cpufreq_config->menu) return;
   e_object_del(E_OBJECT(cpufreq_config->menu));
   cpufreq_config->menu = NULL;
   if (cpufreq_config->menu_poll) e_object_del(E_OBJECT(cpufreq_config->menu_poll));
   cpufreq_config->menu_poll = NULL;
   if (cpufreq_config->menu_governor) e_object_del(E_OBJECT(cpufreq_config->menu_governor));
   cpufreq_config->menu_governor = NULL;
   if (cpufreq_config->menu_frequency) e_object_del(E_OBJECT(cpufreq_config->menu_frequency));
   cpufreq_config->menu_frequency = NULL;
   if (cpufreq_config->menu_powersave) e_object_del(E_OBJECT(cpufreq_config->menu_powersave));
   cpufreq_config->menu_powersave = NULL;
}

static void
_cpufreq_set_governor(const char *governor)
{
   char buf[4096];
   int ret;

   snprintf(buf, sizeof(buf), 
	    "%s %s %s", cpufreq_config->set_exe_path, "governor", governor);
   ret = system(buf);
   if (ret != 0)
     {
	E_Dialog *dia;

	dia = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_e_mod_cpufreq_error_setfreq");
	if (!dia) return;
	e_dialog_title_set(dia, "Enlightenment Cpufreq Module");
	e_dialog_icon_set(dia, "enlightenment/e", 64);
	e_dialog_text_set(dia, _("There was an error trying to set the<br>"
				 "cpu frequency governor via the module's<br>"
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
   
#ifdef __FreeBSD__
   frequency /= 1000;
#endif 
   if (!cpufreq_config->status->can_set_frequency)
     {
	E_Dialog *dia;

	dia = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_e_mod_cpufreq_error_setfreq");
	if (!dia) return;
	e_dialog_title_set(dia, "Enlightenment Cpufreq Module");
	e_dialog_icon_set(dia, "enlightenment/e", 64);
	e_dialog_text_set(dia, _("Your kernel does not support setting the<br>"
				 "CPU frequency at all. You may be missing<br>"
				 "Kernel modules or features, or your CPU<br>"
				 "simply does not support this feature."));
	e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
	e_win_centered_set(dia->win, 1);
	e_dialog_show(dia);
	return;
     }
   snprintf(buf, sizeof(buf),
	    "%s %s %i", cpufreq_config->set_exe_path, "frequency", frequency);
   ret = system(buf);
   if (ret != 0)
     {
	E_Dialog *dia;

	dia = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_e_mod_cpufreq_error_setfreq");
	if (!dia) return;
	e_dialog_title_set(dia, "Enlightenment Cpufreq Module");
	e_dialog_icon_set(dia, "enlightenment/e", 64);
	e_dialog_text_set(dia, _("There was an error trying to set the<br>"
				 "cpu frequency setting via the module's<br>"
				 "setfreq utility."));
	e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
	e_win_centered_set(dia->win, 1);
	e_dialog_show(dia);
     }
}

static int
_cpufreq_cb_check(void *data)
{
   Instance *inst;
   Eina_List *l;
   int active;

   if (cpufreq_config->menu_poll) return 1;
   active = cpufreq_config->status->active;
   if (_cpufreq_status_check_current(cpufreq_config->status))
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

   return 1;
}

static Status *
_cpufreq_status_new()
{
   Status *s;

   s = E_NEW(Status, 1);
   if (!s) return NULL;
   s->active = -1;
   return s;
}

static void
_cpufreq_status_free(Status *s)
{
   Eina_List *l;

   if (s->frequencies) eina_list_free(s->frequencies);
   if (s->governors)
     {
	for (l = s->governors; l; l = l->next) free(l->data);
	eina_list_free(s->governors);
     }
   if (s->cur_governor) free(s->cur_governor);
   if (s->orig_governor) eina_stringshare_del(s->orig_governor);
   free(s);
}

static int
_cpufreq_cb_sort(const void *item1, const void *item2)
{
   int a, b;

   a = (int)item1;
   b = (int)item2;
   if (a < b) return -1;
   else if (a > b) return 1;
   return 0;
}

static int
_cpufreq_status_check_available(Status *s)
{
   char buf[4096];
   Eina_List *l;
#ifdef __FreeBSD__
   int freq, i;
   size_t len = 0;
   char *freqs, *pos, *q;

   /* read freq_levels sysctl and store it in freq */
   len = sizeof(buf);
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
	     s->frequencies = eina_list_append(s->frequencies, (void *)freq);

	     pos = q + 1;
	     pos = strchr(pos, ' ');
	  }
     }
   
   /* sort is not necessary because freq_levels is already sorted */
   /* freebsd doesn't have governors */
   if (s->governors)
     {
	for (l = s->governors; l; l = l->next) free(l->data);
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
	
	fgets(buf, sizeof(buf), f);
	buf[sizeof(buf) - 1] = 0;
	fclose(f);
	
	freq = strtok(buf, " ");
	do 
	  {
	     if (atoi(freq) != 0) 
	       {
		  s->frequencies = eina_list_append(s->frequencies,	
						    (void *) atoi(freq));
	       }
	     freq = strtok(NULL, " ");
	  }
	while (freq != NULL);

	s->frequencies = eina_list_sort(s->frequencies,
					eina_list_count(s->frequencies),
					_cpufreq_cb_sort);
     }

   f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors", "r");
   if (f)
     {
	char *gov;
	
	if (s->governors)
	  {
	     for (l = s->governors; l; l = l->next)
	       free(l->data);
	     eina_list_free(s->governors);
	     s->governors = NULL;
	  }

	fgets(buf, sizeof(buf), f);
	buf[sizeof(buf) - 1] = 0;
	fclose(f);

	gov = strtok(buf, " ");
	do 
	  {
	     while ((*gov) && (isspace(*gov))) gov++;
	     if (strlen(gov) != 0)
	       s->governors = eina_list_append(s->governors, strdup(gov));
	     gov = strtok(NULL, " ");
	  }
	while (gov != NULL);

	s->governors = eina_list_sort(s->governors,
				      eina_list_count(s->governors),
				      (int (*)(const void *, const void *))strcmp);
     }
#endif
   return 1;
}

static int
_cpufreq_status_check_current(Status *s)
{
   char buf[4096];
   int i;
   FILE *f;
   int ret = 0;
   int frequency = 0;
#ifdef __FreeBSD__
   int len = 4;

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
   s->active = 0;
   
   _cpufreq_status_check_available(s);
   f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r");
   if (f)
     {
	fgets(buf, sizeof(buf), f);
	buf[sizeof(buf) - 1] = 0;
	fclose(f);
	
	frequency = atoi(buf);
	if (frequency != s->cur_frequency)
	  ret = 1;
	s->cur_frequency = frequency;
	s->active = 1;
     }
   
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
	fgets(buf, sizeof(buf), f);
	buf[sizeof(buf) - 1] = 0;
	fclose(f);

	if ((s->cur_governor == NULL) || (strcmp(buf, s->cur_governor)))
	  {
	     ret = 1;

	     if (s->cur_governor)
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
   int count;

   count = eina_list_count(cpufreq_config->status->frequencies);
   frequency_msg = malloc(sizeof(Edje_Message_Int_Set) + (count - 1) * sizeof(int));
   frequency_msg->count = count;
   for (l = cpufreq_config->status->frequencies, i = 0; l; l = l->next, i++) 
     frequency_msg->val[i] = (int)l->data;
   edje_object_message_send(inst->o_cpu, EDJE_MESSAGE_INT_SET, 1, frequency_msg);
   free(frequency_msg);

   count = eina_list_count(cpufreq_config->status->governors);
   governor_msg = malloc(sizeof(Edje_Message_String_Set) + (count - 1) * sizeof(char *));
   governor_msg->count = count;
   for (l = cpufreq_config->status->governors, i = 0; l; l = l->next, i++)
     governor_msg->str[i] = (char *)l->data;
   edje_object_message_send(inst->o_cpu, EDJE_MESSAGE_STRING_SET, 2, governor_msg);
   free(governor_msg);
}

static void
_cpufreq_face_update_current(Instance *inst)
{
   Edje_Message_Int_Set *frequency_msg;
   Edje_Message_String governor_msg;

   frequency_msg = malloc(sizeof(Edje_Message_Int_Set) + sizeof(int));
   frequency_msg->count = 2;
   frequency_msg->val[0] = cpufreq_config->status->cur_frequency;
   frequency_msg->val[1] = cpufreq_config->status->can_set_frequency;
   edje_object_message_send(inst->o_cpu, EDJE_MESSAGE_INT_SET, 3,
			    frequency_msg);
   free(frequency_msg);

   /* BSD crashes here without the if-condition
    * since it has no governors (yet) */
   if (cpufreq_config->status->cur_governor != NULL)
     {
	governor_msg.str = cpufreq_config->status->cur_governor;
	edje_object_message_send(inst->o_cpu, EDJE_MESSAGE_STRING, 4, 
				 &governor_msg);
     }
}

static void
_cpufreq_face_cb_set_frequency(void *data, Evas_Object *obj, const char *emission, const char *src)
{
   Eina_List *l;
   int next_frequency = 0;

   for (l = cpufreq_config->status->frequencies; l; l = l->next)
     {
	if (cpufreq_config->status->cur_frequency == (int)l->data)
	  {
	     if (!strcmp(emission, "e,action,frequency,increase"))
	       {
		  if (l->next) next_frequency = (int)l->next->data;
		  break;
	       }
	     else if (!strcmp(emission, "e,action,frequency,decrease"))
	       {
		  if (l->prev) next_frequency = (int)l->prev->data;
		  break;
	       }
	     else
	       break;
	  }
     }
   if (next_frequency != 0) _cpufreq_set_frequency(next_frequency);
}

static void
_cpufreq_face_cb_set_governor(void *data, Evas_Object *obj, const char *emission, const char *src)
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
   if (next_governor != NULL) _cpufreq_set_governor(next_governor);
}

static int
_cpufreq_event_cb_powersave(void *data __UNUSED__, int type, void *event)
{
   E_Event_Powersave_Update *ev;
   Eina_List *l;
   Eina_Bool has_powersave, has_conservative;

   if (type != E_EVENT_POWERSAVE_UPDATE) return 1;
   if (!cpufreq_config->auto_powersave) return 1;

   ev = event;
   if (!cpufreq_config->status->orig_governor)
     cpufreq_config->status->orig_governor = eina_stringshare_add(cpufreq_config->status->cur_governor);

   for (l = cpufreq_config->status->governors; l; l = l->next)
     {
	if (!strcmp(l->data, "conservative"))
	  has_conservative = EINA_TRUE;
	else if (!strcmp(l->data, "powersave"))
	  has_powersave = EINA_TRUE;
     }

   switch(ev->mode)
     {
      case E_POWERSAVE_MODE_NONE:
      case E_POWERSAVE_MODE_LOW:
	 _cpufreq_set_governor(cpufreq_config->status->orig_governor);
	 eina_stringshare_del(cpufreq_config->status->orig_governor);
	 cpufreq_config->status->orig_governor = NULL;
	 break;
      case E_POWERSAVE_MODE_MEDIUM:
      case E_POWERSAVE_MODE_HIGH:
	 if (cpufreq_config->powersave_governor || has_conservative)
	   {
	      if (cpufreq_config->powersave_governor)
		_cpufreq_set_governor(cpufreq_config->powersave_governor);
	      else
		_cpufreq_set_governor("conservative");
	      break;
	   }
      case E_POWERSAVE_MODE_EXTREME:
	 if (has_powersave)
	   _cpufreq_set_governor("powersave");
	 break;
     }

   return 1;
}

static void
_cpufreq_menu_fast(void *data, E_Menu *m, E_Menu_Item *mi)
{
   cpufreq_config->poll_interval = 4;
   ecore_poller_del(cpufreq_config->frequency_check_poller);
   cpufreq_config->frequency_check_poller = 
     ecore_poller_add(ECORE_POLLER_CORE, cpufreq_config->poll_interval,
		      _cpufreq_cb_check, NULL);
   e_config_save_queue();
}

static void
_cpufreq_menu_medium(void *data, E_Menu *m, E_Menu_Item *mi)
{
   cpufreq_config->poll_interval = 8;
   ecore_poller_del(cpufreq_config->frequency_check_poller);
   cpufreq_config->frequency_check_poller = 
     ecore_poller_add(ECORE_POLLER_CORE, cpufreq_config->poll_interval,
		      _cpufreq_cb_check, NULL);
   e_config_save_queue();
}

static void
_cpufreq_menu_normal(void *data, E_Menu *m, E_Menu_Item *mi)
{
   cpufreq_config->poll_interval = 32;
   ecore_poller_del(cpufreq_config->frequency_check_poller);
   cpufreq_config->frequency_check_poller = 
     ecore_poller_add(ECORE_POLLER_CORE, cpufreq_config->poll_interval,
		      _cpufreq_cb_check, NULL);
   e_config_save_queue();
}

static void
_cpufreq_menu_slow(void *data, E_Menu *m, E_Menu_Item *mi)
{
   cpufreq_config->poll_interval = 64;
   ecore_poller_del(cpufreq_config->frequency_check_poller);
   cpufreq_config->frequency_check_poller = 
     ecore_poller_add(ECORE_POLLER_CORE, cpufreq_config->poll_interval,
		      _cpufreq_cb_check, NULL);
   e_config_save_queue();
}

static void
_cpufreq_menu_very_slow(void *data, E_Menu *m, E_Menu_Item *mi)
{
   cpufreq_config->poll_interval = 256;
   ecore_poller_del(cpufreq_config->frequency_check_poller);
   cpufreq_config->frequency_check_poller = 
     ecore_poller_add(ECORE_POLLER_CORE, cpufreq_config->poll_interval,
		      _cpufreq_cb_check, NULL);
   e_config_save_queue();
}

static void
_cpufreq_menu_restore_governor(void *data, E_Menu *m, E_Menu_Item *mi)
{
   cpufreq_config->restore_governor = e_menu_item_toggle_get(mi);
   if ((!cpufreq_config->governor) || 
       (strcmp(cpufreq_config->status->cur_governor, cpufreq_config->governor)))
     {
	if (cpufreq_config->governor) eina_stringshare_del(cpufreq_config->governor);
	cpufreq_config->governor = eina_stringshare_add(cpufreq_config->status->cur_governor);
     }
   e_config_save_queue();
}

static void
_cpufreq_menu_auto_powersave(void *data, E_Menu *m, E_Menu_Item *mi)
{
   cpufreq_config->auto_powersave = e_menu_item_toggle_get(mi);
   e_config_save_queue();
}

static void
_cpufreq_menu_governor(void *data, E_Menu *m, E_Menu_Item *mi)
{
   const char *governor;

   governor = data;
   if (governor)
     {
	_cpufreq_set_governor(governor);
	if (cpufreq_config->governor) eina_stringshare_del(cpufreq_config->governor);
	cpufreq_config->governor = eina_stringshare_add(governor);
     }
   e_config_save_queue();
}

static void
_cpufreq_menu_powersave_governor(void *data, E_Menu *m, E_Menu_Item *mi)
{
   const char *governor;

   governor = data;
   if (governor)
     {
	if (cpufreq_config->powersave_governor)
	  eina_stringshare_del(cpufreq_config->powersave_governor);
	cpufreq_config->powersave_governor = eina_stringshare_add(governor);
     }
   e_config_save_queue();
}

static void
_cpufreq_menu_frequency(void * data, E_Menu *m, E_Menu_Item *mi)
{
   int frequency;
   
   frequency = (int)data;
   if (frequency > 0) _cpufreq_set_frequency(frequency);
}

/***************************************************************************/
/**/
/* module setup */
EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION,
     "Cpufreq"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   char buf[4096];
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
   
   cpufreq_config = e_config_domain_load("module.cpufreq", conf_edd);
   if (cpufreq_config && cpufreq_config->config_version != CPUFREQ_CONFIG_VERSION)
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
     }
   E_CONFIG_LIMIT(cpufreq_config->poll_interval, 1, 1024);
   
   snprintf(buf, sizeof(buf), "%s/%s/freqset",
	    e_module_dir_get(m), MODULE_ARCH);
   cpufreq_config->set_exe_path = strdup(buf);
   cpufreq_config->frequency_check_poller = 
     ecore_poller_add(ECORE_POLLER_CORE,
		     cpufreq_config->poll_interval,
		     _cpufreq_cb_check, NULL);
   cpufreq_config->status = _cpufreq_status_new();

   _cpufreq_status_check_available(cpufreq_config->status);
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
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   e_gadcon_provider_unregister(&_gadcon_class);
   
   if (cpufreq_config->frequency_check_poller)
     ecore_poller_del(cpufreq_config->frequency_check_poller);
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
   free(cpufreq_config);
   cpufreq_config = NULL;
   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   e_config_domain_save("module.cpufreq", conf_edd, cpufreq_config);
   return 1;
}
/**/
/***************************************************************************/
