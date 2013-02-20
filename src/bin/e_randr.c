#include "e.h"

/* local function prototypes */
static Eina_Bool _e_randr_config_load(void);
static void _e_randr_config_new(void);
static void _e_randr_config_free(void);
static Eina_Bool _e_randr_config_cb_timer(void *data);
static void _e_randr_config_restore(void);

static Eina_Bool _e_randr_event_cb_screen_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);
static Eina_Bool _e_randr_event_cb_crtc_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);
static Eina_Bool _e_randr_event_cb_output_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);
static Eina_Bool _e_randr_event_cb_property_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED);

/* local variables */
static Eina_List *_randr_event_handlers = NULL;
static E_Config_DD *_e_randr_edd = NULL;
static E_Config_DD *_e_randr_crtc_edd = NULL;
static E_Config_DD *_e_randr_output_edd = NULL;

/* external variables */
EAPI E_Randr_Config *e_randr_cfg = NULL;

/* private internal functions */
EINTERN Eina_Bool 
e_randr_init(void)
{
   /* check if randr is available */
   if (!ecore_x_randr_query()) return EINA_FALSE;

   /* try to load config */
   if (!_e_randr_config_load())
     {
        /* NB: We should probably print an error here */
        return EINA_FALSE;
     }

   /* tell randr that we are interested in receiving events
    * 
    * NB: Requires RandR >= 1.2 */
   if (ecore_x_randr_version_get() >= E_RANDR_VERSION_1_2)
     {
        Ecore_X_Window root = 0;

        if ((root = ecore_x_window_root_first_get()))
          ecore_x_randr_events_select(root, EINA_TRUE);

        /* setup randr event listeners */
        E_LIST_HANDLER_APPEND(_randr_event_handlers, 
                              ECORE_X_EVENT_SCREEN_CHANGE, 
                              _e_randr_event_cb_screen_change, NULL);
        E_LIST_HANDLER_APPEND(_randr_event_handlers, 
                              ECORE_X_EVENT_RANDR_CRTC_CHANGE, 
                              _e_randr_event_cb_crtc_change, NULL);
        E_LIST_HANDLER_APPEND(_randr_event_handlers, 
                              ECORE_X_EVENT_RANDR_OUTPUT_CHANGE, 
                              _e_randr_event_cb_output_change, NULL);
        E_LIST_HANDLER_APPEND(_randr_event_handlers, 
                              ECORE_X_EVENT_RANDR_OUTPUT_PROPERTY_NOTIFY, 
                              _e_randr_event_cb_property_change, NULL);
     }

   return EINA_TRUE;
}

EINTERN int 
e_randr_shutdown(void)
{
   /* check if randr is available */
   if (!ecore_x_randr_query()) return 1;

   if (ecore_x_randr_version_get() >= E_RANDR_VERSION_1_2)
     {
        Ecore_X_Window root = 0;

        /* remove randr event listeners */
        E_FREE_LIST(_randr_event_handlers, ecore_event_handler_del);

        /* tell randr that we are not interested in receiving events anymore */
        if ((root = ecore_x_window_root_first_get()))
          ecore_x_randr_events_select(root, EINA_FALSE);
     }

   E_CONFIG_DD_FREE(_e_randr_output_edd);
   E_CONFIG_DD_FREE(_e_randr_crtc_edd);
   E_CONFIG_DD_FREE(_e_randr_edd);

   return 1;
}

/* public API functions */
EAPI Eina_Bool 
e_randr_config_save(void)
{
   /* save the new config */
   return e_config_domain_save("e_randr", _e_randr_edd, e_randr_cfg);
}

/* local functions */
static Eina_Bool 
_e_randr_config_load(void)
{
   E_Randr_Output_Config eroc;
   Eina_Bool do_restore = EINA_TRUE;

   /* define edd for output config */
   _e_randr_output_edd = 
     E_CONFIG_DD_NEW("E_Randr_Output_Config", E_Randr_Output_Config);
#undef T
#undef D
#define T E_Randr_Output_Config
#define D _e_randr_output_edd
   E_CONFIG_VAL(D, T, xid, UINT);
   E_CONFIG_VAL(D, T, crtc, UINT);
   E_CONFIG_VAL(D, T, policy, UINT);
   E_CONFIG_VAL(D, T, primary, UCHAR);
   eet_data_descriptor_element_add(D, "edid", EET_T_UCHAR, EET_G_VAR_ARRAY,
                                   (char *)(&(eroc.edid)) - (char *)(&(eroc)),
                                   (char *)(&(eroc.edid_count)) -
                                   (char *)(&(eroc)), NULL, NULL);
   eet_data_descriptor_element_add(D, "clones", EET_T_UINT, EET_G_VAR_ARRAY,
                                   (char *)(&(eroc.clones)) - (char *)(&(eroc)),
                                   (char *)(&(eroc.clone_count)) -
                                   (char *)(&(eroc)), NULL, NULL);
   E_CONFIG_VAL(D, T, connected, UCHAR);
   E_CONFIG_VAL(D, T, exists, UCHAR);

   /* define edd for crtc config */
   _e_randr_crtc_edd = 
     E_CONFIG_DD_NEW("E_Randr_Crtc_Config", E_Randr_Crtc_Config);
#undef T
#undef D
#define T E_Randr_Crtc_Config
#define D _e_randr_crtc_edd
   E_CONFIG_VAL(D, T, xid, UINT);
   E_CONFIG_VAL(D, T, x, INT);
   E_CONFIG_VAL(D, T, y, INT);
   E_CONFIG_VAL(D, T, width, INT);
   E_CONFIG_VAL(D, T, height, INT);
   E_CONFIG_VAL(D, T, orient, UINT);
   E_CONFIG_VAL(D, T, mode, UINT);
   E_CONFIG_VAL(D, T, exists, UCHAR);
   E_CONFIG_LIST(D, T, outputs, _e_randr_output_edd);

   /* define edd for randr config */
   _e_randr_edd = E_CONFIG_DD_NEW("E_Randr_Config", E_Randr_Config);
#undef T
#undef D
#define T E_Randr_Config
#define D _e_randr_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, screen.width, INT);
   E_CONFIG_VAL(D, T, screen.height, INT);
   E_CONFIG_LIST(D, T, crtcs, _e_randr_crtc_edd);
   E_CONFIG_VAL(D, T, restore, UCHAR);
   E_CONFIG_VAL(D, T, poll_interval, INT);

   /* try to load the randr config */
   if ((e_randr_cfg = e_config_domain_load("e_randr", _e_randr_edd)))
     {
        /* check randr config version */
        if (e_randr_cfg->version < (E_RANDR_CONFIG_FILE_EPOCH * 1000000))
          {
             /* config is too old */
             do_restore = EINA_FALSE;
             _e_randr_config_free();
             ecore_timer_add(1.0, _e_randr_config_cb_timer,
                             _("Settings data needed upgrading. Your old settings have<br>"
                               "been wiped and a new set of defaults initialized. This<br>"
                               "will happen regularly during development, so don't report a<br>"
                               "bug. This simply means Enlightenment needs new settings<br>"
                               "data by default for usable functionality that your old<br>"
                               "settings simply lack. This new set of defaults will fix<br>"
                               "that by adding it in. You can re-configure things now to your<br>"
                               "liking. Sorry for the hiccup in your settings.<br>"));
          }
        else if (e_randr_cfg->version > E_RANDR_CONFIG_FILE_VERSION)
          {
             /* config is too new */
             do_restore = EINA_FALSE;
             _e_randr_config_free();
             ecore_timer_add(1.0, _e_randr_config_cb_timer,
                             _("Your settings are NEWER than Enlightenment. This is very<br>"
                               "strange. This should not happen unless you downgraded<br>"
                               "Enlightenment or copied the settings from a place where<br>"
                               "a newer version of Enlightenment was running. This is bad and<br>"
                               "as a precaution your settings have been now restored to<br>"
                               "defaults. Sorry for the inconvenience.<br>"));
          }
     }

   /* if config was too old or too new, then reload a fresh one */
   if (!e_randr_cfg) 
     {
        do_restore = EINA_FALSE;
        _e_randr_config_new();
     }

   /* e_randr_config_new could return without actually creating a new config */
   if (!e_randr_cfg) return EINA_FALSE;

   /* handle upgrading any old config */
   /* if (e_randr_cfg->version < E_RANDR_CONFIG_FILE_VERSION) */
   /*   { */

   /*   } */

   if ((do_restore) && (e_randr_cfg->restore))
     _e_randr_config_restore();

   return EINA_TRUE;
}

static void 
_e_randr_config_new(void)
{
   Ecore_X_Window root = 0;
   Ecore_X_Randr_Crtc *crtcs = NULL;
   Ecore_X_Randr_Output primary = 0;
   int ncrtcs = 0, i = 0;

   /* create new randr cfg */
   if (!(e_randr_cfg = E_NEW(E_Randr_Config, 1)))
     return;

   /* set version */
   e_randr_cfg->version = E_RANDR_CONFIG_FILE_VERSION;

   /* by default, restore config */
   e_randr_cfg->restore = EINA_TRUE;

   /* by default, use 4 sec poll interval */
   e_randr_cfg->poll_interval = 32;

   /* grab the root window once */
   root = ecore_x_window_root_first_get();

   /* get which output is primary */
   primary = ecore_x_randr_primary_output_get(root);

   /* record the current screen size in our config */
   ecore_x_randr_screen_current_size_get(root, &e_randr_cfg->screen.width, 
                                         &e_randr_cfg->screen.height, 
                                         NULL, NULL);

   /* try to get the list of crtcs from x */
   if ((crtcs = ecore_x_randr_crtcs_get(root, &ncrtcs)))
     {
        /* loop the crtcs */
        for (i = 0; i < ncrtcs; i++)
          {
             E_Randr_Crtc_Config *crtc_cfg = NULL;
             Ecore_X_Randr_Output *outputs = NULL;
             int noutputs = 0;

             /* try to create new crtc config */
             if (!(crtc_cfg = E_NEW(E_Randr_Crtc_Config, 1)))
               continue;

             /* assign the xid */
             crtc_cfg->xid = crtcs[i];
             crtc_cfg->exists = EINA_TRUE;

             /* record the geometry of this crtc in our config */
             ecore_x_randr_crtc_geometry_get(root, crtcs[i], 
                                             &crtc_cfg->x, &crtc_cfg->y, 
                                             &crtc_cfg->width, 
                                             &crtc_cfg->height);

             /* record the orientation of this crtc in our config */
             crtc_cfg->orient = 
               ecore_x_randr_crtc_orientation_get(root, crtcs[i]);

             /* record the mode id of this crtc in our config */
             crtc_cfg->mode = ecore_x_randr_crtc_mode_get(root, crtcs[i]);

             /* try to get any outputs on this crtc */
             if ((outputs = 
                  ecore_x_randr_crtc_outputs_get(root, crtcs[i], &noutputs)))
               {
                  int j = 0;

                  for (j = 0; j < noutputs; j++)
                    {
                       E_Randr_Output_Config *output_cfg = NULL;
                       Ecore_X_Randr_Connection_Status status = 1;
                       int clone_count = 0;

                       /* try to create new output config */
                       if (!(output_cfg = E_NEW(E_Randr_Output_Config, 1)))
                         continue;

                       /* assign output xid */
                       output_cfg->xid = outputs[j];

                       /* assign crtc for this output */
                       output_cfg->crtc = crtcs[i];

                       /* set this output policy */
                       output_cfg->policy = ECORE_X_RANDR_OUTPUT_POLICY_NONE;

                       /* get if this output is the primary */
                       output_cfg->primary = EINA_FALSE;
                       if (outputs[j] == primary) 
                         output_cfg->primary = EINA_TRUE;

                       /* record the edid for this output */
                       output_cfg->edid = 
                         ecore_x_randr_output_edid_get(root, outputs[j], 
                                                       &output_cfg->edid_count);

                       /* get the clones for this output */
                       output_cfg->clones = 
                         ecore_x_randr_output_clones_get(root, outputs[j], 
                                                         &clone_count);
                       
                       output_cfg->clone_count = (long)clone_count;

                       status = 
                         ecore_x_randr_output_connection_status_get(root, outputs[j]);

                       output_cfg->connected = EINA_FALSE;
                       if (status == ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED)
                         output_cfg->connected = EINA_TRUE;

                       output_cfg->exists = EINA_TRUE;

                       /* add this output to the list for this crtc */
                       crtc_cfg->outputs = 
                         eina_list_append(crtc_cfg->outputs, output_cfg);
                    }

                  free(outputs);
               }

             /* append this crtc config to randr config */
             e_randr_cfg->crtcs = 
               eina_list_append(e_randr_cfg->crtcs, crtc_cfg);
          }

        free(crtcs);
     }

   /* set limits */
   E_CONFIG_LIMIT(e_randr_cfg->poll_interval, 1, 1024);

   /* save the new config */
   e_randr_config_save();
}

static void 
_e_randr_config_free(void)
{
   E_Randr_Crtc_Config *crtc = NULL;

   /* safety check */
   if (!e_randr_cfg) return;

   /* loop the config crtcs and free them */
   EINA_LIST_FREE(e_randr_cfg->crtcs, crtc)
     {
        E_Randr_Output_Config *output = NULL;

        /* loop the config outputs on this crtc and free them */
        EINA_LIST_FREE(crtc->outputs, output)
          {
             if (output->clones) free(output->clones);
             if (output->edid) free(output->edid);

             E_FREE(output);
          }

        E_FREE(crtc);
     }

   /* free the config */
   E_FREE(e_randr_cfg);
}

static Eina_Bool 
_e_randr_config_cb_timer(void *data)
{
   e_util_dialog_show(_("Randr Settings Upgraded"), "%s", (char *)data);
   return EINA_FALSE;
}

static void 
_e_randr_config_restore(void)
{
   Ecore_X_Window root = 0;
   Ecore_X_Randr_Crtc *crtcs;
   int ncrtcs = 0;
   Eina_Bool need_reset = EINA_FALSE;

   /* try to restore settings
    * 
    * NB: When we restore, check the resolutions (current vs saved)
    * and if there is no change then we do not need to call 
    * screen_reset as this triggers a full comp refresh. We can 
    * accomplish this simply by checking the mode */

   /* grab the root window */
   root = ecore_x_window_root_first_get();

   /* try to get the list of crtcs */
   if ((crtcs = ecore_x_randr_crtcs_get(root, &ncrtcs)))
     {
        int i = 0;

        for (i = 0; i < ncrtcs; i++)
          {
             Eina_List *l;
             E_Randr_Crtc_Config *crtc_cfg;
             Ecore_X_Randr_Mode mode;
             Ecore_X_Randr_Output *outputs;
             int noutputs = 0;

             /* get the mode */
             mode = ecore_x_randr_crtc_mode_get(root, crtcs[i]);

             /* get the outputs */
             outputs = 
               ecore_x_randr_crtc_outputs_get(root, crtcs[i], &noutputs);

             /* loop our config and find this crtc */
             EINA_LIST_FOREACH(e_randr_cfg->crtcs, l, crtc_cfg)
               {
                  /* try to find this crtc */
                  if (crtc_cfg->xid != crtcs[i]) continue;

                  /* apply the stored settings */
                  if (!crtc_cfg->mode)
                    ecore_x_randr_crtc_settings_set(root, crtc_cfg->xid, 
                                                    NULL, 0, 0, 0, 0, 
                                                    ECORE_X_RANDR_ORIENTATION_ROT_0);
                  else
                    ecore_x_randr_crtc_settings_set(root, crtc_cfg->xid, 
                                                    outputs, noutputs, 
                                                    crtc_cfg->x, crtc_cfg->y, 
                                                    crtc_cfg->mode, 
                                                    crtc_cfg->orient);

                  if (crtc_cfg->mode != mode)
                    need_reset = EINA_TRUE;

                  break;
               }

             /* free any allocated memory from ecore_x_randr */
             free(outputs);
          }

        /* free any allocated memory from ecore_x_randr */
        free(crtcs);
     }

   if (need_reset) ecore_x_randr_screen_reset(root);
}

static Eina_Bool 
_e_randr_event_cb_screen_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_X_Event_Screen_Change *ev;
   Eina_Bool changed = EINA_FALSE;

   ev = event;

   printf("E_RANDR Event: Screen Change\n");

   if (e_randr_cfg->screen.width != ev->size.width)
     {
        e_randr_cfg->screen.width = ev->size.width;
        changed = EINA_TRUE;
     }

   if (e_randr_cfg->screen.height != ev->size.height)
     {
        e_randr_cfg->screen.height = ev->size.height;
        changed = EINA_TRUE;
     }

   if (changed) e_randr_config_save();

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool 
_e_randr_event_cb_crtc_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_X_Event_Randr_Crtc_Change *ev;
   Eina_List *l = NULL;
   E_Randr_Crtc_Config *crtc_cfg;
   Eina_Bool changed = EINA_FALSE;

   ev = event;
   if (ev->crtc == 0) return ECORE_CALLBACK_RENEW;

   printf("E_RANDR Event: Crtc Change: %d\n", ev->crtc);

   EINA_LIST_FOREACH(e_randr_cfg->crtcs, l, crtc_cfg)
     {
        if (crtc_cfg->xid == ev->crtc)
          {
             if ((crtc_cfg->x != ev->geo.x) || 
                 (crtc_cfg->y != ev->geo.y) || 
                 (crtc_cfg->width != ev->geo.w) || 
                 (crtc_cfg->height != ev->geo.h) || 
                 (crtc_cfg->orient != ev->orientation) || 
                 (crtc_cfg->mode != ev->mode))
               {
                  crtc_cfg->x = ev->geo.x;
                  crtc_cfg->y = ev->geo.y;
                  crtc_cfg->width = ev->geo.w;
                  crtc_cfg->height = ev->geo.h;
                  crtc_cfg->orient = ev->orientation;
                  crtc_cfg->mode = ev->mode;

                  changed = EINA_TRUE;
               }

             break;
          }
     }

   if (changed) e_randr_config_save();

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool 
_e_randr_event_cb_output_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_X_Event_Randr_Output_Change *ev;
   Eina_List *l = NULL;
   E_Randr_Crtc_Config *crtc_cfg;
   Eina_Bool changed = EINA_FALSE;

   ev = event;

   /* TODO: NB: Hmmm, this is problematic :( The spec says we should get an 
    * event here when an output is disconnected (hotplug) if 
    * the hardware (video card) is capable of detecting this HOWEVER, in my 
    * tests, my nvidia card does not detect this.
    * 
    * To work around this, we have added a poller to check X randr config 
    * against what we have saved in e_randr_cfg */
   printf("E_RANDR Event: Output Change: %d\n", ev->output);

   EINA_LIST_FOREACH(e_randr_cfg->crtcs, l, crtc_cfg)
     {
        Eina_List *o;
        E_Randr_Output_Config *output_cfg;

        if (ev->crtc != crtc_cfg->xid) continue;

        if ((crtc_cfg->mode != ev->mode) || 
            (crtc_cfg->orient != ev->orientation))
          {
             crtc_cfg->mode = ev->mode;
             crtc_cfg->orient = ev->orientation;
             changed = EINA_TRUE;
          }

        EINA_LIST_FOREACH(crtc_cfg->outputs, o, output_cfg)
          {
             if (output_cfg->xid == ev->output)
               {
                  Eina_Bool connected = EINA_FALSE;

                  connected = ((ev->connection) ? EINA_FALSE : EINA_TRUE);

                  if ((output_cfg->crtc != ev->crtc) || 
                      (output_cfg->connected != connected))
                    {
                       printf("Output Changed: %d\n", ev->output);
                       printf("\tConnected: %d\n", connected);

                       output_cfg->crtc = ev->crtc;
                       output_cfg->connected = connected;
                       output_cfg->exists = connected;

                       changed = EINA_TRUE;
                    }

                  break;
               }
          }
     }

   if (changed) e_randr_config_save();

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool 
_e_randr_event_cb_property_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event EINA_UNUSED)
{
   /* Ecore_X_Event_Randr_Output_Property_Notify *ev; */

   /* ev = event; */
   printf("E_RANDR Event: Property Change\n");
   return ECORE_CALLBACK_RENEW;
}
