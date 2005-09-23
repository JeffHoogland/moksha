/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/*
 * TODO:
 * * Check if randr is available. It might be disabled in
 *   ecore_x, or not available on screen
 * * Make a clock ticking down on the dialog
 * * Add destroy callback for dialog
 */

static Randr *_randr_new(void);
static void _randr_free(Randr *e);
static void _randr_config_menu_new(Randr *e);
static void _randr_menu_resolution_add(void *data, E_Menu *m);
static void _randr_menu_resolution_del(void *data, E_Menu *m);
static void _randr_menu_cb_store(void *data, E_Menu *m, E_Menu_Item *mi);
static void _randr_menu_cb_resolution_change(void *data, E_Menu *m, E_Menu_Item *mi);
static void _randr_dialog_cb_ok(void *data, E_Dialog *dia);
static void _randr_dialog_cb_cancel(void *data, E_Dialog *dia);
static int  _randr_timer_cb(void *data);
static void _randr_save_res(Randr_Resolution *res);

static E_Config_DD *conf_edd;
static E_Config_DD *conf_manager_edd;

E_Module_Api e_module_api = 
{
   E_MODULE_API_VERSION,
   "RandR"
};

void *
e_modapi_init(E_Module *m)
{
   Randr *e;
   
   /* Create the button */
   e = _randr_new();
   m->config_menu = e->config_menu;
   return e;
}

int
e_modapi_shutdown(E_Module *m)
{
   Randr *e;

   if (m->config_menu) m->config_menu = NULL;
   
   e = m->data;
   if (e) _randr_free(e);
   return 1;
}

int
e_modapi_save(E_Module *m)
{
   Randr *e;
   
   e = m->data;
   e_config_domain_save("module.randr", conf_edd, e->conf);
   
   return 1;
}

int
e_modapi_info(E_Module *m)
{
   /*
   char buf[4096];
   */
   
   m->label = strdup(_("Randr"));
   /*
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
   */
   return 1;
}

int
e_modapi_about(E_Module *m)
{
   e_module_dialog_show(_("Enlightenment Randr Module"), 
			_("Module to change screen resolution for E17"));
   return 1;
}

static Randr *
_randr_new(void)
{
   Randr *e;
   
   e = E_NEW(Randr, 1);
   if (!e) return NULL;
   
   conf_manager_edd = E_CONFIG_DD_NEW("Randr_Config_Manager", Config_Manager);
#undef T
#undef D
#define T Config_Manager
#define D conf_manager_edd
   E_CONFIG_VAL(D, T, manager, INT);
   E_CONFIG_VAL(D, T, width, INT);
   E_CONFIG_VAL(D, T, height, INT);

   conf_edd = E_CONFIG_DD_NEW("Randr_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, store, INT);
   E_CONFIG_LIST(D, T, managers, conf_manager_edd);
   
   e->conf = e_config_domain_load("module.randr", conf_edd);
   if (!e->conf)
     {
       	e->conf = E_NEW(Config, 1);
	e->conf->store = 1;
     }
   else if ((e->conf->store) && (e->conf->managers))
     {
	/* Restore resoultion */
	Evas_List *l;
	Ecore_X_Screen_Size size;

	for (l = e->conf->managers; l; l = l->next)
	  {
	     E_Manager *man;
	     Config_Manager *cm;

	     cm = l->data;
	     man = e_manager_number_get(cm->manager);
	     size = ecore_x_randr_current_screen_size_get(man->root);
	     if ((man) && ((cm->width != size.width) || (cm->height != size.height)))
	       {
		  size.width = cm->width;
		  size.height = cm->height;
		  ecore_x_randr_screen_size_set(man->root, size);
	       }
	  }
     }
 
   _randr_config_menu_new(e);

   e->augmentation = e_int_menus_menu_augmentation_add("config",
						       _randr_menu_resolution_add, e,
						       _randr_menu_resolution_del, e);

   return e;
}

static void
_randr_free(Randr *e)
{
   Evas_List *l;

   E_CONFIG_DD_FREE(conf_edd);
   E_CONFIG_DD_FREE(conf_manager_edd);

   if (e->timer)
     ecore_timer_del(e->timer);
   if (e->dialog)
     e_object_del(E_OBJECT(e->dialog));
   
   e_object_del(E_OBJECT(e->config_menu));
   if (e->resolution_menu)
     e_object_del(E_OBJECT(e->resolution_menu));
   
   e_int_menus_menu_augmentation_del("config", e->augmentation);
   for (l = e->conf->managers; l; l = l->next)
     free(l->data);
   evas_list_free(e->conf->managers);
   free(e->conf);
   free(e);
}

static void
_randr_config_menu_new(Randr *e)
{
   E_Menu *m;
   E_Menu_Item *mi;

   m = e_menu_new();
   e->config_menu = m;

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Restore Resolution on Startup"));
   e_menu_item_check_set(mi, 1);
   e_menu_item_toggle_set(mi, e->conf->store);
   e_menu_item_callback_set(mi, _randr_menu_cb_store, e);
}

static void
_randr_menu_resolution_add(void *data, E_Menu *m)
{
   E_Manager *man;
   Randr *e;
   E_Menu *subm, *root;
   E_Menu_Item *mi;
   Ecore_X_Screen_Size *sizes;
   Ecore_X_Screen_Size size;
   int i, n;

   e = data;

   subm = e_menu_new();
   e->resolution_menu = subm;

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Resolution"));
   e_menu_item_submenu_set(mi, subm);

   root = e_menu_root_get(m);
   if (!root->zone)
     man = e_manager_current_get();
   else
     man = root->zone->container->manager;

   /* Get the size and possible sizes for the manager */
   sizes = ecore_x_randr_screen_sizes_get(man->root, &n);
   size = ecore_x_randr_current_screen_size_get(man->root);

   if (sizes)
     {
	Randr_Resolution *res;
	char buf[16];

	for (i = 0; i < n; i++)
	  {
	     res = E_NEW(Randr_Resolution, 1);
	     if (!res) continue;

	     res->prev = size;
	     res->next = sizes[i];
	     e_object_ref(E_OBJECT(man));
	     res->manager = man;
	     res->randr = e;

	     snprintf(buf, sizeof(buf), "%dx%d", sizes[i].width, sizes[i].height);
	     mi = e_menu_item_new(subm);
	     e_menu_item_radio_set(mi, 1);
	     e_menu_item_radio_group_set(mi, 1);
	     if ((sizes[i].width == size.width) && (sizes[i].height == size.height))
	       e_menu_item_toggle_set(mi, 1);
	     e_menu_item_label_set(mi, buf);
	     e_menu_item_callback_set(mi, _randr_menu_cb_resolution_change, e);

	     e_object_data_set(E_OBJECT(mi), res);

	  }
	free(sizes);
     }
}

static void
_randr_menu_resolution_del(void *data, E_Menu *m)
{
   Randr *e;

   e = data;

   if (e->resolution_menu)
     {
	Evas_List *l;

	for (l = e->resolution_menu->items; l; l = l->next)
	  {
	     E_Object *obj;
	     void *data;

	     obj = l->data;
	     data = e_object_data_get(obj);
	     if (data)
	       free(data);
	  }

	e_object_del(E_OBJECT(e->resolution_menu));
	e->resolution_menu = NULL;
     }
}

static void
_randr_menu_cb_store(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Randr *e;

   e = data;
   e->conf->store = e_menu_item_toggle_get(mi);
}

static void
_randr_menu_cb_resolution_change(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Randr *e;
   Randr_Resolution *res;
   
   e = data;
   res = e_object_data_get(E_OBJECT(mi));
   e_object_data_set(E_OBJECT(mi), NULL);
   if (!res) return;

   ecore_x_randr_screen_size_set(res->manager->root, res->next);

   e->dialog = e_dialog_new(m->zone->container);
   e_dialog_title_set(e->dialog, "Resolution change");
   e_dialog_text_set(e->dialog, "Keep new resolution?");
   e_dialog_button_add(e->dialog, "OK", NULL, _randr_dialog_cb_ok, res);
   e_dialog_button_add(e->dialog, "Cancel", NULL, _randr_dialog_cb_cancel, res);
   e_win_borderless_set(e->dialog->win, 1);
   e_win_layer_set(e->dialog->win, 6);
   e_win_centered_set(e->dialog->win, 1);
   e_win_sticky_set(e->dialog->win, 1);
   e_dialog_show(e->dialog);

   e->timer = ecore_timer_add(15.0, _randr_timer_cb, res);
}

static void
_randr_dialog_cb_ok(void *data, E_Dialog *dia)
{
   Randr_Resolution *res;

   /* Do nothing */
   res = data;
   _randr_save_res(res);
   e_object_unref(E_OBJECT(res->manager));
   e_object_del(E_OBJECT(res->randr->dialog));
   res->randr->dialog = NULL;
   if (res->randr->timer)
     {
	ecore_timer_del(res->randr->timer);
	res->randr->timer = NULL;
     }
   free(res);
}

static void
_randr_dialog_cb_cancel(void *data, E_Dialog *dia)
{
   Randr_Resolution *res;

   /* Restore old resolution */
   res = data;
   ecore_x_randr_screen_size_set(res->manager->root, res->prev);
   e_object_unref(E_OBJECT(res->manager));
   e_object_del(E_OBJECT(res->randr->dialog));
   res->randr->dialog = NULL;
   if (res->randr->timer)
     {
	ecore_timer_del(res->randr->timer);
	res->randr->timer = NULL;
     }
   free(res);
}

static int
_randr_timer_cb(void *data)
{
   Randr_Resolution *res;

   /* Restore old resolution */
   res = data;
   ecore_x_randr_screen_size_set(res->manager->root, res->prev);
   e_object_unref(E_OBJECT(res->manager));
   e_object_del(E_OBJECT(res->randr->dialog));
   res->randr->dialog = NULL;
   res->randr->timer = NULL;
   free(res);
   return 0;
}

static void
_randr_save_res(Randr_Resolution *res)
{
   Config_Manager *cm = NULL;
   Evas_List *l;

   /* Find this manager config */
   for (l = res->randr->conf->managers; l; l = l->next)
     {
	Config_Manager *current;

	current = l->data;
	if (current->manager == res->manager->num)
	  {
	     cm = current;
	     break;
	  }
     }
   /* If not found, create new config */
   if (!cm)
     {
	cm = E_NEW(Config_Manager, 1);
	if (cm)
	  res->randr->conf->managers = evas_list_append(res->randr->conf->managers, cm);
     }
   /* Save config */
   if (cm)
     {
	cm->manager = res->manager->num;
	cm->width = res->next.width;
	cm->height = res->next.height;
     }
   e_config_save_queue();
}
