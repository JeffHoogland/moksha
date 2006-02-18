/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include <Ecore.h>
#include <errno.h>
#include <ctype.h>
#include "e_mod_main.h"
#include "config.h"

#ifdef __FreeBSD__   
#include <sys/types.h>
#include <sys/sysctl.h>
#endif  

/* FIXME: check permissions (can execute) setfreq before trying
 * FIXME: display throttling state ???
 * FIXME: handle multiple cpu's (yes it's likely rare - but possible)
 */

static Cpufreq *_cpufreq_new(E_Module *module);
static void     _cpufreq_free(Cpufreq *cpufreq);
static void     _cpufreq_set_governor(Cpufreq *cpufreq, const char *governor);
static void     _cpufreq_menu_fast(void *data, E_Menu *m, E_Menu_Item *mi);
static void     _cpufreq_menu_medium(void *data, E_Menu *m, E_Menu_Item *mi);
static void     _cpufreq_menu_normal(void *data, E_Menu *m, E_Menu_Item *mi);
static void     _cpufreq_menu_slow(void *data, E_Menu *m, E_Menu_Item *mi);
static void     _cpufreq_menu_very_slow(void *data, E_Menu *m, E_Menu_Item *mi);
static void	_cpufreq_menu_cb_allow_overlap(void *data, E_Menu *m, E_Menu_Item *mi);
static void     _cpufreq_menu_restore_governor(void *data, E_Menu *m, E_Menu_Item *mi);
static void     _cpufreq_menu_governor(void *data, E_Menu *m, E_Menu_Item *mi);
static void     _cpufreq_menu_frequency(void *data, E_Menu *m, E_Menu_Item *mi);
static void     _cpufreq_config_menu_new(Cpufreq *cpufreq);
static int      _cpufreq_cb_check(void *data);

static Status * _cpufreq_status_new();
static void     _cpufreq_status_free(Status *e);
static int      _cpufreq_status_check_available(Status *e);
static int      _cpufreq_status_check_current(Status *e);
static int      _cpufreq_cb_sort(void *item1, void *item2);

static Cpufreq_Face *_cpufreq_face_new(E_Container *con, Cpufreq *owner);
static void          _cpufreq_face_free(Cpufreq_Face *face);
static void          _cpufreq_face_menu_new(Cpufreq_Face *face);
static void          _cpufreq_face_enable(Cpufreq_Face *face);
static void          _cpufreq_face_disable(Cpufreq_Face *face);
static void          _cpufreq_face_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change);
static void          _cpufreq_face_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi);
static void          _cpufreq_face_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void          _cpufreq_face_update_available(Cpufreq_Face *face);
static void          _cpufreq_face_update_current(Cpufreq_Face *face);
static void          _cpufreq_face_cb_set_frequency(void *data, Evas_Object *o, const char *emission, const char *source);
static void          _cpufreq_face_cb_set_governor(void *data, Evas_Object *o, const char *emission, const char *source);

static void	_cpufreq_cb_update_policy(Cpufreq *e);

static E_Config_DD *conf_edd;
static E_Config_DD *conf_face_edd;

static int cpufreq_count;

/* public module routines */
EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION,
   "Cpufreq"
};

EAPI void *
e_modapi_init(E_Module *module)
{
   Cpufreq *freq;
   
   freq = _cpufreq_new(module);
   module->config_menu = freq->config_menu;
   return freq;
}

EAPI int
e_modapi_shutdown(E_Module *module)
{
   Cpufreq *cpufreq;

   if (module->config_menu)
     module->config_menu = NULL;

   cpufreq = module->data;
   if (cpufreq)
     _cpufreq_free(cpufreq);
   
   return 1;
}

EAPI int
e_modapi_save(E_Module *module)
{
   Cpufreq *e;

   e = module->data;
   e_config_domain_save("module.cpufreq", conf_edd, e->conf);
   return 1;
}

EAPI int
e_modapi_info(E_Module *module)
{
   char buf[4096];
   
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(module));
   module->icon_file = strdup(buf);
   return 1;
}

EAPI int
e_modapi_about(E_Module *module)
{
   e_module_dialog_show(_("CPU Frequency Controller Module"), 
			_("A simple module to control the frequency of the system CPU.<br>"
			  "This is especially useful to save power on laptops."));
   return 1;
}

static Cpufreq *
_cpufreq_new(E_Module *module)
{
   Cpufreq *e;
   Evas_List *managers, *l, *l2, *cl;
   E_Menu_Item *mi;
   char buf[4096];
   
   cpufreq_count = 0;
   e = E_NEW(Cpufreq, 1);
   if (!e) return NULL;

   conf_face_edd = E_CONFIG_DD_NEW("Cpufreq_Config_Face", Config_Face);
#undef T
#undef D
#define T Config_Face
#define D conf_face_edd
   E_CONFIG_VAL(D, T, enabled, UCHAR);

   conf_edd = E_CONFIG_DD_NEW("Cpufreq_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, poll_time, DOUBLE);
   E_CONFIG_VAL(D, T, allow_overlap, INT);
   E_CONFIG_LIST(D, T, faces, conf_face_edd);
   E_CONFIG_VAL(D, T, restore_governor, INT);
   E_CONFIG_VAL(D, T, governor, STR);
   
   e->conf = e_config_domain_load("module.cpufreq", conf_edd);
   if (!e->conf)
     {
	e->conf = E_NEW(Config, 1);
	e->conf->poll_time = 2.0;
	e->conf->restore_governor = 0;
	e->conf->governor = NULL;
	e->conf->allow_overlap = 0;
     }
   E_CONFIG_LIMIT(e->conf->poll_time, 0.5, 60.0);
   E_CONFIG_LIMIT(e->conf->allow_overlap, 0, 1);
#ifdef __FreeBSD__	
   /* does e_module_dir_get(module) work correctly in linux???  - yes it does... what's wrong in bsd? */
   snprintf(buf, sizeof(buf), "%s/%s/cpufreq/freqset", e_module_dir_get(module), MODULE_ARCH);
#else   
   snprintf(buf, sizeof(buf), "%s/%s/freqset", e_module_dir_get(module), MODULE_ARCH);
#endif
   buf[sizeof(buf) - 1] = 0;
   e->set_exe_path = strdup(buf);
   e->frequency_check_timer = ecore_timer_add(e->conf->poll_time, _cpufreq_cb_check, e);
   e->status = _cpufreq_status_new();

   _cpufreq_status_check_available(e->status);
   if ((e->conf->restore_governor) && (e->conf->governor))
     {
	/* If the governor is available, restore it */
	for (l = e->status->governors; l; l = l->next)
	  {
	     if (!strcmp(l->data, e->conf->governor))
	       _cpufreq_set_governor(e, e->conf->governor);
	  }
     }
   _cpufreq_config_menu_new(e);
	
   managers = e_manager_list();
   cl = e->conf->faces;
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
		
	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     Cpufreq_Face *ef;

	     con = l2->data;
	     ef = _cpufreq_face_new(con, e);
	     if (ef)
	       {
		  e->faces = evas_list_append(e->faces, ef);

		  if (!cl)
		    {
		       ef->conf = E_NEW(Config_Face, 1);
		       ef->conf->enabled = 1;
		       e->conf->faces = evas_list_append(e->conf->faces, ef->conf);
		    }
		  else
		    {
		       ef->conf = cl->data;
		       cl = cl->next;
		    }

		  _cpufreq_face_menu_new(ef);

		  /* Add 'Allow Overlap' option */
		  mi = e_menu_item_new(ef->menu);
		  e_menu_item_label_set(mi, _("Allow Overlap"));
		  e_menu_item_check_set(mi, 1);
		  e_menu_item_toggle_set(mi, e->conf->allow_overlap);
		  e_menu_item_callback_set(mi, _cpufreq_menu_cb_allow_overlap, e);

		  /* Add poll time menu to this face */
		  mi = e_menu_item_new(ef->menu);
		  e_menu_item_label_set(mi, _("Set Poll Time"));
		  e_menu_item_submenu_set(mi, e->config_menu_poll);

		  if (e->menu_governor)
		    {
		       mi = e_menu_item_new(ef->menu);
		       e_menu_item_label_set(mi, _("Restore Controller on Startup"));
		       e_menu_item_check_set(mi, 1);
		       e_menu_item_toggle_set(mi, e->conf->restore_governor);
		       e_menu_item_callback_set(mi, _cpufreq_menu_restore_governor, e);

		       /* Add governors menu to this face */
		       mi = e_menu_item_new(ef->menu);
		       e_menu_item_label_set(mi, _("Set Controller"));
		       e_menu_item_submenu_set(mi, e->menu_governor);
		    }
		  
		  if (e->menu_frequency)
		    {
		       /* Add frequency menu to this face */
		       mi = e_menu_item_new(ef->menu);
		       e_menu_item_label_set(mi, _("Set Speed"));
		       e_menu_item_submenu_set(mi, e->menu_frequency);
		    }

		  /* Add this face to the main menu */
		  mi = e_menu_item_new(e->config_menu);
		  e_menu_item_label_set(mi, con->name);
		  e_menu_item_submenu_set(mi, ef->menu);

		  if (!ef->conf->enabled)
		    _cpufreq_face_disable(ef);
	       }
	  }
     }

   _cpufreq_cb_check(e);

   return e;
}

static void
_cpufreq_free(Cpufreq *e)
{
   Evas_List *l;

   E_CONFIG_DD_FREE(conf_edd);
   E_CONFIG_DD_FREE(conf_face_edd);

   for (l = e->faces; l; l = l->next)
     _cpufreq_face_free(l->data);
   evas_list_free(e->faces);

   e_object_del(E_OBJECT(e->config_menu));
   e_object_del(E_OBJECT(e->config_menu_poll));
   if (e->menu_governor)
     e_object_del(E_OBJECT(e->menu_governor));
   if (e->menu_frequency)
     e_object_del(E_OBJECT(e->menu_frequency));

   ecore_timer_del(e->frequency_check_timer);

   _cpufreq_status_free(e->status);

   evas_list_free(e->conf->faces);
   free(e->set_exe_path);
   if (e->conf->governor) evas_stringshare_del(e->conf->governor);
   free(e->conf);
   free(e);
}

static void
_cpufreq_set_governor(Cpufreq *e, const char *governor)
{
   /* TODO: Use ecore_exe */
   char buf[4096];
   int ret;

   snprintf(buf, sizeof(buf), 
	    "%s %s %s", e->set_exe_path, "governor", governor);
   ret = system(buf);
   if (ret != 0)
     {
	E_Dialog *dia;

	dia = e_dialog_new(e_container_current_get(e_manager_current_get()));
	if (!dia) return;
	e_dialog_title_set(dia, "Enlightenment Cpufreq Module");
	e_dialog_icon_set(dia, "enlightenment/e", 64);
	e_dialog_text_set(dia, _("There was an error trying to set the cpu frequency<br>"
				 "governor via the module's setfreq utility."));
	e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
	e_win_centered_set(dia->win, 1);
	e_dialog_show(dia);
     }
}

static void
_cpufreq_set_frequency(Cpufreq *e, int frequency)
{
   char buf[4096];
   int ret;
   
#ifdef __FreeBSD__
   frequency /= 1000;
#endif 
   snprintf(buf, sizeof(buf),
	    "%s %s %i", e->set_exe_path, "frequency", frequency);
   ret = system(buf);
   if (ret != 0)
     {
	E_Dialog *dia;

	dia = e_dialog_new(e_container_current_get(e_manager_current_get()));
	if (!dia) return;
	e_dialog_title_set(dia, "Enlightenment Cpufreq Module");
	e_dialog_icon_set(dia, "enlightenment/e", 64);
	e_dialog_text_set(dia, _("There was an error trying to set the cpu frequency<br>"
				 "setting via the module's setfreq utility."));
	e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
	e_win_centered_set(dia->win, 1);
	e_dialog_show(dia);
     }
}

static void
_cpufreq_menu_fast(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Cpufreq *e;

   e = data;
   e->conf->poll_time = 0.5;
   ecore_timer_del(e->frequency_check_timer);
   e->frequency_check_timer = ecore_timer_add(e->conf->poll_time, _cpufreq_cb_check, e);
   e_config_save_queue();
}

static void
_cpufreq_menu_medium(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Cpufreq *e;

   e = data;
   e->conf->poll_time = 1.0;
   ecore_timer_del(e->frequency_check_timer);
   e->frequency_check_timer = ecore_timer_add(e->conf->poll_time, _cpufreq_cb_check, e);
   e_config_save_queue();
}

static void
_cpufreq_menu_normal(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Cpufreq *e;

   e = data;
   e->conf->poll_time = 2.0;
   ecore_timer_del(e->frequency_check_timer);
   e->frequency_check_timer = ecore_timer_add(e->conf->poll_time, _cpufreq_cb_check, e);
   e_config_save_queue();
}

static void
_cpufreq_menu_slow(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Cpufreq *e;

   e = data;
   e->conf->poll_time = 5.0;
   ecore_timer_del(e->frequency_check_timer);
   e->frequency_check_timer = ecore_timer_add(e->conf->poll_time, _cpufreq_cb_check, e);
   e_config_save_queue();
}

static void
_cpufreq_menu_very_slow(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Cpufreq *e;

   e = data;
   e->conf->poll_time = 30.0;
   ecore_timer_del(e->frequency_check_timer);
   e->frequency_check_timer = ecore_timer_add(e->conf->poll_time, _cpufreq_cb_check, e);
   e_config_save_queue();
}

static void _cpufreq_cb_update_policy(Cpufreq *e)
{
  Cpufreq_Face     *cf;
  Evas_List        *l;
  E_Gadman_Policy   policy;

  for (l = e->faces; l; l = l->next)
    {
      cf = l->data;
      policy = cf->gmc->policy;

      if (e->conf->allow_overlap == 0)
	policy &= ~E_GADMAN_POLICY_ALLOW_OVERLAP;
      else
	policy |= E_GADMAN_POLICY_ALLOW_OVERLAP;

      e_gadman_client_policy_set(cf->gmc, policy);
    }
}

static void _cpufreq_menu_cb_allow_overlap(void *data, E_Menu *m, E_Menu_Item *mi)
{
  Cpufreq *e;

  e = data;
  e->conf->allow_overlap = e_menu_item_toggle_get(mi);
  _cpufreq_cb_update_policy(e);
  e_config_save_queue();
}



static void
_cpufreq_menu_restore_governor(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Cpufreq *e;

   e = data;
   e->conf->restore_governor = e_menu_item_toggle_get(mi);
   if ((!e->conf->governor) || strcmp(e->status->cur_governor, e->conf->governor))
     {
	if (e->conf->governor) evas_stringshare_del(e->conf->governor);
	e->conf->governor = evas_stringshare_add(e->status->cur_governor);
     }
   e_config_save_queue();
}

static void
_cpufreq_menu_governor(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Cpufreq *e;
   char *governor;

   e = data;
   governor = e_object_data_get(E_OBJECT(mi));
   if (governor)
     {
	_cpufreq_set_governor(e, governor);
	if (e->conf->governor) evas_stringshare_del(e->conf->governor);
	e->conf->governor = evas_stringshare_add(governor);
     }
   e_config_save_queue();
}

static void
_cpufreq_menu_frequency(void * data, E_Menu *m, E_Menu_Item *mi)
{
   int frequency;
   
   frequency = (int) e_object_data_get(E_OBJECT(mi));
   if (frequency > 0)
     {
	_cpufreq_set_frequency(data, frequency);
     }
}

static void
_cpufreq_config_menu_new(Cpufreq *e)
{
   E_Menu *mn;
   E_Menu_Item *mi;
   Evas_List *l;
   char buf[256];

   mn = e_menu_new();

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Check Fast (0.5 sec)"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time <= 0.5) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _cpufreq_menu_fast, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Check Medium (1 sec)"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time > 0.5) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _cpufreq_menu_medium, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Check Normal (2 sec)"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time >= 2.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _cpufreq_menu_normal, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Check Slow (5 sec)"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time >= 5.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _cpufreq_menu_slow, e);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Check Very Slow (30 sec)"));
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (e->conf->poll_time >= 30.0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _cpufreq_menu_very_slow, e);

   e->config_menu_poll = mn;

   if (e->status->governors)
     {
	mn = e_menu_new();

	for (l = e->status->governors; l; l = l->next)
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
	     e_object_data_set(E_OBJECT(mi), l->data);
	     e_menu_item_callback_set(mi, _cpufreq_menu_governor, e);
	  }
	
	e->menu_governor = mn;
     }

   if (e->status->frequencies)
     {
	mn = e_menu_new();
	
	/* FIXME: sotring ints in pointers for freq's? BAD! */
	for (l = e->status->frequencies; l; l = l->next)
	  {
	     int frequency;
	     
	     frequency = (int)l->data;
	     mi = e_menu_item_new(mn);
	     if (frequency < 1000000)
	       snprintf(buf, sizeof(buf), _("%i MHz"), 
			frequency / 1000);
	     else
	       snprintf(buf, sizeof(buf), _("%i.%i GHz"), 
			frequency / 1000000, (frequency % 1000000) / 100000);
	     buf[sizeof(buf) - 1] = 0;
	     e_menu_item_label_set(mi, buf);
	     e_menu_item_radio_set(mi, 1);
	     e_menu_item_radio_group_set(mi, 1);
	     e_object_data_set(E_OBJECT(mi), l->data);
	     e_menu_item_callback_set(mi, _cpufreq_menu_frequency, e);
	  }
	e->menu_frequency = mn;
     }

   mn = e_menu_new();

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Allow Overlap"));
   e_menu_item_check_set(mi, 1);
   e_menu_item_toggle_set(mi, e->conf->allow_overlap);
   e_menu_item_callback_set(mi, _cpufreq_menu_cb_allow_overlap, e);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Set Poll Time"));
   e_menu_item_submenu_set(mi, e->config_menu_poll);

   if (e->menu_governor)
     {
	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Restore Governor on Startup"));
	e_menu_item_check_set(mi, 1);
	e_menu_item_toggle_set(mi, e->conf->restore_governor);
	e_menu_item_callback_set(mi, _cpufreq_menu_restore_governor, e);

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Set Controller"));
	e_menu_item_submenu_set(mi, e->menu_governor);
     }
   
   if (e->menu_frequency)
     {
	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Set Speed"));
	e_menu_item_submenu_set(mi, e->menu_frequency);
     }

   e->config_menu = mn;
}

static int
_cpufreq_cb_check(void *data)
{
   Cpufreq *e;
   Cpufreq_Face *face;
   Evas_List *l;
   int active;
   
   e = data;
   active = e->status->active;
   if (_cpufreq_status_check_current(e->status))
     {
	for (l = e->faces; l; l = l->next) 
	  {
	     face = l->data;
	     _cpufreq_face_update_current(face);
	  }
     }
   if (active != e->status->active)
     {
	for (l = e->faces; l; l = l->next) 
	  {
	     face = l->data;
	     if (e->status->active == 0)
	       edje_object_signal_emit(face->freq_object, "passive", "");
	     else if (e->status->active == 1)
	       edje_object_signal_emit(face->freq_object, "active", "");
	  }
     }

   return 1;
}

static Status *
_cpufreq_status_new()
{
   Status *e;

   e = E_NEW(Status, 1);
   if (!e) return NULL;

   e->frequencies = NULL;
   e->governors = NULL;
   e->cur_frequency = 0;
   e->can_set_frequency = 0;
   e->cur_governor = NULL;
   e->active = -1;

   return e;
}

static void
_cpufreq_status_free(Status *e)
{
   Evas_List *l;

   if (e->frequencies)
     evas_list_free(e->frequencies);
   if (e->governors)
     {
	for (l = e->governors; l; l = l->next)
	  free(l->data);
	evas_list_free(e->governors);
     }
   if (e->cur_governor)
     free(e->cur_governor);

   free(e);
}

static int
_cpufreq_cb_sort(void *item1, void *item2)
{
   int a, b;

   a = (int) item1;
   b = (int) item2;
   if (a < b)
     return -1;
   else if (a > b)
     return 1;
   else
     return 0;
}

static int
_cpufreq_status_check_available(Status *e)
{
   char buf[4096];
   Evas_List *l;
#ifdef __FreeBSD__
   int freq, i;
   size_t len = 0;
   char *freqs, *pos, *q;

   /* read freq_levels sysctl and store it in freq */
   len = sizeof(buf);
   if (sysctlbyname("dev.cpu.0.freq_levels", buf, &len, NULL, 0) == 0)
     {    
	/* sysctl returns 0 on success */
	if (e->frequencies)
	  {
	     evas_list_free(e->frequencies);
	     e->frequencies = NULL;
	  }

	/* parse freqs and store the frequencies in e->frequencies */ 
	pos = buf;
	while (pos)
	  {
	     q = strchr(pos, '/');
	     if (!q) break;

	     *q = '\0';
	     freq = atoi(pos);
	     freq *= 1000;
	     e->frequencies = evas_list_append(e->frequencies, (void *)freq);

	     pos = q + 1;
	     pos = strchr(pos, ' ');
	  }
     }
   
   /* sort is not necessary because freq_levels is already sorted */
   /* freebsd doesn't have governors */
   if (e->governors)
     {
	for (l = e->governors; l; l = l->next) free(l->data);
	evas_list_free(e->governors);
	e->governors = NULL;
     }
#else
   FILE *f;
   
   f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies", "r");
   if (f)
     {
	char *freq;
	
	if (e->frequencies)
	  {
	     evas_list_free(e->frequencies);
	     e->frequencies = NULL;
	  }
	
	fgets(buf, sizeof(buf), f);
	buf[sizeof(buf) - 1] = 0;
	fclose(f);
	
	freq = strtok(buf, " ");
	do 
	  {
	     if (atoi(freq) != 0) 
	       {
		  e->frequencies = evas_list_append(e->frequencies,	
						    (void *) atoi(freq));
	       }
	     freq = strtok(NULL, " ");
	  }
	while (freq != NULL);

	// sort list
	e->frequencies = evas_list_sort(e->frequencies,	evas_list_count(e->frequencies),
					_cpufreq_cb_sort);
     }

   f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors", "r");
   if (f)
     {
	char *gov;
	
	if (e->governors)
	  {
	     for (l = e->governors; l; l = l->next)
	       free(l->data);
	     evas_list_free(e->governors);
	     e->governors = NULL;
	  }

	fgets(buf, sizeof(buf), f);
	buf[sizeof(buf) - 1] = 0;
	fclose(f);

	gov = strtok(buf, " ");
	do 
	  {
	     while ((*gov) && (isspace(*gov))) gov++;
	     if (strlen(gov) != 0)
	       {
//		  if ((!strcmp(gov, "ondemand")) ||
//		      (!strcmp(gov, "userspace")) ||
//		      (!strcmp(gov, "powersave")) ||
//		      (!strcmp(gov, "performance"))
//		      )
		    e->governors = evas_list_append(e->governors, strdup(gov));
	       }
	     gov = strtok(NULL, " ");
	  }
	while (gov != NULL);

	e->governors = evas_list_sort(e->governors, evas_list_count(e->governors),
				      (int (*)(void *, void *))strcmp);
     }
#endif
   return 1;
}

static int
_cpufreq_status_check_current(Status *e)
{
   char buf[4096];
   int i;
   FILE *f;
   int ret = 0;
   int frequency = 0;
#ifdef __FreeBSD__
   int len = 4;

   e->active = 0;
   /* frequency is stored in dev.cpu.0.freq */
   if (sysctlbyname("dev.cpu.0.freq", &frequency, &len, NULL, 0) == 0)
     {
	frequency *= 1000;
	if (frequency != e->cur_frequency) ret = 1;
	e->cur_frequency = frequency;
	e->active = 1;
     }

    /* hardcoded for testing */
    e->can_set_frequency = 1;
    e->cur_governor = NULL;
#else
   e->active = 0;
   
   f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r");
   if (f)
     {
	fgets(buf, sizeof(buf), f);
	buf[sizeof(buf) - 1] = 0;
	fclose(f);
	
	frequency = atoi(buf);
	if (frequency != e->cur_frequency)
	  ret = 1;
	e->cur_frequency = frequency;
	e->active = 1;
     }
   
   f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed", "r");
   if (f)
     {
	e->can_set_frequency = 1;
	fclose(f);
     }
   else
     {
	e->can_set_frequency = 0;
     }
   
   f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "r");
   if (f)
     {
	fgets(buf, sizeof(buf), f);
	buf[sizeof(buf) - 1] = 0;
	fclose(f);

	if ((e->cur_governor == NULL) || (strcmp(buf, e->cur_governor)))
	  {
	     ret = 1;

	     if (e->cur_governor)
	       free(e->cur_governor);
	     e->cur_governor = strdup(buf);

	     for (i = strlen(e->cur_governor) - 1; i >= 0; i--)
	       {
		  if (isspace(e->cur_governor[i]))
		    e->cur_governor[i] = 0;
		  else
		    break;
	       }
	  }
     }
#endif
   return ret;
}

static Cpufreq_Face *
_cpufreq_face_new(E_Container *con, Cpufreq *owner)
{
   Evas_Object *o;
   Cpufreq_Face *ef;
   E_Gadman_Policy  policy;

   ef = E_NEW(Cpufreq_Face, 1);
   if (!ef) return NULL;

   ef->con = con;
   e_object_ref(E_OBJECT(con));
   ef->owner = owner;

   evas_event_freeze(con->bg_evas);

   o = edje_object_add(con->bg_evas);
   ef->freq_object = o;
   e_theme_edje_object_set(o, "base/theme/modules/cpufreq",
			   "modules/cpufreq/main");
   edje_object_signal_callback_add(o, "next_governor", "governor", _cpufreq_face_cb_set_governor, owner);
   edje_object_signal_callback_add(o, "increase_frequency", "frequency", _cpufreq_face_cb_set_frequency, owner);
   edje_object_signal_callback_add(o, "decrease_frequency", "frequency", _cpufreq_face_cb_set_frequency, owner);
   evas_object_show(o);

   o = evas_object_rectangle_add(con->bg_evas);
   ef->event_object = o;
   evas_object_layer_set(o, 0);
   evas_object_repeat_events_set(o, 1);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _cpufreq_face_cb_mouse_down, ef);
   evas_object_show(o);

   ef->gmc = e_gadman_client_new(ef->con->gadman);
   e_gadman_client_domain_set(ef->gmc, "module.cpufreq", cpufreq_count++);

   policy = E_GADMAN_POLICY_ANYWHERE  |
           E_GADMAN_POLICY_HMOVE     |
           E_GADMAN_POLICY_VMOVE     |
           E_GADMAN_POLICY_HSIZE     |
           E_GADMAN_POLICY_VSIZE;

   if (owner->conf->allow_overlap == 0)
     policy &= ~E_GADMAN_POLICY_ALLOW_OVERLAP;
   else
     policy |= E_GADMAN_POLICY_ALLOW_OVERLAP;

   e_gadman_client_policy_set(ef->gmc, policy);
   e_gadman_client_min_size_set(ef->gmc, 4, 4);
   e_gadman_client_max_size_set(ef->gmc, 128, 128);
   /* This module needs a slightly higher min size */
   e_gadman_client_auto_size_set(ef->gmc, 40, 40);
   e_gadman_client_align_set(ef->gmc, 1.0, 1.0);
   e_gadman_client_resize(ef->gmc, 40, 40);
   e_gadman_client_change_func_set(ef->gmc, _cpufreq_face_cb_gmc_change, ef);
   e_gadman_client_load(ef->gmc);

   _cpufreq_face_update_available(ef);

   evas_event_thaw(con->bg_evas);

   return ef;
}

static void
_cpufreq_face_free(Cpufreq_Face *ef)
{
   e_object_unref(E_OBJECT(ef->con));
   e_object_del(E_OBJECT(ef->gmc));
   e_object_del(E_OBJECT(ef->menu));
   evas_object_del(ef->freq_object);
   evas_object_del(ef->event_object);
   
   free(ef->conf);
   free(ef);
   cpufreq_count--;
}

static void
_cpufreq_face_menu_new(Cpufreq_Face *face)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   mn = e_menu_new();
   face->menu = mn;
   
   /* Edit */
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Edit Mode"));
   e_menu_item_callback_set(mi, _cpufreq_face_cb_menu_edit, face);
}

static void
_cpufreq_face_enable(Cpufreq_Face *face)
{
   face->conf->enabled = 1;
   evas_object_show(face->freq_object);
   evas_object_show(face->event_object);
   e_config_save_queue();
}

static void
_cpufreq_face_disable(Cpufreq_Face *face)
{
   face->conf->enabled = 0;
   evas_object_hide(face->freq_object);
   evas_object_hide(face->event_object);
   e_config_save_queue();
}

static void
_cpufreq_face_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change)
{
   Cpufreq_Face *ef;
   Evas_Coord x, y, w, h;

   ef = data;
   switch (change)
     {
      case E_GADMAN_CHANGE_MOVE_RESIZE:
	 e_gadman_client_geometry_get(ef->gmc, &x, &y, &w, &h);
	 evas_object_move(ef->freq_object, x, y);
	 evas_object_move(ef->event_object, x, y);
	 evas_object_resize(ef->freq_object, w, h);
	 evas_object_resize(ef->event_object, w, h);
	 break;
      case E_GADMAN_CHANGE_RAISE:
	 evas_object_raise(ef->freq_object);
	 evas_object_raise(ef->event_object);
	 break;
      case E_GADMAN_CHANGE_EDGE:
      case E_GADMAN_CHANGE_ZONE:
	 break;
     }
}

static void
_cpufreq_face_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Cpufreq_Face *face;

   face = data;
   e_gadman_mode_set(face->gmc->gadman, E_GADMAN_MODE_EDIT);
}

static void
_cpufreq_face_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Cpufreq_Face *ef;

   ev = event_info;
   ef = data;
   if (ev->button == 3)
     {
	e_menu_activate_mouse(ef->menu, e_zone_current_get(ef->con),
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
	e_util_container_fake_mouse_up_all_later(ef->con);
     }
}

static void
_cpufreq_face_update_available(Cpufreq_Face *face)
{
   Edje_Message_Int_Set *frequency_msg;
   Edje_Message_String_Set *governor_msg;
   Evas_List *l;
   int i;
   int count;

   count = evas_list_count(face->owner->status->frequencies);
   frequency_msg = malloc(sizeof(Edje_Message_Int_Set) + (count - 1) * sizeof(int));
   frequency_msg->count = count;
   for (l = face->owner->status->frequencies, i = 0; l; l = l->next, i++) 
     {
	frequency_msg->val[i] = (int) l->data;
     }
   edje_object_message_send(face->freq_object, EDJE_MESSAGE_INT_SET, 1, frequency_msg);
   free(frequency_msg);

   count = evas_list_count(face->owner->status->governors);
   governor_msg = malloc(sizeof(Edje_Message_String_Set) + (count - 1) * sizeof(char *));
   governor_msg->count = count;
   for (l = face->owner->status->governors, i = 0; l; l = l->next, i++)
     governor_msg->str[i] = (char *) l->data;
   edje_object_message_send(face->freq_object, EDJE_MESSAGE_STRING_SET, 2, governor_msg);
   free(governor_msg);
}

static void
_cpufreq_face_update_current(Cpufreq_Face *face)
{
   Edje_Message_Int_Set *frequency_msg;
   Edje_Message_String governor_msg;

   frequency_msg = malloc(sizeof(Edje_Message_Int_Set) + sizeof(int));
   frequency_msg->count = 2;
   frequency_msg->val[0] = face->owner->status->cur_frequency;
   frequency_msg->val[1] = face->owner->status->can_set_frequency;
   edje_object_message_send(face->freq_object, EDJE_MESSAGE_INT_SET, 3, frequency_msg);
   free(frequency_msg);

   /* BSD crashes here without the if-condition
    * since it has no governors (yet) */
   if (face->owner->status->cur_governor != NULL)
     {
         governor_msg.str = face->owner->status->cur_governor;
         edje_object_message_send(face->freq_object, EDJE_MESSAGE_STRING, 4, &governor_msg);
    }
	 
   if (face->owner->menu_frequency)
     {
	Evas_List *l;
	
	for (l = face->owner->menu_frequency->items; l; l = l->next)
	  {
	     E_Menu_Item *mi;
	     int freq;
	     
	     mi = l->data;
	     freq = (int)e_object_data_get(E_OBJECT(mi));
	     if (freq == face->owner->status->cur_frequency)
	       {
		  e_menu_item_toggle_set(mi, 1);
		  break;
	       }
	  }
     }
   if (face->owner->menu_governor)
     {
	Evas_List *l;
	
	for (l = face->owner->menu_governor->items; l; l = l->next)
	  {
	     E_Menu_Item *mi;
	     char *gov;
	     
	     mi = l->data;
	     gov = (char *)e_object_data_get(E_OBJECT(mi));
	     if (!strcmp(face->owner->status->cur_governor, gov))
	       {
		  e_menu_item_toggle_set(mi, 1);
		  break;
	       }
	  }
     }
}

static void
_cpufreq_face_cb_set_frequency(void *data, Evas_Object *obj, const char *emission, const char *src)
{
   Cpufreq *e;
   Evas_List *l;
   int next_frequency = 0;

   e = data;
   
   for (l = e->status->frequencies; l; l = l->next)
     {
	if (e->status->cur_frequency == (int) l->data)
	  {
	     if (!strcmp(emission, "increase_frequency"))
	       {
		  if (l->next)
		    next_frequency = (int) l->next->data;
		  break;
	       }
	     else if (!strcmp(emission, "decrease_frequency"))
	       {
		  if (l->prev)
		    next_frequency = (int) l->prev->data;
		  break;
	       }
	     else
	       break;
	  }
     }

   if (next_frequency != 0)
     _cpufreq_set_frequency(e, next_frequency);
}

static void
_cpufreq_face_cb_set_governor(void *data, Evas_Object *obj, const char *emission, const char *src)
{
   Cpufreq *e;
   Evas_List *l;
   char *next_governor = NULL;

   e = data;
   
   for (l = e->status->governors; l; l = l->next)
     {
	if (!strcmp(l->data, e->status->cur_governor))
	  {
	     if (l->next)
	       next_governor = l->next->data;
	     else
	       next_governor = e->status->governors->data;
	     break;
	  }
     }

   if (next_governor != NULL)
     _cpufreq_set_governor(e, next_governor);
}
