#include "e.h"

/* local function prototypes */
static Eina_Bool _e_randr_config_load(void);
static void _e_randr_config_new(void);
static void _e_randr_config_free(void);
static Eina_Bool _e_randr_config_cb_timer(void *data);
static void _e_randr_config_restore(void);
static Eina_Bool _e_randr_config_crtc_update(E_Randr_Crtc_Config *cfg);
static Eina_Bool _e_randr_config_output_update(E_Randr_Output_Config *cfg);
static E_Randr_Crtc_Config *_e_randr_config_output_crtc_find(E_Randr_Output_Config *cfg);
static Ecore_X_Randr_Mode _e_randr_config_output_preferred_mode_get(E_Randr_Output_Config *cfg);
static E_Randr_Output_Config *_e_randr_config_output_new(unsigned int id);

static Eina_Bool _e_randr_event_cb_screen_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);
static Eina_Bool _e_randr_event_cb_crtc_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);
static Eina_Bool _e_randr_event_cb_output_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event);

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
   E_CONFIG_VAL(D, T, config_timestamp, ULL);
   E_CONFIG_VAL(D, T, primary, INT);

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
   e_randr_cfg->primary = ecore_x_randr_primary_output_get(root);

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

             /* fill in crtc_cfg values from X */
             _e_randr_config_crtc_update(crtc_cfg);

             /* try to get any outputs on this crtc */
             if ((outputs = 
                  ecore_x_randr_crtc_outputs_get(root, crtcs[i], &noutputs)))
               {
                  int j = 0;

                  for (j = 0; j < noutputs; j++)
                    {
                       E_Randr_Output_Config *output_cfg = NULL;

                       /* try to create new output config */
                       if (!(output_cfg = _e_randr_config_output_new(outputs[j])))
                         continue;

                       /* assign crtc for this output */
                       output_cfg->crtc = crtcs[i];
                       output_cfg->exists = EINA_TRUE;
                       if ((int)outputs[j] == e_randr_cfg->primary)
                         output_cfg->primary = EINA_TRUE;

                       if (!primary)
                         {
                            /* X has no primary output set */
                            if (j == 0)
                              {
                                 /* if no primary is set, then we should 
                                  * use the first output listed by xrandr */
                                 output_cfg->primary = EINA_TRUE;
                                 e_randr_cfg->primary = (int)outputs[j];

                                 ecore_x_randr_primary_output_set(root, 
                                                                  e_randr_cfg->primary);
                              }
                         }

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

   /* update recorded config timestamp */
   e_randr_cfg->config_timestamp = ecore_x_randr_config_timestamp_get(root);

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
   int sw = 0, sh = 0;

   printf("E_RANDR Restore\n");

   /* grab the root window */
   root = ecore_x_window_root_first_get();

   /* get the current screen size */
   ecore_x_randr_screen_current_size_get(root, &sw, &sh, NULL, NULL);
   printf("\tCurrent Screen Size: %d %d\n", sw, sh);

   /* set the screen size */
   /* NB: Disabled for now as our saved screen size may not be valid 
    * if any of our saved outputs are missing in X */
   /* ecore_x_randr_screen_current_size_set(root, e_randr_cfg->screen.width,  */
   /*                                       e_randr_cfg->screen.height, -1, -1); */

   /* try to get the list of existing crtcs from X */
   if ((crtcs = ecore_x_randr_crtcs_get(root, &ncrtcs)))
     {
        int i = 0;
        Eina_List *valid_crtcs = NULL;
        Eina_List *l;
        E_Randr_Crtc_Config *crtc_cfg;
        Eina_Bool primary_set = EINA_FALSE;

        printf("\tHave Crtcs\n");

        /* for each crtc that X has, check our config and see if we have it */
        for (i = 0; i < ncrtcs; i++)
          {
             Eina_Bool crtc_found = EINA_FALSE;

             printf("\t\tChecking For Crtc: %d in Our Config\n", crtcs[i]);

             /* loop our config and find this crtc */
             EINA_LIST_FOREACH(e_randr_cfg->crtcs, l, crtc_cfg)
               {
                  /* try to find this crtc */
                  if (crtc_cfg->xid != crtcs[i]) continue;

                  crtc_found = EINA_TRUE;
                  printf("\t\t\tFound Crtc in Config\n");
                  valid_crtcs = eina_list_append(valid_crtcs, crtc_cfg);
               }

             if (!crtc_found)
               {
                  printf("\t\t\tCrtc Not Found in Config\n");
                  printf("\t\t\t\tCreating New\n");
                  /* create new crtc config */
                  if ((crtc_cfg = E_NEW(E_Randr_Crtc_Config, 1)))
                    {
                       /* assign id */
                       crtc_cfg->xid = crtcs[i];
                       crtc_cfg->exists = EINA_TRUE;

                       /* fill in crtc_cfg values from X */
                       _e_randr_config_crtc_update(crtc_cfg);

                       printf("\t\t\t\tNew Crtc Geom: %d %d %d %d\n",
                              crtc_cfg->x, crtc_cfg->y, crtc_cfg->width, 
                              crtc_cfg->height);
                       printf("\t\t\t\tNew Crtc Mode: %d\n", crtc_cfg->mode);

                       /* append to randr cfg */
                       e_randr_cfg->crtcs = 
                         eina_list_append(e_randr_cfg->crtcs, crtc_cfg);

                       e_randr_config_save();

                       /* append to our short list */
                       valid_crtcs = eina_list_append(valid_crtcs, crtc_cfg);
                    }
               }
          }

        free(crtcs);

        printf("\t\tLooping Valid Crtcs\n");

        /* for each X crtc that we have config for, check outputs */
        EINA_LIST_FOREACH(valid_crtcs, l, crtc_cfg)
          {
             Ecore_X_Randr_Output *outputs;
             int noutputs;
             Eina_List *valid_outputs = NULL;

             /* we have this crtc. try to get list of outputs from X */
             if ((outputs = 
                  ecore_x_randr_crtc_outputs_get(root, crtc_cfg->xid, &noutputs)))
               {
                  int j = 0;

                  printf("\t\t\tHave X Outputs For Crtc: %d\n", crtc_cfg->xid);

                  /* loop the X outputs and find a matching config */
                  for (j = 0; j < noutputs; j++)
                    {
                       Eina_List *ll;
                       E_Randr_Output_Config *output_cfg;
                       Ecore_X_Randr_Connection_Status status = 
                         ECORE_X_RANDR_CONNECTION_STATUS_UNKNOWN;

                       status = 
                         ecore_x_randr_output_connection_status_get(root, 
                                                                    outputs[j]);
                       if (status != ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED)
                         continue;

                       printf("\t\t\t\tChecking for Output: %d in Our Config\n", outputs[j]);

                       /* loop our outputs for this crtc */
                       EINA_LIST_FOREACH(crtc_cfg->outputs, ll, output_cfg)
                         {
                            /* try to find this output */
                            if (output_cfg->xid != outputs[j]) continue;

                            /* we have this output */
                            printf("\t\t\t\tFound Output In Config: %d\n", outputs[j]);

                            /* add this output config to the list of 
                             * ones that we are going to restore for 
                             * this crtc */
                            valid_outputs = 
                              eina_list_append(valid_outputs, output_cfg);

                            break;
                         }
                    }

                  free(outputs);
               }
             else
               {
                  printf("\t\t\tNo Outputs For Crtc: %d\n", crtc_cfg->xid);
                  /* crtc has no outputs assigned */
                  /* Need to check possibles */
               }

             /* apply settings for this crtc */
             printf("\t\t\t\tApplying Crtc Settings: %d\n", crtc_cfg->xid);

             if (!crtc_cfg->mode)
               {
                  printf("\t\t\t\t\tCRTC HAS NO MODE !!!\n");
                  ecore_x_randr_crtc_settings_set(root, crtc_cfg->xid, 
                                                  NULL, 0, 0, 0, 0, 
                                                  ECORE_X_RANDR_ORIENTATION_ROT_0);
               }
             else 
               {
                  int ocount, c = 0;

                  ocount = eina_list_count(valid_outputs);
                  printf("\t\t\t\t\tNum Outputs: %d\n", ocount);

                  if (ocount > 0)
                    {
                       Ecore_X_Randr_Output *couts;
                       Eina_List *o;
                       E_Randr_Output_Config *out;
                       Eina_Bool need_size_set = EINA_FALSE;

                       couts = malloc(ocount * sizeof(Ecore_X_Randr_Output));
                       EINA_LIST_FOREACH(valid_outputs, o, out)
                         {
                            couts[c] = out->xid;
                            c++;
                         }

                       switch (crtc_cfg->orient)
                         {
                          case ECORE_X_RANDR_ORIENTATION_ROT_0:
                          case ECORE_X_RANDR_ORIENTATION_ROT_180:
                            if (sw < (crtc_cfg->x + crtc_cfg->width))
                              {
                                 sw += crtc_cfg->width;
                                 need_size_set = EINA_TRUE;
                              }
                            if (sh < (crtc_cfg->y + crtc_cfg->height))
                              {
                                 sh = (crtc_cfg->y + crtc_cfg->height);
                                 need_size_set = EINA_TRUE;
                              }
                            break;
                          case ECORE_X_RANDR_ORIENTATION_ROT_90:
                          case ECORE_X_RANDR_ORIENTATION_ROT_270:
                            if (sw < (crtc_cfg->x + crtc_cfg->width))
                              {
                                 sw = (crtc_cfg->x + crtc_cfg->width);
                                 need_size_set = EINA_TRUE;
                              }
                            if (sh < (crtc_cfg->y + crtc_cfg->height))
                              {
                                 sh += crtc_cfg->height;
                                 need_size_set = EINA_TRUE;
                              }
                            break;
                          default:
                            break;
                         }

                       if (need_size_set)
                         {

                            printf("\t\t\t\t\tNew Screen Size: %d %d\n", 
                                   sw, sh);
                            ecore_x_randr_screen_current_size_set(root, 
                                                                  sw, sh, 
                                                                  -1, -1);
                            need_size_set = EINA_FALSE;
                         }

                       printf("\t\t\t\t\tCrtc Settings: %d %d %d %d %d %d\n", 
                              crtc_cfg->xid, crtc_cfg->x, crtc_cfg->y, 
                              crtc_cfg->width, crtc_cfg->height, 
                              crtc_cfg->mode);

                       ecore_x_randr_crtc_settings_set(root, crtc_cfg->xid, 
                                                       couts, ocount, 
                                                       crtc_cfg->x, 
                                                       crtc_cfg->y, 
                                                       crtc_cfg->mode, 
                                                       crtc_cfg->orient);

                       if (!primary_set)
                         {
                            EINA_LIST_FOREACH(valid_outputs, o, out)
                              {
                                 if ((out->primary) && 
                                     ((int)out->xid == e_randr_cfg->primary))
                                   {
                                      ecore_x_randr_primary_output_set(root, out->xid);
                                      primary_set = EINA_TRUE;
                                      break;
                                   }
                              }
                         }

                       if (!primary_set)
                         {
                            /* if no primary was set, set it to the first output */
                            out = eina_list_nth(valid_outputs, 0);
                            ecore_x_randr_primary_output_set(root, out->xid);
                            e_randr_cfg->primary = (int)out->xid;
                            e_randr_config_save();
                            primary_set = EINA_TRUE;
                         }

                       free(couts);
                    }
               }
             eina_list_free(valid_outputs);
          }
        eina_list_free(valid_crtcs);
     }

//   if (need_reset) ecore_x_randr_screen_reset(root);
}

static Eina_Bool 
_e_randr_event_cb_screen_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_X_Event_Screen_Change *ev;
   Eina_Bool changed = EINA_FALSE;
   Ecore_X_Randr_Output primary = 0;

   ev = event;

   printf("E_RANDR Event: Screen Change: %d %d\n", 
          ev->size.width, ev->size.height);

   /* check if this event's root window is Our root window */
   if (ev->root != e_manager_current_get()->root) 
     return ECORE_CALLBACK_RENEW;

   primary = ecore_x_randr_primary_output_get(ev->root);

   if (e_randr_cfg->primary != (int)primary)
     {
        e_randr_cfg->primary = (int)primary;
        changed = EINA_TRUE;
     }

   if (e_randr_cfg->screen.width != ev->size.width)
     {
        printf("\tWidth Changed\n");
        e_randr_cfg->screen.width = ev->size.width;
        changed = EINA_TRUE;
     }

   if (e_randr_cfg->screen.height != ev->size.height)
     {
        printf("\tHeight Changed\n");
        e_randr_cfg->screen.height = ev->size.height;
        changed = EINA_TRUE;
     }

   if (e_randr_cfg->config_timestamp != ev->config_time)
     {
        printf("\tConfig Timestamp Changed\n");
        e_randr_cfg->config_timestamp = ev->config_time;
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
   Eina_Bool crtc_new = EINA_FALSE;
   Eina_Bool crtc_found = EINA_FALSE;
   Eina_Bool crtc_changed = EINA_FALSE;

   ev = event;

   /* loop our crtc configs and try to find this one */
   EINA_LIST_FOREACH(e_randr_cfg->crtcs, l, crtc_cfg)
     {
        /* skip if not this crtc */
        if (crtc_cfg->xid != ev->crtc) continue;

        crtc_found = EINA_TRUE;
        break;
     }

   if (!crtc_found)
     {
        /* if this crtc is not found in our config, create it */
        if ((crtc_cfg = E_NEW(E_Randr_Crtc_Config, 1)))
          {
             /* assign id */
             crtc_cfg->xid = ev->crtc;
             crtc_cfg->exists = EINA_TRUE;

             crtc_new = EINA_TRUE;

             /* append to randr cfg */
             e_randr_cfg->crtcs = 
               eina_list_append(e_randr_cfg->crtcs, crtc_cfg);
          }
     }

   /* check (and update if needed) our crtc config
    * NB: This will fill in any new ones also */
   crtc_changed = _e_randr_config_crtc_update(crtc_cfg);

   /* save the config if anything changed or we added a new one */
   if ((crtc_changed) || (crtc_new)) 
     {
        printf("E_RANDR Event: Crtc Change\n");
        printf("\tCrtc: %d Changed or New. Saving Config\n", ev->crtc);
        e_randr_config_save();
     }

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool 
_e_randr_event_cb_output_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_X_Event_Randr_Output_Change *ev;
   Eina_List *l = NULL;
   E_Randr_Crtc_Config *crtc_cfg;
   E_Randr_Output_Config *output_cfg;
   Eina_Bool output_new = EINA_FALSE;
   Eina_Bool output_found = EINA_FALSE;
   Eina_Bool output_changed = EINA_FALSE;
   Eina_Bool output_removed = EINA_FALSE;

   ev = event;

   /* check if this event's root window is Our root window */
   if (ev->win != e_manager_current_get()->root) 
     return ECORE_CALLBACK_RENEW;

   printf("E_RANDR Event: Output Change\n");
   printf("\tOutput: %d\n", ev->output);

   if (ev->crtc)
     printf("\t\tCrtc: %lu\n", (unsigned long)ev->crtc);
   else
     printf("\t\tNo Crtc\n");

   printf("\t\tMode: %d\n", ev->mode);

   if (ev->connection == 0)
     printf("\t\tOutput Connected\n");
   else if (ev->connection == 1)
     printf("\t\tOutput Disconnected\n");

   /* loop our crtcs and try to find this output */
   printf("\tLooping Our Crtc Configs\n");
   EINA_LIST_FOREACH(e_randr_cfg->crtcs, l, crtc_cfg)
     {
        Eina_List *ll;

        /* loop the outputs in our crtc cfg and try to find this one */
        printf("\t\tLooping Our Output Configs on this Crtc: %d\n", crtc_cfg->xid);
        EINA_LIST_FOREACH(crtc_cfg->outputs, ll, output_cfg)
          {
             /* try to find this output */
             if (output_cfg->xid != ev->output) continue;

             /* FIXME: NB: Hmmm, we may need to also compare edids here (not just X id) */

             printf("\t\t\tFound Output %d on Crtc: %d\n", output_cfg->xid, output_cfg->crtc);
             output_found = EINA_TRUE;

             /* is this output still on the same crtc ? */
             if (output_cfg->crtc != ev->crtc)
               {
                  printf("\t\t\t\tOutput Moved Crtc or Removed\n");

                  /* if event crtc is 0, then this output is not assigned to any crtc, 
                   * so we need to remove it from any existing crtc_cfg Outputs.
                   * 
                   * NB: In a typical scenario, we would remove and free this output cfg, 
                   * HOWEVER we will NOT do that here. Reasoning is that if someone 
                   * replugs this output, we can restore any saved config. 
                   * 
                   * NB: Do not call _e_randr_config_output_update in this case as that will 
                   * overwrite any of our saved config
                   * 
                   * So for now, just disable it in config by setting exists == FALSE */
                  if (!ev->crtc)
                    {
                       /* free this output_cfg */
                       /* if (output_cfg->clones) free(output_cfg->clones); */
                       /* if (output_cfg->edid) free(output_cfg->edid); */
                       /* E_FREE(output_cfg); */

                       /* remove from this crtc */
                       /* crtc_cfg->outputs = eina_list_remove_list(crtc_cfg->outputs, ll); */

                       /* just mark it as not existing */
                       output_cfg->exists = EINA_FALSE;

                       /* set flag */
                       output_removed = EINA_TRUE;
                    }
                  else
                    {
                       /* output moved to new crtc */
                       printf("\t\t\tOutput Moved to New Crtc\n");
                    }
               }
             else
               {
                  printf("\t\t\t\tOutput On Same Crtc\n");

                  /* check (and update if needed) our output config */
                  output_changed = _e_randr_config_output_update(output_cfg);
               }

             if (output_found) break;
          }

        if (output_found) break;
     }

   /* if the output was not found above, and it is plugged in, 
    * then we need to create a new one */
   if ((!output_found) && (ev->connection == 0))
     {
        printf("\tOutput Not Found In Config: %d\n", ev->output);
        printf("\t\tCreate New Output Config\n");

        if ((output_cfg = _e_randr_config_output_new(ev->output)))
          {
             output_new = EINA_TRUE;

             /* since this is a new output cfg, the above 
              * output_update function (inside new) will set 'exists' to false 
              * because no crtc has been assigned yet.
              * 
              * We need to find a valid crtc for this output and set the 
              * 'crtc' and 'exists' properties */
             if ((crtc_cfg = _e_randr_config_output_crtc_find(output_cfg)))
               {
                  Ecore_X_Randr_Mode mode;
                  int ocount, c = 0;

                  /* we found a valid crtc for this output */
                  output_cfg->crtc = crtc_cfg->xid;
                  output_cfg->exists = (output_cfg->crtc != 0);

                  printf("\t\t\tOutput Crtc Is: %d\n", output_cfg->crtc);

                  /* get the preferred mode for this output */
                  if ((mode = _e_randr_config_output_preferred_mode_get(output_cfg)))
                    {
                       Evas_Coord mw = 0, mh = 0;

                       /* get the size of this mode */
                       ecore_x_randr_mode_size_get(ev->win, mode, &mw, &mh);

                       /* update crtc config with this mode info */
                       crtc_cfg->mode = mode;
                       crtc_cfg->width = mw;
                       crtc_cfg->height = mh;
                    }

                  /* append this output_cfg to the crtc_cfg list of outputs */
                  crtc_cfg->outputs = 
                    eina_list_append(crtc_cfg->outputs, output_cfg);

                  /* tell X about this new output */
                  ocount = eina_list_count(crtc_cfg->outputs);
                  printf("\tNum Outputs: %d\n", ocount);

                  if (ocount > 0)
                    {
                       Ecore_X_Randr_Output *couts;
                       Eina_List *o;
                       E_Randr_Output_Config *out;

                       couts = malloc(ocount * sizeof(Ecore_X_Randr_Output));
                       EINA_LIST_FOREACH(crtc_cfg->outputs, o, out)
                         {
                            couts[c] = out->xid;
                            c++;
                         }

                       printf("\tCrtc Settings: %d %d %d %d\n", crtc_cfg->xid, 
                              crtc_cfg->x, crtc_cfg->y, crtc_cfg->mode);

                       ecore_x_randr_crtc_settings_set(ev->win, crtc_cfg->xid, 
                                                       couts, ocount, 
                                                       crtc_cfg->x, 
                                                       crtc_cfg->y, 
                                                       crtc_cfg->mode, 
                                                       crtc_cfg->orient);
                       free(couts);
                    }
               }
          }
     }

   /* save the config if anything changed or we added a new one */
   if ((output_changed) || (output_new) || (output_removed))
     {
        printf("\t\t\t\tOutput Changed, Added, or Removed. Saving Config\n");
        e_randr_config_save();
     }

   /* if we added or removed any outputs, we need to reset */
   if ((output_new) || (output_removed))
     ecore_x_randr_screen_reset(ev->win);

   return ECORE_CALLBACK_RENEW;
}

/* This function compares our crtc config against what X has and updates our
 * view of this crtc. It returns EINA_TRUE is anything changed 
 * 
 * NB: This Does Not Handle Outputs on the Crtc.*/
static Eina_Bool 
_e_randr_config_crtc_update(E_Randr_Crtc_Config *cfg)
{
   Ecore_X_Window root = 0;
   Eina_Bool ret = EINA_FALSE;

   /* grab the root window */
   root = ecore_x_window_root_first_get();

#if ((ECORE_VERSION_MAJOR >= 1) && (ECORE_VERSION_MINOR >= 8))
   Ecore_X_Randr_Crtc_Info *cinfo;

   /* get crtc info from X */
   if ((cinfo = ecore_x_randr_crtc_info_get(root, cfg->xid)))
     {
        /* check for changes */
        if ((cfg->x != cinfo->x) || (cfg->y != cinfo->y) || 
            (cfg->width != (int)cinfo->width) || (cfg->height != (int)cinfo->height) || 
            (cfg->mode != cinfo->mode) || (cfg->orient != cinfo->rotation))
          {
             cfg->x = cinfo->x;
             cfg->y = cinfo->y;
             cfg->width = cinfo->width;
             cfg->height = cinfo->height;
             cfg->mode = cinfo->mode;
             cfg->orient = cinfo->rotation;

             ret = EINA_TRUE;
          }

        ecore_x_randr_crtc_info_free(cinfo);
     }
#else
   Evas_Coord x, y, w, h;
   unsigned int orient, mode;

   /* get geometry of this crtc */
   ecore_x_randr_crtc_geometry_get(root, ev->crtc, &x, &y, &w, &h);
   if ((cfg->x != x) || (cfg->y != y) || 
       (cfg->width != w) || (cfg->height != h))
     {
        cfg->x = x;
        cfg->y = y;
        cfg->width = w;
        cfg->height = h;

        ret = EINA_TRUE;
     }

   /* get orientation */
   orient = ecore_x_randr_crtc_orientation_get(root, cfg->xid);
   if (cfg->orient != orient)
     {
        cfg->orient = orient;
        ret = EINA_TRUE;
     }

   /* get mode */
   mode = ecore_x_randr_crtc_mode_get(root, cfg->xid);
   if (cfg->mode != mode)
     {
        cfg->mode = mode;
        ret = EINA_TRUE;
     }
#endif

   return ret;
}

static Eina_Bool 
_e_randr_config_output_update(E_Randr_Output_Config *cfg)
{
   Ecore_X_Window root = 0;
   Eina_Bool ret = EINA_FALSE;
   Ecore_X_Randr_Output primary = 0;
   Ecore_X_Randr_Crtc crtc;
   Ecore_X_Randr_Connection_Status status;
   /* int clone_count = 0; */

   /* grab the root window */
   root = ecore_x_window_root_first_get();

   /* get which output is primary */
   primary = ecore_x_randr_primary_output_get(root);

   /* set this output policy */
   cfg->policy = ECORE_X_RANDR_OUTPUT_POLICY_NONE;

   /* get if this output is the primary */
   if (cfg->primary != ((cfg->xid == primary)))
     {
        cfg->primary = ((cfg->xid == primary));
        ret = EINA_TRUE;
     }

   /* get the crtc for this output */
   crtc = ecore_x_randr_output_crtc_get(root, cfg->xid);
   if (cfg->crtc != crtc)
     {
        cfg->crtc = crtc;
        ret = EINA_TRUE;
     }

   /* does it exist in X ?? */
   if (cfg->exists != (crtc != 0))
     {
        cfg->exists = (crtc != 0);
        ret = EINA_TRUE;
     }

   /* record the edid for this output */
   /* cfg->edid = ecore_x_randr_output_edid_get(root, cfg->xid, &cfg->edid_count); */

   /* get the clones for this output */
   /* cfg->clones =  */
   /*   ecore_x_randr_output_clones_get(root, cfg->xid, &clone_count); */
   /* cfg->clone_count = (unsigned long)clone_count; */

   status = ecore_x_randr_output_connection_status_get(root, cfg->xid);
   if (cfg->connected != (status == ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED))
     {
        cfg->connected = (status == ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED);
        ret = EINA_TRUE;
     }

   return ret;
}

static E_Randr_Crtc_Config *
_e_randr_config_output_crtc_find(E_Randr_Output_Config *cfg)
{
   Ecore_X_Window root = 0;
   E_Randr_Crtc_Config *crtc_cfg = NULL;
   Ecore_X_Randr_Crtc *possible;
   int num = 0, i = 0;
   Eina_List *l;
   Eina_Bool crtc_found = EINA_FALSE;

   /* grab the root window */
   root = ecore_x_window_root_first_get();

   /* get a list of possible crtcs for this output */
   if (!(possible = 
         ecore_x_randr_output_possible_crtcs_get(root, cfg->xid, &num)))
     return NULL;

   if (num == 0)
     {
        if (possible) free(possible);
        return NULL;
     }

   /* loop the possible crtcs */
   for (i = 0; i < num; i++)
     {
        /* loop our crtc configs and try to find this one */
        EINA_LIST_FOREACH(e_randr_cfg->crtcs, l, crtc_cfg)
          {
             /* skip if not the one we are looking for */
             if (crtc_cfg->xid != possible[i]) continue;

             /* check if this crtc already has outputs assigned.
              * skip if it does because we are trying to find a free crtc */
             if (eina_list_count(crtc_cfg->outputs) > 0) continue;

             crtc_found = EINA_TRUE;
             break;
          }

        if (crtc_found) break;
     }

   free(possible);

   if (crtc_found) return crtc_cfg;

   return NULL;
}

static Ecore_X_Randr_Mode 
_e_randr_config_output_preferred_mode_get(E_Randr_Output_Config *cfg)
{
   Ecore_X_Window root = 0;
   Ecore_X_Randr_Mode *modes;
   Ecore_X_Randr_Mode mode;
   int n = 0, p = 0;

   /* grab the root window */
   root = ecore_x_window_root_first_get();

   /* get the list of modes for this output */
   if (!(modes = ecore_x_randr_output_modes_get(root, cfg->xid, &n, &p)))
     return 0;

   if (n == 0)
     {
        if (modes) free(modes);
        return 0;
     }

   mode = modes[p - 1];
   free(modes);

   return mode;
}

static E_Randr_Output_Config *
_e_randr_config_output_new(unsigned int id)
{
   E_Randr_Output_Config *cfg = NULL;

   if ((cfg = E_NEW(E_Randr_Output_Config, 1)))
     {
        /* assign output xid */
        cfg->xid = id;

        /* check (and update if needed) our output config */
        _e_randr_config_output_update(cfg);
     }

   return cfg;
}
