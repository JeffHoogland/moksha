#include <e.h>
#include "e_mod_main.h"
#include "e_mod_gadman.h"
#include "e_mod_config.h"
#include "config.h"

struct _E_Config_Dialog_Data
{
   Evas_Object *o_avail;        //List of available gadgets
   Evas_Object *o_add;          //Add button
};

/* Local protos */
static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _fill_gadgets_list(Evas_Object *ilist);
static void _cb_add(void *data, void *data2);
static void _avail_list_cb_change(void *data, Evas_Object *obj);

EAPI E_Config_Dialog *
e_int_config_gadman_module(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   char buf[4096];

   /* check if config dialog exists ... */
   if (e_config_dialog_find("E", "_e_modules_gadman_config_dialog"))
     return NULL;

   /* ... else create it */
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->override_auto_apply = 0; //TODO this should remove the ok/apply buttons ... but doesnt work :(

   snprintf(buf, sizeof(buf), "%s/e-module-gadman.edj", Man->module->dir);
   cfd = e_config_dialog_new(con, _("Gadgets Manager"),
                             "E", "_e_modules_gadman_config_dialog",
                             buf, 0, v, Man);

   Man->config_dialog = cfd;

   return Man->config_dialog;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Man->config_dialog = NULL;
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob, *ol;

   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Available Gadgets"), 0);

   //o_avail  List of available gadgets
   ol = e_widget_ilist_add(evas, 24, 24, NULL);
   e_widget_ilist_multi_select_set(ol, 0);
   e_widget_on_change_hook_set(ol, _avail_list_cb_change, cfdata);
   cfdata->o_avail = ol;
   _fill_gadgets_list(ol);
   e_widget_framelist_object_append(of, ol);

   //o_add  Button to add a gadget
   ob = e_widget_button_add(evas, _("Add Gadget"), NULL, _cb_add, cfdata, NULL);
   e_widget_disabled_set(ob, 1);
   cfdata->o_add = ob;
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   gadman_gadget_edit_end();
   e_config_save_queue();
   return 1;
}

static void 
_fill_gadgets_list(Evas_Object *ilist)
{
   Evas_List *l = NULL;
   int w;

   e_widget_ilist_freeze(ilist);
   e_widget_ilist_clear(ilist);

   for (l = e_gadcon_provider_list(); l; l = l->next) 
     {
        E_Gadcon_Client_Class *cc;
        Evas_Object *icon = NULL;
        const char *lbl = NULL;

        if (!(cc = l->data)) continue;
        if (cc->func.label) lbl = cc->func.label();
        if (!lbl) lbl = cc->name;
        if (cc->func.icon) icon = cc->func.icon(Man->gc->evas);
        e_widget_ilist_append(ilist, icon, lbl, NULL, (void *)cc, NULL);
     }

   e_widget_ilist_go(ilist);
   e_widget_min_size_get(ilist, &w, NULL);
   if (w < 200) w = 200;
   e_widget_min_size_set(ilist, w, 250);
   e_widget_ilist_thaw(ilist);
}

static void 
_cb_add(void *data, void *data2) 
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *l = NULL;
   int i;

   if (!(cfdata = data)) return;

   for (i = 0, l = e_widget_ilist_items_get(cfdata->o_avail); l; l = l->next, i++) 
     {
        E_Ilist_Item *item = NULL;
        E_Gadcon_Client_Class *cc;
        E_Gadcon_Client *gcc;

        if (!(item = l->data)) continue;
        if (!item->selected) continue;

        cc = e_widget_ilist_nth_data_get(cfdata->o_avail, i);
        if (!cc) continue;

        gcc = gadman_gadget_add(cc, 0);
        gadman_gadget_edit_start(gcc);
     }

   if (l) evas_list_free(l);
}

static void 
_avail_list_cb_change(void *data, Evas_Object *obj) 
{
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = data)) return;
   e_widget_disabled_set(cfdata->o_add, 0);
}
