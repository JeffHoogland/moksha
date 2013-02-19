#include "e.h"

#define E_RANDR_VERSION_1_1 ((1 << 16) | 1)
#define E_RANDR_VERSION_1_2 ((1 << 16) | 2)
#define E_RANDR_VERSION_1_3 ((1 << 16) | 3)
#define E_RANDR_VERSION_1_4 ((1 << 16) | 4)

/* local function prototypes */
static Eina_Bool _e_randr_config_load(void);
static void _e_randr_config_new(void);
static void _e_randr_config_free(void);
static Eina_Bool _e_randr_config_cb_timer(void *data);

static Eina_Bool _e_randr_event_cb_screen_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);
static Eina_Bool _e_randr_event_cb_crtc_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);
static Eina_Bool _e_randr_event_cb_output_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);
static Eina_Bool _e_randr_event_cb_property_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);

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

   /* try to restore settings */

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
   E_CONFIG_VAL(D, T, screen.timestamp, DOUBLE);
   E_CONFIG_LIST(D, T, crtcs, _e_randr_crtc_edd);

   /* try to load the randr config */
   if ((e_randr_cfg = e_config_domain_load("e_randr", _e_randr_edd)))
     {
        /* check randr config version */
        if (e_randr_cfg->version < (E_RANDR_CONFIG_FILE_EPOCH * 1000000))
          {
             /* config is too old */
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
   if (!e_randr_cfg) _e_randr_config_new();

   /* e_randr_config_new could return without actually creating a new config */
   if (!e_randr_cfg) return EINA_FALSE;

   /* handle upgrading any old config */
   if (e_randr_cfg->version < E_RANDR_CONFIG_FILE_VERSION)
     {
        printf("TODO: Upgrade Old Randr Config !!\n");
     }

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

                       /* try to create new output config */
                       if (!(output_cfg = E_NEW(E_Randr_Output_Config, 1)))
                         continue;

                       /* assign output xid */
                       output_cfg->xid = outputs[j];

                       /* assign crtc for this output */
                       output_cfg->crtc = crtcs[i];
                       /* TODO: Add code to determine policy */

                       /* get if this output is the primary */
                       output_cfg->primary = EINA_FALSE;
                       if (outputs[j] == primary) 
                         output_cfg->primary = EINA_TRUE;

                       /* record the edid for this output */
                       output_cfg->edid = 
                         ecore_x_randr_output_edid_get(root, outputs[j], 
                                                       &output_cfg->edid_count);

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
   /* E_CONFIG_LIMIT(); */

   /* save the new config */
   e_config_domain_save("e_randr", _e_randr_edd, e_randr_cfg);
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

static Eina_Bool 
_e_randr_event_cb_screen_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_X_Event_Screen_Change *ev;

   ev = event;
   printf("E_RANDR Event: Screen Change\n");
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool 
_e_randr_event_cb_crtc_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_X_Event_Randr_Crtc_Change *ev;

   ev = event;
   printf("E_RANDR Event: Crtc Change\n");
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool 
_e_randr_event_cb_output_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_X_Event_Randr_Output_Change *ev;

   ev = event;
   printf("E_RANDR Event: Output Change\n");
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool 
_e_randr_event_cb_property_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_X_Event_Randr_Output_Property_Notify *ev;

   ev = event;
   printf("E_RANDR Event: Property Change\n");
   return ECORE_CALLBACK_RENEW;
}
