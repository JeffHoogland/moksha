#include "e.h"
#include "e_mod_main.h"
#include "e_int_config_randr.h"
#include "e_smart_randr.h"
#include "e_smart_monitor.h"

/* local structures */
struct _E_Config_Dialog_Data
{
   Evas_Object *o_scroll;
};

/* local function prototypes */
static void *_create_data(E_Config_Dialog *cfd __UNUSED__);
static void _free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _randr_cb_changed(void *data, Evas_Object *obj __UNUSED__, void *event __UNUSED__);

/* local variables */

E_Config_Dialog *
e_int_config_randr(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_randr_screen_info.randr_version < ECORE_X_RANDR_1_2) 
     return NULL;

   if (e_config_dialog_find("E", "screen/screen_setup")) return NULL;

   if (!(v = E_NEW(E_Config_Dialog_View, 1))) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;

   v->override_auto_apply = EINA_TRUE;

   cfd = e_config_dialog_new(con, _("Screen Setup"), 
                             "E", "screen/screen_setup", 
                             "preferences-system-screen-resolution", 
                             0, v, NULL);

   /* NB: These are just arbitrary values I picked. Feel free to change */
   e_win_size_min_set(cfd->dia->win, 180, 230);
   e_dialog_resizable_set(cfd->dia, 1);

   return cfd;
}

/* local functions */
static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   if (!(cfdata = E_NEW(E_Config_Dialog_Data, 1))) return NULL;
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   evas_object_smart_callback_del(cfdata->o_scroll, "changed", 
                                  _randr_cb_changed);
   evas_object_del(cfdata->o_scroll);

   E_FREE(cfdata);
}

static int 
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Eina_List *l;
   Evas_Object *mon;
   Ecore_X_Window root;
   Eina_Bool reset = EINA_FALSE;

   root = cfd->con->manager->root;

   EINA_LIST_FOREACH(e_smart_randr_monitors_get(cfdata->o_scroll), l, mon)
     {
        E_Randr_Crtc_Info *crtc;
        E_Randr_Output_Info *output;
        E_Smart_Monitor_Changes changes = E_SMART_MONITOR_CHANGED_NONE;

        if (!(output = e_smart_monitor_output_get(mon)))
          continue;

        if (!(crtc = e_smart_monitor_crtc_get(mon)))
          {
             Eina_List *c;

             EINA_LIST_FOREACH(E_RANDR_12->crtcs, c, crtc)
               {
                  if (crtc->current_mode)
                    {
                       crtc = NULL;
                       continue;
                    }
                  break;
               }
          }

        if (!crtc) continue;

        if (!(changes = e_smart_monitor_changes_get(mon)))
          continue;

        if (changes & E_SMART_MONITOR_CHANGED_ENABLED)
          {
             if (e_smart_monitor_enabled_get(mon))
               {
                  if (crtc)
                    {
                       Ecore_X_Randr_Output *outputs;
                       Ecore_X_Randr_Mode_Info *mode;
                       Ecore_X_Randr_Orientation orient;
                       Evas_Coord mx, my;
                       int noutputs = -1;

                       mode = e_smart_monitor_mode_get(mon);
                       orient = e_smart_monitor_orientation_get(mon);
                       e_smart_monitor_position_get(mon, &mx, &my);

                       noutputs = eina_list_count(crtc->outputs);
                       if (noutputs < 1)
                         {
                            outputs = calloc(1, sizeof(Ecore_X_Randr_Output));
                            outputs[0] = output->xid;
                            noutputs = 1;
                         }
                       else
                         {
                            int i = 0;

                            outputs = 
                              calloc(noutputs, sizeof(Ecore_X_Randr_Output));
                            for (i = 0; i < noutputs; i++)
                              {
                                 E_Randr_Output_Info *ero;

                                 ero = eina_list_data_get(eina_list_nth(crtc->outputs, i));
                                 outputs[i] = ero->xid;
                              }
                         }

                       ecore_x_randr_crtc_settings_set(root, crtc->xid, 
                                                       outputs, 
                                                       noutputs, mx, my,
                                                       mode->xid, orient);
                       if (outputs) free(outputs);
                    }
               }
             else
               ecore_x_randr_crtc_settings_set(root, crtc->xid, 
                                               NULL, 0, 0, 0, 0, 
                                               ECORE_X_RANDR_ORIENTATION_ROT_0);

             reset = EINA_TRUE;
          }

        if (changes & E_SMART_MONITOR_CHANGED_POSITION)
          {
             if (crtc)
               {
                  Evas_Coord mx, my;
                  Evas_Coord cx, cy;

                  e_smart_monitor_position_get(mon, &mx, &my);
                  ecore_x_randr_crtc_pos_get(root, crtc->xid, &cx, &cy);
                  if ((cx != mx) || (cy != my))
                    {
                       ecore_x_randr_crtc_pos_set(root, crtc->xid, mx, my);
                       reset = EINA_TRUE;
                    }
               }
          }

        if (changes & E_SMART_MONITOR_CHANGED_ROTATION)
          {
             if (crtc)
               {
                  Ecore_X_Randr_Orientation orient;

                  orient = e_smart_monitor_orientation_get(mon);
                  if ((crtc) && (orient != crtc->current_orientation))
                    {
                       ecore_x_randr_crtc_orientation_set(root, 
                                                          crtc->xid, orient);
                       reset = EINA_TRUE;
                    }
               }
          }

        if ((changes & E_SMART_MONITOR_CHANGED_REFRESH) || 
            (changes & E_SMART_MONITOR_CHANGED_RESOLUTION))
          {
             if (crtc)
               {
                  Ecore_X_Randr_Mode_Info *mode;
                  Ecore_X_Randr_Output *outputs = NULL;
                  int noutputs = -1;

                  if (output) outputs = &output->xid;

                  if ((crtc) && (crtc->outputs))
                    noutputs = eina_list_count(crtc->outputs);

                  mode = e_smart_monitor_mode_get(mon);
                  ecore_x_randr_crtc_mode_set(root, crtc->xid, 
                                              outputs, noutputs, mode->xid);
                  reset = EINA_TRUE;
               }
          }

        if (reset)
          {
             /* monitors changes have been sent. Signal this monitor so that 
              * we can reset the 'original' values to the 'current' values 
              * and reset the 'changes' variable */
             e_smart_monitor_changes_sent(mon);
          }
     }

   if (reset) ecore_x_randr_screen_reset(root);

   return 1;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o;
   Eina_List *l;
   E_Randr_Output_Info *output;

   o = e_widget_list_add(evas, 0, 0);

   cfdata->o_scroll = e_smart_randr_add(evas);
   e_smart_randr_virtual_size_set(cfdata->o_scroll, 
                                  E_RANDR_12->max_size.width,
                                  E_RANDR_12->max_size.height);
   evas_object_show(cfdata->o_scroll);

   evas_object_smart_callback_add(cfdata->o_scroll, "changed", 
                                  _randr_cb_changed, cfd);

   /* create monitors based on 'outputs' */
   EINA_LIST_FOREACH(E_RANDR_12->outputs, l, output)
     {
        Evas_Object *m;

        if (!output) continue;

        if (output->connection_status != 
            ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED)
          continue;

        if (!(m = e_smart_monitor_add(evas))) continue;
        e_smart_monitor_info_set(m, output, output->crtc);
        e_smart_randr_monitor_add(cfdata->o_scroll, m);
        evas_object_show(m);
     }

   e_widget_list_object_append(o, cfdata->o_scroll, 1, 1, 0.5);

   return o;
}

static void 
_randr_cb_changed(void *data, Evas_Object *obj, void *event __UNUSED__)
{
   E_Config_Dialog *cfd;
   Eina_Bool changed = EINA_FALSE;

   if (!(cfd = data)) return;
   changed = e_smart_randr_changed_get(obj);
   e_config_dialog_changed_set(cfd, changed);
}
