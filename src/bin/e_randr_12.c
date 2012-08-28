#include "e.h"
#include "e_randr_private.h"

#define POLLINTERVAL 128

// Set functions for the global e_randr_screen_info struct
static void                     _screen_primary_output_assign(E_Randr_Output_Info *removed);

// Init helper functions
static void                     _outputs_init(void);
static void                     _crtcs_init(void);
static Eina_Bool                _structs_init(void);

// Retrieval helper functions
static Ecore_X_Randr_Mode_Info *_mode_geo_identical_find(Eina_List *modes, Ecore_X_Randr_Mode_Info *mode);

// Event helper functions
static Eina_Bool                _x_poll_cb(void *data __UNUSED__);
static Eina_Bool                _crtc_change_event_cb(void *data, int type, void *e);
static Eina_Bool                _output_change_event_cb(void *data, int type, void *e);
static Eina_Bool                _output_property_change_event_cb(void *data, int type, void *e);

static Ecore_Poller *poller = NULL;
static Eina_List *_event_handlers = NULL;
static const char *_CONNECTION_STATES_STRINGS[] = {"CONNECTED", "DISCONNECTED", "UNKNOWN"};
static const char *_POLICIES_STRINGS[] = {"ABOVE", "RIGHT", "BELOW", "LEFT", "CLONE", "NONE"};

//"New" helper functions
/**
 * @return array of E_Randr_Screen_Info_12 elements, or in case not all could
 * be created or parameter 'nrequested'==0, NULL
 */
static E_Randr_Screen_Info_12 *
_screen_info_12_new(void)
{
   E_Randr_Screen_Info_12 *randr_info_12 = NULL;

   EINA_SAFETY_ON_TRUE_RETURN_VAL((e_randr_screen_info.randr_version < ECORE_X_RANDR_1_2), NULL);

   randr_info_12 = E_NEW(E_Randr_Screen_Info_12, 1);

   randr_info_12->min_size.width = Ecore_X_Randr_Unset;
   randr_info_12->min_size.height = Ecore_X_Randr_Unset;
   randr_info_12->max_size.width = Ecore_X_Randr_Unset;
   randr_info_12->max_size.height = Ecore_X_Randr_Unset;
   randr_info_12->current_size.width = Ecore_X_Randr_Unset;
   randr_info_12->current_size.height = Ecore_X_Randr_Unset;
   randr_info_12->crtcs = NULL;
   randr_info_12->outputs = NULL;
   randr_info_12->modes = NULL;
   randr_info_12->primary_output = NULL;
   randr_info_12->alignment = ECORE_X_RANDR_RELATIVE_ALIGNMENT_NONE;

   ecore_x_randr_screen_size_range_get(e_randr_screen_info.root,
                                       &randr_info_12->min_size.width,
                                       &randr_info_12->min_size.height,
                                       &randr_info_12->max_size.width,
                                       &randr_info_12->max_size.height);
   ecore_x_randr_screen_current_size_get(e_randr_screen_info.root,
                                         &randr_info_12->current_size.width,
                                         &randr_info_12->current_size.height,
                                         NULL, NULL);

   return randr_info_12;
}

static Eina_Bool
_structs_init(void)
{
   //Output stuff
   Ecore_X_Randr_Output *outputs;
   E_Randr_Output_Info *output_info = NULL;
   int noutputs = 0;
   //CRTC stuff
   Ecore_X_Randr_Crtc *crtcs = NULL;
   E_Randr_Crtc_Info *crtc_info = NULL;
   int ncrtcs = 0;
   //Modes stuff
   Ecore_X_Randr_Mode_Info **modes = NULL;
   int nmodes = 0;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(E_RANDR_12_NO, EINA_FALSE);

   outputs = ecore_x_randr_outputs_get(e_randr_screen_info.root, &noutputs);
   if (noutputs == 0) return EINA_FALSE;

   while (--noutputs >= 0)
     {
        output_info = _output_info_new(outputs[noutputs]);
        if (output_info)
          e_randr_screen_info.rrvd_info.randr_info_12->outputs = eina_list_append(e_randr_screen_info.rrvd_info.randr_info_12->outputs, output_info);
     }
   free(outputs);

   crtcs = ecore_x_randr_crtcs_get(e_randr_screen_info.root, &ncrtcs);
   if (ncrtcs == 0) return EINA_FALSE;

   while (--ncrtcs >= 0)
     {
        crtc_info = _crtc_info_new(crtcs[ncrtcs]);
        e_randr_screen_info.rrvd_info.randr_info_12->crtcs = eina_list_append(e_randr_screen_info.rrvd_info.randr_info_12->crtcs, crtc_info);
     }
   free(crtcs);

   modes = ecore_x_randr_modes_info_get(e_randr_screen_info.root, &nmodes);
   if (nmodes == 0) return EINA_FALSE;

   while (--nmodes >= 0)
     {
        e_randr_screen_info.rrvd_info.randr_info_12->modes = eina_list_append(e_randr_screen_info.rrvd_info.randr_info_12->modes, modes[nmodes]);
     }

   free(modes);
   _outputs_init();
   _crtcs_init();

   return EINA_TRUE;
}

//Set value / retrieval helper functions

static void
_crtcs_init(void)
{
   E_Randr_Crtc_Info *crtc = NULL;
   Eina_List *iter;

   EINA_SAFETY_ON_TRUE_RETURN(E_RANDR_12_NO);

   EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->crtcs, iter, crtc)
     _crtc_refs_set(crtc);
}

static void
_outputs_init(void)
{
   E_Randr_Output_Info *output = NULL;
   Eina_List *iter;

   EINA_SAFETY_ON_TRUE_RETURN(E_RANDR_12_NO);

   EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->outputs, iter, output)
     {
        _output_refs_set(output);
        if (output->connection_status == ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED)
          {
             _monitor_info_free(output->monitor);
             output->monitor = _monitor_info_new(output);
          }
     }
}

static void
_screen_primary_output_assign(E_Randr_Output_Info *removed)
{
   Eina_List *iter;
   E_Randr_Output_Info *primary_output = NULL, *output_info;

   EINA_SAFETY_ON_TRUE_RETURN(E_RANDR_12_NO_OUTPUTS);

   if (e_randr_screen_info.rrvd_info.randr_info_12->primary_output && (removed != e_randr_screen_info.rrvd_info.randr_info_12->primary_output)) return;
   if (!(primary_output = _12_screen_info_output_info_get(ecore_x_randr_primary_output_get(e_randr_screen_info.root))))
     {
        EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->outputs, iter, output_info)
          {
             if (!output_info || (output_info->connection_status != ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED) || !output_info->crtc || !output_info->crtc->current_mode) continue;
             primary_output = output_info;
             break;
          }
     }
   e_randr_screen_info.rrvd_info.randr_info_12->primary_output = primary_output;
}

//"Free" helper functions

/**
 * @param screen_info the screen info to be freed.
 */
void
_12_screen_info_free(E_Randr_Screen_Info_12 *screen_info)
{
   Ecore_X_Randr_Mode_Info *mode_info;
   E_Randr_Crtc_Info *crtc_info;
   E_Randr_Output_Info *output_info;

   EINA_SAFETY_ON_NULL_RETURN(screen_info);
   EINA_SAFETY_ON_TRUE_RETURN(E_RANDR_12_NO);

   if (screen_info->crtcs)
     {
        EINA_LIST_FREE(screen_info->crtcs, crtc_info)
          _crtc_info_free(crtc_info);
        free(eina_list_nth(screen_info->crtcs, 0));
     }

   if (screen_info->outputs)
     {
        EINA_LIST_FREE(screen_info->outputs, output_info)
          _output_info_free(output_info);
        free(eina_list_nth(screen_info->outputs, 0));
     }

   if (screen_info->modes)
     {
        EINA_LIST_FREE(screen_info->modes, mode_info)
          ecore_x_randr_mode_info_free(mode_info);
     }

   free(screen_info);
}

/*
 *********************************************
 *
 * Getter functions for e_randr_screen_info struct
 *
 * ********************************************
 */
Ecore_X_Randr_Mode_Info *
_12_screen_info_mode_info_get(const Ecore_X_Randr_Mode mode)
{
   Eina_List *iter;
   Ecore_X_Randr_Mode_Info *mode_info;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(E_RANDR_12_NO_MODE(mode), NULL);

   EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->modes, iter, mode_info)
     {
        if (mode_info && (mode_info->xid == mode)) return mode_info;
     }
   return NULL;
}

E_Randr_Output_Info *
_12_screen_info_output_info_get(const Ecore_X_Randr_Output output)
{
   Eina_List *iter;
   E_Randr_Output_Info *output_info;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(E_RANDR_12_NO_OUTPUTS, NULL);

   EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->outputs, iter, output_info)
     {
        if (output_info && (output_info->xid == output)) return output_info;
     }
   return NULL;
}

E_Randr_Crtc_Info *
_12_screen_info_crtc_info_get(const Ecore_X_Randr_Crtc crtc)
{
   Eina_List *iter;
   E_Randr_Crtc_Info *crtc_info;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(E_RANDR_12_NO_CRTCS, NULL);

   EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->crtcs, iter, crtc_info)
     {
        if (crtc_info && (crtc_info->xid == crtc)) return crtc_info;
     }
   return NULL;
}

Eina_Bool
_12_screen_info_edid_is_available(const E_Randr_Edid_Hash *hash)
{
   Eina_List *iter;
   E_Randr_Output_Info *output_info;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(E_RANDR_12_NO_OUTPUTS, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(hash, EINA_FALSE);

   EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->outputs, iter, output_info)
     {
        if (!output_info || !output_info->monitor)
          continue;
        if (output_info->monitor->edid_hash.hash == hash->hash)
          return EINA_TRUE;
     }
   return EINA_FALSE;
}

/*
 * returns a mode within a given list of modes that is gemetrically identical.
 * If none is found, NULL is returned.
 */
static Ecore_X_Randr_Mode_Info *
_mode_geo_identical_find(Eina_List *modes, Ecore_X_Randr_Mode_Info *mode)
{
   Eina_List *iter;
   Ecore_X_Randr_Mode_Info *mode_info;

   EINA_LIST_FOREACH(modes, iter, mode_info)
     {
        if ((mode_info->width == mode->width) && (mode_info->height == mode->height))
          return mode_info;
     }
   return NULL;
}

/*****************************************************************
 *
 * Init. and Shutdown code
 *
 *****************************************************************
 */
Eina_Bool
_12_screen_info_refresh(void)
{
   EINA_SAFETY_ON_TRUE_RETURN_VAL((e_randr_screen_info.randr_version < ECORE_X_RANDR_1_2), EINA_FALSE);

   if (e_randr_screen_info.rrvd_info.randr_info_12)
     _12_screen_info_free(e_randr_screen_info.rrvd_info.randr_info_12);
   if (!(e_randr_screen_info.rrvd_info.randr_info_12 = _screen_info_12_new()) ||
       !_structs_init())
     return EINA_FALSE;

   _screen_primary_output_assign(NULL);

   return EINA_TRUE;
}

/******************************************************************
 *
 * Event code
 *
 ******************************************************************
 */

static Eina_Bool
_x_poll_cb(void *data __UNUSED__)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(e_randr_screen_info.rrvd_info.randr_info_12, ECORE_CALLBACK_CANCEL);

   ecore_x_randr_screen_primary_output_orientations_get(e_randr_screen_info.root);

   return ECORE_CALLBACK_RENEW;
}

void
_12_event_listeners_add(void)
{
   EINA_SAFETY_ON_TRUE_RETURN(E_RANDR_12_NO);

   ecore_x_randr_events_select(e_randr_screen_info.root, EINA_TRUE);
   _event_handlers = eina_list_append(_event_handlers, ecore_event_handler_add(ECORE_X_EVENT_RANDR_CRTC_CHANGE, _crtc_change_event_cb, NULL));
   _event_handlers = eina_list_append(_event_handlers, ecore_event_handler_add(ECORE_X_EVENT_RANDR_OUTPUT_CHANGE, _output_change_event_cb, NULL));
   _event_handlers = eina_list_append(_event_handlers, ecore_event_handler_add(ECORE_X_EVENT_RANDR_OUTPUT_PROPERTY_NOTIFY, _output_property_change_event_cb, NULL));
   // WORKAROUND problem of X not sending events
   poller = ecore_poller_add(ECORE_POLLER_CORE, POLLINTERVAL, _x_poll_cb, NULL);
}

/* Usually events are triggered in the following order.
 * (Dis)connect Display Scenario:
 * 1.) ECORE_X_EVENT_OUTPUT_CHANGE //Triggered, when a display is connected to an
 * output
 * 2.) ECORE_X_EVENT_CRTC_CHANGE //Triggered when the CRTC mode is changed (eg.
 * enabled by e.g. e_randr or xrandr)
 * 3.) ECORE_X_EVENT_OUTPUT_CHANGE //Triggered for each output changed by the
 * preceeding enabling.
 *
 * When the mode of a CRTC is changed only events 2 and 3 are triggered
 *
 */
static Eina_Bool
_output_change_event_cb(void *data __UNUSED__, int type, void *ev)
{
   Ecore_X_Event_Randr_Output_Change *oce = (Ecore_X_Event_Randr_Output_Change *)ev;
   E_Randr_Output_Info *output_info = NULL;
   E_Randr_Crtc_Info *crtc_info = NULL;
   Eina_Bool policy_success = EINA_FALSE, con_state_changed = EINA_FALSE;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(E_RANDR_12_NO, ECORE_CALLBACK_CANCEL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL((type != ECORE_X_EVENT_RANDR_OUTPUT_CHANGE), ECORE_CALLBACK_RENEW);

   /* event information:
      Ecore_X_Window                  win;
      Ecore_X_Randr_Output            output;
      Ecore_X_Randr_Crtc              crtc;
      Ecore_X_Randr_Mode              mode;
      Ecore_X_Randr_Orientation       orientation;
      Ecore_X_Randr_Connection_Status connection;
      Ecore_X_Render_Subpixel_Order   subpixel_order;
    */

   EINA_SAFETY_ON_FALSE_RETURN_VAL((output_info = _12_screen_info_output_info_get(oce->output)), ECORE_CALLBACK_RENEW);

   DBG("E_RANDR: Output event: \n"
       "\t\t: relative to win: %d\n"
       "\t\t: output (xid): %d\n"
       "\t\t: used by crtc (xid): %d\n"
       "\t\t: mode: %d\n"
       "\t\t: orientation: %d\n"
       "\t\t: connection state: %s\n"
       "\t\t: subpixel_order: %d",
       oce->win,
       oce->output,
       oce->crtc,
       oce->mode,
       oce->orientation,
       _CONNECTION_STATES_STRINGS[oce->connection],
       oce->subpixel_order);

   crtc_info = _12_screen_info_crtc_info_get(oce->crtc);
   //WORKAROUND
   //Reason: Missing events, when an output is moved from one CRTC to
   //        another
   if (output_info->crtc && (crtc_info != output_info->crtc))
     output_info->crtc->outputs = eina_list_remove(output_info->crtc->outputs, output_info);
   //END WORKAROUND
   output_info->crtc = crtc_info;

   //Update mode references in case a mode was added manually
   if (output_info->monitor)
     {
        eina_list_free(output_info->monitor->modes);
        output_info->monitor->modes = NULL;
        eina_list_free(output_info->monitor->preferred_modes);
        output_info->monitor->preferred_modes = NULL;
        _monitor_modes_refs_set(output_info->monitor, output_info->xid);
        //Also update common modes of the used CRTC
        if (crtc_info && crtc_info->current_mode)
          {
             eina_list_free(crtc_info->outputs);
             crtc_info->outputs = NULL;
             eina_list_free(crtc_info->outputs_common_modes);
             crtc_info->outputs_common_modes = NULL;
             _crtc_outputs_refs_set(crtc_info);
          }
     }

   con_state_changed = (Eina_Bool)(output_info->connection_status != oce->connection);
   output_info->connection_status = oce->connection;
   output_info->subpixel_order = oce->subpixel_order;

   if (con_state_changed)
     {
        _monitor_info_free(output_info->monitor);
        output_info->monitor = NULL;

        if (oce->connection == ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED)
          {
             //New device connected!
             output_info->monitor = _monitor_info_new(output_info);
             INF("E_RANDR: Output %d was newly connected.", output_info->xid);

             //only try to enable the monitor if there is no serialized setup
             if (!_12_try_restore_configuration())
               {
                  policy_success = e_randr_12_try_enable_output(output_info, output_info->policy, EINA_FALSE);    //maybe give a success message?
                  INF("E_RANDR: Policy \"%s\" was enforced %ssuccesfully.", _POLICIES_STRINGS[output_info->policy - 1], (policy_success ? "" : "un"));
               }
          }
        else
          {
             //connection_state is 'unknown' or 'disconnected': treat as disconnected!
             if (output_info->crtc)
               {
                  output_info->crtc->outputs = eina_list_remove(output_info->crtc->outputs, output_info);
                  //in case this output was the last one connected on a CRTC,
                  //disable it again
                  if (eina_list_count(output_info->crtc->outputs) == 0)
                    {
                       //in case it was the only output running on this CRTC, disable
                       //it.
                       ecore_x_randr_crtc_mode_set(e_randr_screen_info.root, output_info->crtc->xid, NULL, Ecore_X_Randr_None, Ecore_X_Randr_None);
                    }
               }
             //retry to find a suiting serialized setup for the remaining
             //connected monitors
             _12_try_restore_configuration();
          }
     }

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_crtc_change_event_cb(void *data __UNUSED__, int type, void *ev)
{
   Ecore_X_Event_Randr_Crtc_Change *cce = (Ecore_X_Event_Randr_Crtc_Change *)ev;
   E_Randr_Crtc_Info *crtc_info;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(E_RANDR_12_NO, ECORE_CALLBACK_CANCEL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL((type != ECORE_X_EVENT_RANDR_CRTC_CHANGE), ECORE_CALLBACK_RENEW);

   /* event information:
      Ecore_X_Window                win;
      Ecore_X_Randr_Crtc            crtc;
      Ecore_X_Randr_Mode            mode;
      Ecore_X_Randr_Orientation     orientation;
      int                           x;
      int                           y;
      int                           width;
      int                           height;
    */
   DBG("E_RANDR: CRTC event: \n"
       "\t\t: relative to win: %d\n"
       "\t\t: crtc (xid): %d\n"
       "\t\t: mode (xid): %d\n"
       "\t\t: orientation: %d\n"
       "\t\t: x: %d\n"
       "\t\t: y: %d\n"
       "\t\t: width: %d\n"
       "\t\t: height: %d",
       cce->win,
       cce->crtc,
       cce->mode,
       cce->orientation,
       cce->geo.x,
       cce->geo.y,
       cce->geo.w,
       cce->geo.h);

   crtc_info = _12_screen_info_crtc_info_get(cce->crtc);
   EINA_SAFETY_ON_NULL_RETURN_VAL(crtc_info, ECORE_CALLBACK_RENEW);

   crtc_info->current_mode = _12_screen_info_mode_info_get(cce->mode);
   crtc_info->current_orientation = cce->orientation;
   crtc_info->geometry.x = cce->geo.x;
   crtc_info->geometry.y = cce->geo.y;
   crtc_info->geometry.w = cce->geo.w;
   crtc_info->geometry.h = cce->geo.h;

   //update screensize if necessary
   e_randr_screen_info.rrvd_info.randr_info_12->current_size.width = MAX((cce->geo.x + cce->geo.w), e_randr_screen_info.rrvd_info.randr_info_12->current_size.width);
   e_randr_screen_info.rrvd_info.randr_info_12->current_size.height = MAX((cce->geo.y + cce->geo.h), e_randr_screen_info.rrvd_info.randr_info_12->current_size.height);

   //update output data
   eina_list_free(crtc_info->outputs);
   crtc_info->outputs = NULL;
   eina_list_free(crtc_info->outputs_common_modes);
   crtc_info->outputs_common_modes = NULL;

   //if still enabled, update references to outputs
   if (crtc_info->current_mode)
     {
        eina_list_free(crtc_info->outputs);
        crtc_info->outputs = NULL;
        eina_list_free(crtc_info->outputs_common_modes);
        crtc_info->outputs_common_modes = NULL;
        _crtc_outputs_refs_set(crtc_info);
     }

   //crop the screen
   ecore_x_randr_screen_reset(e_randr_screen_info.root);

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_output_property_change_event_cb(void *data __UNUSED__, int type, void *ev)
{
   Ecore_X_Event_Randr_Output_Property_Notify *opce = (Ecore_X_Event_Randr_Output_Property_Notify *)ev;
   E_Randr_Output_Info *output_info;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(E_RANDR_12_NO, ECORE_CALLBACK_CANCEL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL((type != ECORE_X_EVENT_RANDR_OUTPUT_PROPERTY_NOTIFY), ECORE_CALLBACK_RENEW);

   /* event information:
      Ecore_X_Window                win;
      Ecore_X_Randr_Output          output;
      Ecore_X_Atom                  property;
      Ecore_X_Time                  time;
      Ecore_X_Randr_Property_Change state;
    */
   EINA_SAFETY_ON_FALSE_RETURN_VAL((output_info = _12_screen_info_output_info_get(opce->output)), ECORE_CALLBACK_RENEW);

   return ECORE_CALLBACK_RENEW;
}

/*
 * Try to enable this output on an unoccupied CRTC. 'Force' in this context
 * means, that if there are only occupied CRTCs, we disable another output to
 * enable this one. If not forced we will - if we don't find an unoccupied CRTC
 * - try to share the output of a CRTC with other outputs already using it
 *   (clone).
 */
EINTERN Eina_Bool
e_randr_12_try_enable_output(E_Randr_Output_Info *output_info, Ecore_X_Randr_Output_Policy policy, Eina_Bool force)
{
   Eina_List *iter, *outputs_list = NULL, *common_modes = NULL;
   E_Randr_Crtc_Info *crtc_info = NULL, *usable_crtc = NULL;
   const E_Randr_Crtc_Info *crtc_rel = NULL;
   E_Randr_Output_Info *primary_output;
   Ecore_X_Randr_Output *outputs;
   Ecore_X_Randr_Mode_Info *mode_info;
   int dx = Ecore_X_Randr_None, dy = Ecore_X_Randr_None;
   Eina_Bool ret = EINA_TRUE;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output_info, EINA_FALSE);
   EINA_SAFETY_ON_TRUE_RETURN_VAL((output_info->crtc && output_info->crtc->current_mode), EINA_FALSE);

   /*
    * Try to find a usable crtc for this output. Either unused or forced.
    */
   EINA_LIST_FOREACH(output_info->possible_crtcs, iter, crtc_info)
     {
        if (!crtc_info->current_mode || !crtc_info->outputs || force)
          {
             usable_crtc = crtc_info;
             break;
          }
     }

   /*
    * apparently we don't have a CRTC to make use of the device
    */
   if (!usable_crtc)
     return EINA_FALSE;

   //get the CRTC we will refer to, dependend on policy
   switch (policy)
     {
      case ECORE_X_RANDR_OUTPUT_POLICY_NONE:
        return EINA_TRUE;

      case ECORE_X_RANDR_OUTPUT_POLICY_ASK:
        e_randr_12_ask_dialog_new(output_info);
        return EINA_TRUE;  //This is a bit incorrect (dialog feedback is async), but probably not worth a lock.

      case ECORE_X_RANDR_OUTPUT_POLICY_CLONE:
        /*
         * Order of approaches to enable a clone (of the primary output):
         *
         * 0.  Get Primary output from Server
         * 1.  Try to add new Output to primary output's CRTC, using the mode used
         *     by the primary output
         * 2.  Try to enable clone in the same
         * 2a. exact mode or a
         * 2b. geometrically identical mode
         * 3.  Find a most high resolution mode in common to enable on primary output's CRTC and the new
         *     output's CRTC
         * 4.  fail.
         */
        //Assign new output, if necessary
        _screen_primary_output_assign(output_info);
        if ((primary_output = e_randr_screen_info.rrvd_info.randr_info_12->primary_output))
          {
             if (primary_output->crtc && primary_output->crtc->current_mode && eina_list_data_find(output_info->monitor->modes, primary_output->crtc->current_mode))
               {
                  /*
                   * mode currently used by primary output's CRTC is also supported by the new output
                   */
                  if (eina_list_data_find(primary_output->crtc->possible_outputs, output_info) && eina_list_data_find(output_info->monitor->modes, primary_output->crtc->current_mode))
                    {
                       /*
                        * 1.  Try to add new Output to primary output's CRTC, using the mode used
                        * by the primary output
                        * TODO: check with compatibility list in RandRR >= 1.3
                        * if available
                        *
                        * The new output is also usable by the primary output's
                        * CRTC. Try to enable this output together with the already
                        * enabled outputs on the CRTC in already used mode.
                        */
                       outputs_list = primary_output->crtc->outputs;
                       outputs_list = eina_list_append(outputs_list, output_info);
                       outputs = _outputs_to_array(outputs_list);
                       primary_output->crtc->outputs = NULL;
                       ret &= ecore_x_randr_crtc_mode_set(e_randr_screen_info.root, primary_output->crtc->xid, outputs, eina_list_count(outputs_list), primary_output->crtc->current_mode->xid);
                       free(outputs);
                       eina_list_free(outputs_list);
                       return ret;
                    }
                  else
                    {
                       /*
                        * 2.  Try to enable clone in the same
                        */

                       /*
                        * 2a. exact mode.
                        */
                       ret &= ecore_x_randr_crtc_mode_set(e_randr_screen_info.root, usable_crtc->xid, &output_info->xid, 1, primary_output->crtc->current_mode->xid);
                       ret &= ecore_x_randr_crtc_pos_relative_set(e_randr_screen_info.root, usable_crtc->xid, primary_output->crtc->xid, ECORE_X_RANDR_OUTPUT_POLICY_CLONE, e_randr_screen_info.rrvd_info.randr_info_12->alignment);
                       return ret;
                    }
               }
             else
               {
                  /*
                   * 2b. geometrically identical mode
                   */
                  if (primary_output->crtc && (mode_info = _mode_geo_identical_find(output_info->monitor->modes, primary_output->crtc->current_mode)))
                    {
                       ret &= ecore_x_randr_crtc_mode_set(e_randr_screen_info.root, usable_crtc->xid, &output_info->xid, 1, mode_info->xid);
                       ret &= ecore_x_randr_crtc_pos_relative_set(e_randr_screen_info.root, usable_crtc->xid, primary_output->crtc->xid, ECORE_X_RANDR_OUTPUT_POLICY_CLONE, e_randr_screen_info.rrvd_info.randr_info_12->alignment);
                       return ret;
                    }
                  /*
                   * 3.  Find the highest resolution mode common to enable on primary output's CRTC and the new one.
                   */
                  if (((outputs_list = eina_list_append(outputs_list, primary_output)) && (outputs_list = eina_list_append(outputs_list, output_info))))
                    {
                       if (primary_output->crtc)
                         {
                            common_modes = _outputs_common_modes_get(outputs_list, primary_output->crtc->current_mode);
                            if ((mode_info = eina_list_nth(common_modes, 0)))
                              {
                                 eina_list_free(common_modes);
                                 INF("Will try to set mode: %dx%d for primary and clone.", mode_info->width, mode_info->height);
                                 ret &= ecore_x_randr_crtc_mode_set(e_randr_screen_info.root, primary_output->crtc->xid, ((Ecore_X_Randr_Output *)Ecore_X_Randr_Unset), Ecore_X_Randr_Unset, mode_info->xid);
                                 ret &= ecore_x_randr_crtc_mode_set(e_randr_screen_info.root, usable_crtc->xid, &output_info->xid, 1, mode_info->xid);
                                 ret &= ecore_x_randr_crtc_pos_relative_set(e_randr_screen_info.root, usable_crtc->xid, primary_output->crtc->xid, ECORE_X_RANDR_OUTPUT_POLICY_CLONE, e_randr_screen_info.rrvd_info.randr_info_12->alignment);
                              }
                         }
                       eina_list_free(outputs_list);
                    }
               }
          }
        else
          ERR("E_RANDR: Failed to clone, because of missing or disabled primary output!");
        /*
         * 4. FAIL
         */
        break;

      default:
        //enable and position according to used policies
        if (!(mode_info = ((Ecore_X_Randr_Mode_Info *)eina_list_data_get(output_info->monitor->preferred_modes))))
          {
             ERR("E_RANDR: Could not enable output(%d), as it has no preferred modes (and there for none at all)!", output_info->xid);
             ret = EINA_FALSE;
             break;
          }

        //get the crtc we will place our's relative to. If it's NULL, this is the
        //only output attached, work done.
        if (!(crtc_rel = _crtc_according_to_policy_get(usable_crtc, policy)))
          {
             INF("E_RANDR: CRTC %d enabled. No other CRTC had to be moved.", usable_crtc->xid);
             ret &= ecore_x_randr_crtc_mode_set(e_randr_screen_info.root, usable_crtc->xid, &output_info->xid, 1, mode_info->xid);
             return ret;
          }

        //Calculate new CRTC's position according to policy
        switch (policy)
          {
           case ECORE_X_RANDR_OUTPUT_POLICY_ABOVE:
             usable_crtc->geometry.x = crtc_rel->geometry.x;
             usable_crtc->geometry.y = 0;
             break;

           case ECORE_X_RANDR_OUTPUT_POLICY_RIGHT:
             usable_crtc->geometry.x = (crtc_rel->geometry.x + crtc_rel->geometry.w);
             usable_crtc->geometry.y = crtc_rel->geometry.y;
             break;

           case ECORE_X_RANDR_OUTPUT_POLICY_BELOW:
             usable_crtc->geometry.x = crtc_rel->geometry.x;
             usable_crtc->geometry.y = (crtc_rel->geometry.y + crtc_rel->geometry.h);
             break;

           case ECORE_X_RANDR_OUTPUT_POLICY_LEFT:
             usable_crtc->geometry.y = crtc_rel->geometry.y;
             usable_crtc->geometry.x = 0;
             break;

           default:
             usable_crtc->geometry.y = 0;
             usable_crtc->geometry.x = 0;
          }

        if ((ret &= ecore_x_randr_crtc_settings_set(e_randr_screen_info.root, usable_crtc->xid, &output_info->xid, 1, usable_crtc->geometry.x, usable_crtc->geometry.y, mode_info->xid, ECORE_X_RANDR_ORIENTATION_ROT_0)))
          {
             //WORKAROUND
             //Reason: the CRTC event, that'd bring the new info about the set
             //mode is arriving too late here.
             usable_crtc->current_mode = mode_info;
             usable_crtc->geometry.w = mode_info->width;
             usable_crtc->geometry.h = mode_info->height;
             //WORKAROUND END

             INF("E_RANDR: Moved CRTC %d has geometry (x,y,wxh): %d, %d, %dx%d.", usable_crtc->xid, usable_crtc->geometry.x, usable_crtc->geometry.y, usable_crtc->geometry.w, usable_crtc->geometry.h);
             //following is policy dependend.
             switch (policy)
               {
                case ECORE_X_RANDR_OUTPUT_POLICY_ABOVE:
                  dy = (crtc_rel->geometry.y - usable_crtc->geometry.h);
                  if (dy < 0)
                    {
                       //virtual move (move other CRTCs as nessesary)
                       dy = -dy;
                       ret &= ecore_x_randr_move_all_crtcs_but(e_randr_screen_info.root,
                                                               &usable_crtc->xid,
                                                               1,
                                                               dx,
                                                               dy);
                       INF("E_RANDR: Moving all CRTCs but %d, by %dx%d delta.", usable_crtc->xid, dx, dy);
                    }
                  break;

                case ECORE_X_RANDR_OUTPUT_POLICY_LEFT:
                  dx = (crtc_rel->geometry.x - usable_crtc->geometry.w);
                  if (dx < 0)
                    {
                       //virtual move (move other CRTCs as nessesary)
                       dx = -dx;
                       ret &= ecore_x_randr_move_all_crtcs_but(e_randr_screen_info.root,
                                                               &usable_crtc->xid,
                                                               1,
                                                               dx,
                                                               dy);
                       INF("E_RANDR: Moving all CRTCs but %d, by %dx%d delta.", usable_crtc->xid, dx, dy);
                    }
                  break;

                default:
                  break;
               }
          }
     }

   if (ret)
     ecore_x_randr_screen_reset(e_randr_screen_info.root);

   return ret;
}

void
_12_event_listeners_remove(void)
{
   Ecore_Event_Handler *_event_handler = NULL;

   EINA_LIST_FREE(_event_handlers, _event_handler)
     ecore_event_handler_del(_event_handler);
   ecore_poller_del(poller);
   poller = NULL;
}

