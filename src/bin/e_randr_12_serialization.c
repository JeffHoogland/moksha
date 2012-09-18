#include "e_randr_private.h"
#include "e_randr.h"

#define MODE_STR_LENGTH_MAX 100

static const char *_POLICIES_STRINGS[] = {"ABOVE", "RIGHT", "BELOW", "LEFT", "CLONE", "NONE"};

static E_Randr_Serialized_Crtc  *_serialized_crtc_new(E_Randr_Crtc_Info *crtc_info);
static inline int                _sort_by_number_of_edids(const void *d1, const void *d2);
static inline Eina_List         *_find_matching_outputs(Eina_List *sois);
static inline E_Randr_Crtc_Info *_find_matching_crtc(E_Randr_Serialized_Crtc *sc);
static inline Ecore_X_Randr_Mode_Info *_find_matching_mode_info(Ecore_X_Randr_Mode_Info *mode);

/**********************************************************************
 *
 * Storage/Restorage of setups
 *
 **********************************************************************
 */

/*
 * Layout:
 *
 * Serialized_Setup_12 {
 *      - timestamp
 *      - List<Serialized_CRTC>
 *      - List<EDID>
 * }
 * Serialized_Crtc {
 *      - index
 *      - List<Serialized_Output>
 *      - pos
 *      - orientation
 *      - mode
 * }
 * Serialized_Output {
 *      - name
 *      - name_length
 *      - serialized_edid
 *      - backlight_level
 * }
 * Serialized_EDID {
 *      - edid_hash
 * }
 *
 */
//"Free" helper functions

void
_serialized_output_free(E_Randr_Serialized_Output *so)
{
   EINA_SAFETY_ON_NULL_RETURN(so);

   eina_stringshare_del(so->name);

   free(so);
}

void
_serialized_output_policy_free(E_Randr_Serialized_Output_Policy *sop)
{
   EINA_SAFETY_ON_NULL_RETURN(sop);

   eina_stringshare_del(sop->name);
   free(sop);
}

EINTERN void
e_randr_12_serialized_output_policy_free(E_Randr_Serialized_Output_Policy *policy)
{
   _serialized_output_policy_free(policy);
}

void
_mode_info_free(Ecore_X_Randr_Mode_Info *mode_info)
{
   EINA_SAFETY_ON_NULL_RETURN(mode_info);

   eina_stringshare_del(mode_info->name);
   free(mode_info);
}

void
_serialized_crtc_free(E_Randr_Serialized_Crtc *sc)
{
   E_Randr_Serialized_Output *so;

   EINA_SAFETY_ON_NULL_RETURN(sc);

   EINA_LIST_FREE (sc->outputs, so)
      _serialized_output_free(so);
   _mode_info_free(sc->mode_info);
   free(sc);
}

//"New" helper functions

/**
 * @brief Seeks given data in the list and returns the index
 * of the first element equaling @data
 * @param list The list to be examined
 * @param data The data to be found
 * @return if found, the index of the list node. Else -1.
 */
int
_eina_list_data_index_get(const Eina_List *list, const void *data)
{
   Eina_List *iter;
   void *ndata;
   int i = 0;

   EINA_LIST_REVERSE_FOREACH(list, iter, ndata)
     {
        if (ndata == data)
          return i;
        else
          i++;
     }

   return -1;
}

Ecore_X_Randr_Mode_Info
*_mode_info_clone(const Ecore_X_Randr_Mode_Info *src)
{
   Ecore_X_Randr_Mode_Info *mi = NULL;

   EINA_SAFETY_ON_NULL_RETURN_VAL(src, NULL);

   mi = E_NEW(Ecore_X_Randr_Mode_Info, 1);

   mi->xid = src->xid;
   mi->width = src->width;
   mi->height = src->height;
   mi->dotClock = src->dotClock;
   mi->hSyncStart = src->hSyncStart;
   mi->hSyncEnd = src->hSyncEnd;
   mi->hTotal = src->hTotal;
   mi->hSkew = src->hSkew;
   mi->vSyncStart = src->vSyncStart;
   mi->vSyncEnd = src->vSyncEnd;
   mi->vTotal = src->vTotal;
   if (src->nameLength > 0)
     {
        mi->name = (char*)eina_stringshare_add(src->name);
     }
   mi->nameLength = src->nameLength;
   mi->modeFlags = src->modeFlags;

   return mi;
}

E_Randr_Edid_Hash
*_monitor_edid_hash_clone(E_Randr_Monitor_Info *mi)
{
   E_Randr_Edid_Hash *edid_hash;

   EINA_SAFETY_ON_NULL_RETURN_VAL(mi, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL((mi->edid_hash.hash == 0), NULL);
   edid_hash = malloc(sizeof(E_Randr_Edid_Hash));

   edid_hash->hash = mi->edid_hash.hash;

   return edid_hash;
}

Eina_List *
_outputs_policies_list_new(Eina_List *outputs)
{
   E_Randr_Serialized_Output_Policy *sop;
   Eina_List *iter, *list = NULL;
   E_Randr_Output_Info *oi;

   EINA_SAFETY_ON_NULL_RETURN_VAL(outputs, NULL);

   EINA_LIST_FOREACH(outputs, iter, oi)
     {
        if (!oi->name) continue;

        sop = E_NEW(E_Randr_Serialized_Output_Policy, 1);
        sop->name = eina_stringshare_add(oi->name);
        sop->policy = oi->policy;
        list = eina_list_append(list, sop);
     }

   return list;
}

E_Randr_Serialized_Output *
_serialized_output_new(E_Randr_Output_Info *output_info)
{
   E_Randr_Serialized_Output *so;

   EINA_SAFETY_ON_NULL_RETURN_VAL(output_info, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(output_info->name, NULL);

   so = E_NEW(E_Randr_Serialized_Output, 1);

   so->name = eina_stringshare_add(output_info->name);
   if (output_info->monitor)
     {
        so->backlight_level = output_info->monitor->backlight_level;
     }
   else
     {
        so->backlight_level = Ecore_X_Randr_Unset;
     }

   return so;
}

E_Randr_Serialized_Crtc *
_serialized_crtc_new(E_Randr_Crtc_Info *crtc_info)
{
   E_Randr_Serialized_Crtc *sc = NULL;
   E_Randr_Serialized_Output *so = NULL;
   E_Randr_Output_Info *output_info = NULL;
   Eina_List *iter;

   EINA_SAFETY_ON_NULL_RETURN_VAL(crtc_info, NULL);

   sc = E_NEW(E_Randr_Serialized_Crtc, 1);

   //Get relative index of CRTC
   sc->index = _eina_list_data_index_get(e_randr_screen_info.rrvd_info.randr_info_12->crtcs, crtc_info);

   //Create list of serialized outputs
   EINA_LIST_FOREACH(crtc_info->outputs, iter, output_info)
     {
        if (!(so = _serialized_output_new(output_info)))
          continue;
        sc->outputs = eina_list_append(sc->outputs, so);
        INF("E_RANDR:\t Serialized output %s.", so->name);
     }
   sc->pos.x = crtc_info->geometry.x;
   sc->pos.y = crtc_info->geometry.y;
   sc->orientation = crtc_info->current_orientation;
   //Clone mode
   sc->mode_info = _mode_info_clone(crtc_info->current_mode);

   return sc;
}

E_Randr_Serialized_Setup_12 *
_12_serialized_setup_new(void)
{
   E_Randr_Serialized_Setup_12 *ss = NULL;
   Eina_List *iter;
   E_Randr_Crtc_Info *ci;
   E_Randr_Output_Info *oi;
   E_Randr_Serialized_Crtc *sc;
   E_Randr_Edid_Hash *edid_hash;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(E_RANDR_12_NO, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(e_randr_screen_info.rrvd_info.randr_info_12->outputs, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(e_randr_screen_info.rrvd_info.randr_info_12->crtcs, NULL);

   ss = E_NEW(E_Randr_Serialized_Setup_12, 1);

   ss->timestamp = ecore_time_get();

   //Add CRTCs and their configuration
   EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->crtcs, iter, ci)
     {
        sc = _serialized_crtc_new(ci);
        if (!sc)
          continue;
        ss->crtcs = eina_list_append(ss->crtcs, sc);
        INF("E_RANDR: Serialized CRTC %d (index %d) in mode %s.", ci->xid, sc->index, (sc->mode_info ? sc->mode_info->name : "(disabled)"));
     }

   /*
    * Add EDID hashes of connected outputs
    * for easier comparison during
    * setup restoration
    */
   EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->outputs, iter, oi)
     {
        if (oi->connection_status != ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED)
          continue;
        edid_hash = _monitor_edid_hash_clone(oi->monitor);
        ss->edid_hashes = eina_list_append(ss->edid_hashes, edid_hash);
     }

   return ss;
}

//Update (also retrieval) helper functions

E_Randr_Serialized_Setup_12 *
_matching_serialized_setup_get(Eina_List *setups_12)
{
   E_Randr_Serialized_Setup_12 *ss_12;
   Eina_List *setups_iter, *edid_iter;
   E_Randr_Edid_Hash *edid_hash;

   EINA_SAFETY_ON_TRUE_RETURN_VAL(E_RANDR_12_NO, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(setups_12, NULL);

   //Sort list of setups by the number of monitors involved
   setups_12 = eina_list_sort(setups_12, 0, _sort_by_number_of_edids);

   EINA_LIST_FOREACH(setups_12, setups_iter, ss_12)
     {
        //1. Make sure: #outputs >= #serialized EDIDs
        if (eina_list_count(e_randr_screen_info.rrvd_info.randr_info_12->outputs) < eina_list_count(ss_12->edid_hashes))
          continue;
        //2. Compare #CRTCs
        if (eina_list_count(e_randr_screen_info.rrvd_info.randr_info_12->crtcs) != eina_list_count(ss_12->crtcs))
          continue;

        //3. Find all serialized EDIDs
        EINA_LIST_FOREACH(ss_12->edid_hashes, edid_iter, edid_hash)
          {
             if (!_12_screen_info_edid_is_available(edid_hash))
               goto _setup_12_skip;
          }

        //4. All EDIDs found? Great, let's go!
        return ss_12;
_setup_12_skip:
        continue;
     }

   // None found!
   return NULL;
}

Eina_List *
_outputs_policies_update(Eina_List *sops)
{
   E_Randr_Serialized_Output_Policy *sop;

   EINA_LIST_FREE (sops, sop)
     {
        _serialized_output_policy_free(sop);
     }

   return _outputs_policies_list_new(e_randr_screen_info.rrvd_info.randr_info_12->outputs);
}

Eina_List *
_12_serialized_setup_update(Eina_List *setups_12)
{
   E_Randr_Serialized_Setup_12 *ss_12;

   if (setups_12)
     {
        /*
         * try to find the setup with the same monitors
         * connected in order to replace it
         */
        if ((ss_12 = _matching_serialized_setup_get(setups_12)))
          {
             INF("E_RANDR: Found stored configuration that matches current setup. It was created at %f. Freeing it...", ss_12->timestamp);
             _12_serialized_setup_free(ss_12);
             setups_12 = eina_list_remove(setups_12, ss_12);
          }
     }
   ss_12 = _12_serialized_setup_new();
   setups_12 = eina_list_append(setups_12, ss_12);

   return setups_12;
}

void
_12_policies_restore(void)
{
   E_Randr_Output_Info *output;
   E_Randr_Serialized_Output_Policy *sop;
   Eina_List *iter, *iter2;

   EINA_SAFETY_ON_TRUE_RETURN(E_RANDR_12_NO);
   EINA_SAFETY_ON_NULL_RETURN(e_config->randr_serialized_setup);
   EINA_SAFETY_ON_NULL_RETURN(e_config->randr_serialized_setup->outputs_policies);

   // Restore policies
   EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->outputs, iter, output)
     {
        EINA_LIST_FOREACH(e_config->randr_serialized_setup->outputs_policies, iter2, sop)
          {
             if (!strncmp(sop->name, output->name, output->name_length))
               {
                  output->policy = sop->policy;
                  INF("E_RANDR: Policy \"%s\" for output \"%s\" restored.", _POLICIES_STRINGS[sop->policy - 1], output->name);
               }
          }
     }
}

Eina_Bool
_12_try_restore_configuration(void)
{
   E_Randr_Serialized_Setup_12 *ss_12;
   E_Randr_Serialized_Crtc *sc;
   E_Randr_Crtc_Info *ci;
   Ecore_X_Randr_Output *outputs_array;
   E_Randr_Output_Info *output_info;
   Ecore_X_Randr_Mode_Info *mi = NULL;
   Ecore_X_Randr_Mode mode = 0;
   Eina_List *iter, *outputs_list, *outputs_iter;
   Eina_Bool ret = EINA_TRUE;

   EINA_SAFETY_ON_NULL_RETURN_VAL(e_config, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(e_config->randr_serialized_setup, EINA_FALSE);

   if (!(ss_12 = _matching_serialized_setup_get(e_config->randr_serialized_setup->serialized_setups_12)))
     return EINA_FALSE;

   INF("E_RANDR: Found matching serialized setup.");
   EINA_LIST_FOREACH(ss_12->crtcs, iter, sc)
     {
        ci = _find_matching_crtc(sc);
        if (!ci)
          {
             ERR("E_RANDR: Cannot find a matching CRTC for serialized CRTC index %d.", sc->index);
             return EINA_FALSE;
          }
        outputs_list = _find_matching_outputs(sc->outputs);
        outputs_array = _outputs_to_array(outputs_list);

        if (!sc->mode_info)
          {
             INF("E_RANDR: \tSerialized mode was disabled.");
             mode = Ecore_X_Randr_None;
          }
        else if ((mi = _find_matching_mode_info(sc->mode_info)))
          {
             INF("E_RANDR: \tSerialized mode is now known under the name %s.", mi->name);
             mode = mi->xid;
          }
        else if (mi) /* FIXME: this is impossible, so whoever wrote it probably meant something else */
          {
             // The serialized mode is no longer available
             mi->name = malloc(MODE_STR_LENGTH_MAX);
             //IMPROVABLE: Use random string, like mktemp for files
             snprintf(mi->name, (MODE_STR_LENGTH_MAX - 1), "%ux%u,%lu,%lu", sc->mode_info->width, sc->mode_info->height, sc->mode_info->dotClock, sc->mode_info->modeFlags);
             mi = sc->mode_info;
             mode = ecore_x_randr_mode_info_add(e_randr_screen_info.root, mi);
             if (mode == Ecore_X_Randr_None)
               {
                  eina_list_free(outputs_list);
                  free(outputs_array);
                  continue;
               }
             EINA_LIST_FOREACH(outputs_list, outputs_iter, output_info)
                ecore_x_randr_output_mode_add(output_info->xid, mode);
             INF("E_RANDR: \tSerialized mode was added to the server manually using the name %s.", mi->name);
          }

        // DEBUG
        if (mi)
          DBG("E_RANDR: \tRestoring CRTC %d (index %d) in mode %s.", ci->xid, sc->index, (mode == Ecore_X_Randr_None) ? "(disabled)" : mi->name);
        DBG("E_RANDR: \t\tUsed outputs:");
        EINA_LIST_FOREACH(outputs_list, outputs_iter, output_info)
            DBG("\t\t%s", output_info->name);
        // DEBUG END

        ret &= ecore_x_randr_crtc_settings_set(e_randr_screen_info.root, ci->xid, outputs_array, eina_list_count(outputs_list), sc->pos.x, sc->pos.y, mode, sc->orientation);
        eina_list_free(outputs_list);
        free(outputs_array);
     }
   return ret;
}

void
_12_serialized_setup_free(E_Randr_Serialized_Setup_12 *ss_12)
{
   E_Randr_Serialized_Crtc *sc;
   E_Randr_Edid_Hash *edid_hash;

   if (!ss_12) return;

   EINA_LIST_FREE (ss_12->crtcs, sc)
     {
        _serialized_crtc_free(sc);
     }
   EINA_LIST_FREE (ss_12->edid_hashes, edid_hash)
      free(edid_hash);

   free(ss_12);
}

EINTERN void
e_randr_12_serialized_setup_free(E_Randr_Serialized_Setup_12 *ss_12)
{
    _12_serialized_setup_free(ss_12);
}

void
_12_store_configuration(E_Randr_Configuration_Store_Modifier modifier)
{
   if (modifier & (E_RANDR_CONFIGURATION_STORE_RESOLUTIONS | E_RANDR_CONFIGURATION_STORE_ARRANGEMENT | E_RANDR_CONFIGURATION_STORE_ORIENTATIONS))
     {
          e_config->randr_serialized_setup->serialized_setups_12 = _12_serialized_setup_update(e_config->randr_serialized_setup->serialized_setups_12);
     }

   if (modifier & E_RANDR_CONFIGURATION_STORE_POLICIES)
     {
        //update output policies
        e_config->randr_serialized_setup->outputs_policies = _outputs_policies_update(e_config->randr_serialized_setup->outputs_policies);
     }
}

//Retrievel functions for the current e_randr_screen_info context

// Find entities for restoration in current e_randr_screen_info context
static E_Randr_Crtc_Info *
_find_matching_crtc(E_Randr_Serialized_Crtc *sc)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(sc, NULL);
   EINA_SAFETY_ON_TRUE_RETURN_VAL(E_RANDR_12_NO, NULL);

   INF("E_RANDR: Setup restore.. Runtime system knows about %d CRTCs. Requested CRTC has index %d", eina_list_count(e_randr_screen_info.rrvd_info.randr_info_12->crtcs), sc->index);
   return eina_list_nth(e_randr_screen_info.rrvd_info.randr_info_12->crtcs, sc->index);
}

/**
 * @brief Creates list of E_Randr_Output_Info* elements for list of
 * E_Randr_Serialized_Output* objects.
 * @param sois list of E_Randr_Serialized_Output* elements
 * @return List of E_Randr_Output* elements or NULL, if not all outputs could be
 * found or monitors are connected to different outputs
 */
static Eina_List *
_find_matching_outputs(Eina_List *sois)
{
   Eina_List *r_output_iter, *s_output_iter, *list = NULL;
   E_Randr_Output_Info *oi;
   E_Randr_Serialized_Output *so;

   EINA_LIST_FOREACH(sois, s_output_iter, so)
     {
        INF("E_RANDR: \tLooking for serialized output \"%s\"", so->name);
        EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->outputs, r_output_iter, oi)
          {
             INF("E_RANDR: \t\tComparing to output \"%s\"", oi->name);
             if (!strncmp(so->name, oi->name, oi->name_length))
               {

                  list = eina_list_append(list, oi);
                  break;
               }
          }
     }
   if (list && (eina_list_count(sois) != eina_list_count(list)))
     {
        eina_list_free(list);
        list = NULL;
     }

   return list;
}

static Ecore_X_Randr_Mode_Info *
_find_matching_mode_info(Ecore_X_Randr_Mode_Info *mode)
{
   Eina_List *iter;
   Ecore_X_Randr_Mode_Info *mi = NULL;

   EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->modes, iter, mi)
     {
        if (!strncmp(mode->name, mi->name, mode->nameLength))
          return mi;
     }
   EINA_LIST_FOREACH(e_randr_screen_info.rrvd_info.randr_info_12->modes, iter, mi)
     {
#define EQL(arg) (mi->arg == mode->arg)
         if (EQL(width) &&
                 EQL(height) &&
                 EQL(dotClock) &&
                 EQL(hSyncStart) &&
                 EQL(hSyncEnd) &&
                 EQL(hTotal) &&
                 EQL(hSkew) &&
                 EQL(vSyncStart) &&
                 EQL(vSyncEnd) &&
                 EQL(vTotal) &&
                 EQL(modeFlags))
         return mi;
#undef EQL
     }
   return NULL;
}

static int
_sort_by_number_of_edids(const void *d1, const void *d2)
{
    const E_Randr_Serialized_Setup_12 *ss1 = (const E_Randr_Serialized_Setup_12*)d1;
    const E_Randr_Serialized_Setup_12 *ss2 = (const E_Randr_Serialized_Setup_12*)d2;

   EINA_SAFETY_ON_NULL_RETURN_VAL(ss1, 1);
   EINA_SAFETY_ON_NULL_RETURN_VAL(ss2, -1);

   if (eina_list_count(ss2->edid_hashes) > eina_list_count(ss1->edid_hashes))
     return 1;
   else
     return -1;
}
