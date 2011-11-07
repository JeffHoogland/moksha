/*
 * vim:ts=8:sw=3:sts=8:expandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define ECORE_X_RANDR_1_1 ((1 << 16) | 1)
#define ECORE_X_RANDR_1_2 ((1 << 16) | 2)
#define ECORE_X_RANDR_1_3 ((1 << 16) | 3)

#define Ecore_X_Randr_None   0
#define Ecore_X_Randr_Unset -1

/*
 * Save mechanism:
 * Single monitor:
 * - Save monitor using the resolution
 *
 * Multiple monitors:
 * - Use the EDID information to make sure we restore the right monitor.
 *
 * TODO:
 * -fix output policies above and left
 */

//following macro namescheme follows cardinal relation
//1 : M
#define E_RANDR_NO_SCREEN_RET(ret) if (!e_randr_screen_info) return ret
#define E_RANDR_NO_11_RET(ret) if (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_1) || !e_randr_screen_info->rrvd_info.randr_info_11) return ret
#define E_RANDR_NO_12_RET(ret) if (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2) || !e_randr_screen_info->rrvd_info.randr_info_12) return ret
#define E_RANDR_NO_CRTCS_RET(ret) if (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2) || !e_randr_screen_info->rrvd_info.randr_info_12 || !e_randr_screen_info->rrvd_info.randr_info_12->crtcs) return ret
#define E_RANDR_NO_OUTPUTS_RET(ret) if (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2) || !e_randr_screen_info->rrvd_info.randr_info_12 || !e_randr_screen_info->rrvd_info.randr_info_12->outputs) return ret
#define E_RANDR_NO_CRTC_RET(crtc, ret) if (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2) || !e_randr_screen_info->rrvd_info.randr_info_12 || !e_randr_screen_info->rrvd_info.randr_info_12->crtcs || !crtc) return ret
#define E_RANDR_NO_OUTPUT_RET(output, ret) if (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2) || !e_randr_screen_info->rrvd_info.randr_info_12 || !e_randr_screen_info->rrvd_info.randr_info_12->outputs || !output) return ret
#define E_RANDR_NO_MODE_RET(mode, ret) if (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2) || !e_randr_screen_info->rrvd_info.randr_info_12 || !e_randr_screen_info->rrvd_info.randr_info_12->modes || !mode) return ret
#define E_RANDR_NO_CRTC_OUTPUT_RET(crtc, output, ret) if (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2) || !e_randr_screen_info->rrvd_info.randr_info_12 || !e_randr_screen_info->rrvd_info.randr_info_12->crtcs || !crtc || !e_randr_screen_info->rrvd_info.randr_info_12->outputs || !output) return ret
#define E_RANDR_NO_CRTC_OUTPUT_MODE_RET(crtc, output, mode, ret) if (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2) || !e_randr_screen_info->rrvd_info.randr_info_12 || !e_randr_screen_info->rrvd_info.randr_info_12->crtcs || !crtc || !e_randr_screen_info->rrvd_info.randr_info_12->outputs || !output || !e_randr_screen_info->rrvd_info.randr_info_12->modes || !mode) return ret

#define E_RANDR_NO_11 (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_1) || !e_randr_screen_info->rrvd_info.randr_info_11)
#define E_RANDR_NO_12 (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2) || !e_randr_screen_info->rrvd_info.randr_info_12)
#define E_RANDR_NO_CRTCS (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2) || !e_randr_screen_info->rrvd_info.randr_info_12 || !e_randr_screen_info->rrvd_info.randr_info_12->crtcs)
#define E_RANDR_NO_OUTPUTS (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2) || !e_randr_screen_info->rrvd_info.randr_info_12 || !e_randr_screen_info->rrvd_info.randr_info_12->outputs)
#define E_RANDR_NO_MODES (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2) || !e_randr_screen_info->rrvd_info.randr_info_12 || !e_randr_screen_info->rrvd_info.randr_info_12->modes)
#define E_RANDR_NO_CRTC(crtc) (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2) || !e_randr_screen_info->rrvd_info.randr_info_12 || !e_randr_screen_info->rrvd_info.randr_info_12->crtcs || !crtc)
#define E_RANDR_NO_OUTPUT(output) (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2) || !e_randr_screen_info->rrvd_info.randr_info_12 || !e_randr_screen_info->rrvd_info.randr_info_12->outputs || !output)
#define E_RANDR_NO_CRTC_OUTPUT(crtc, output) (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2) || !e_randr_screen_info->rrvd_info.randr_info_12 || !e_randr_screen_info->rrvd_info.randr_info_12->crtcs || !crtc || !e_randr_screen_info->rrvd_info.randr_info_12->outputs || !output)
#define E_RANDR_NO_CRTC_OUTPUT_MODE(crtc, output, mode) (!e_randr_screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2) || !e_randr_screen_info->rrvd_info.randr_info_12 || !e_randr_screen_info->rrvd_info.randr_info_12->crtcs || !crtc || !e_randr_screen_info->rrvd_info.randr_info_12->outputs || !output || !e_randr_screen_info->rrvd_info.randr_info_12->modes || !mode)

static Eina_Bool _e_randr_init(void);
static void _e_randr_shutdown(void);
static void _e_randr_event_listeners_add(void);
static void _e_randr_event_listeners_remove(void);
static Eina_Bool _e_randr_event_cb(void *data, int type, void *e);
static E_Randr_Screen_Info *_e_randr_screen_info_new(void);
static void _e_randr_screen_info_free(E_Randr_Screen_Info *screen_info);
static E_Randr_Screen_Info_11 *_e_randr_screen_info_11_new(void);
static Eina_Bool _e_randr_screen_info_11_set(void);
static void _e_randr_screen_info_11_free(E_Randr_Screen_Info_11 *screen_info_11);
static E_Randr_Screen_Info_12 *_e_randr_screen_info_12_new(void);
static Eina_Bool _e_randr_screen_info_12_set(E_Randr_Screen_Info_12 *screen_info_12);
static void _e_randr_screen_info_12_free(E_Randr_Screen_Info_12 *screen_info_12);
static E_Randr_Output_Info *_e_randr_output_info_new(int nrequested);
static void _e_randr_output_info_free(E_Randr_Output_Info *output_info);
static E_Randr_Crtc_Info *_e_randr_crtc_info_new(int nrequested);
static void _e_randr_crtc_info_free(E_Randr_Crtc_Info *crtc_info);
static Eina_Bool _e_randr_screen_outputs_init(void);
static Eina_Bool _e_randr_screen_crtcs_init(void);

static Eina_Bool _e_randr_try_restore_11(E_Randr_Screen_Info_11 *si_11);
static Eina_Bool _e_randr_try_restore_12(E_Randr_Screen_Info_12 *si_12);
EAPI void e_randr_store_configuration(E_Randr_Screen_Info *screen_info);
EAPI Eina_Bool e_randr_try_restore_configuration(E_Randr_Screen_Info *screen_info);
static Eina_Bool _e_randr_output_modes_add(E_Randr_Output_Info *output_info);
static void _e_randr_notify_crtc_mode_change(E_Randr_Crtc_Info *crtc_info);
static void _e_randr_notify_output_change(E_Randr_Output_Info *output_info);
static Ecore_X_Randr_Mode_Info *_e_randr_mode_info_get(Ecore_X_Randr_Mode mode);
static E_Randr_Crtc_Info *_e_randr_crtc_info_get(Ecore_X_Randr_Crtc crtc);
static E_Randr_Output_Info *_e_randr_output_info_get(Ecore_X_Randr_Output output);
static void _e_randr_output_info_set(E_Randr_Output_Info *output_info);
static void _e_randr_crtc_info_set(E_Randr_Crtc_Info *crtc_info);
static const E_Randr_Crtc_Info *_e_randr_policy_crtc_get(E_Randr_Crtc_Info* but, E_Randr_Crtc_Info *hint, Ecore_X_Randr_Output_Policy policy);
static Ecore_X_Randr_Output *_e_randr_outputs_to_array(Eina_List *outputs_info);
static Eina_Bool _e_randr_try_enable_output(E_Randr_Output_Info *output_info, Eina_Bool force);
static void _e_randr_crtcs_possible_output_update(E_Randr_Output_Info *output_info);
static void _e_randr_crtc_outputs_refs_update(E_Randr_Crtc_Info *crtc_info);
static Eina_Bool _e_randr_crtc_move_policy(E_Randr_Crtc_Info *new_crtc);
//static int _crtcs_size_sort_cb(const void *d1, const void *d2);
static int _outputs_size_sort_cb(const void *d1, const void *d2);
static int _modes_size_sort_cb(const void *d1, const void *d2);
static Eina_List *_e_randr_outputs_common_modes_get(Eina_List *outputs, Ecore_X_Randr_Mode_Info *max_size_mode);
static Ecore_X_Randr_Mode_Info *_e_randr_outputs_common_mode_max_get(Eina_List *outputs, Ecore_X_Randr_Mode_Info *max_size_mode);
static Ecore_X_Randr_Mode_Info *_e_randr_mode_geo_identical_find(Eina_List *modes, Ecore_X_Randr_Mode_Info *mode);
static Eina_Bool _e_randr_crtc_mode_intersects_crtcs(E_Randr_Crtc_Info *crtc_info, Ecore_X_Randr_Mode_Info *mode);
static Eina_Bool _e_randr_crtc_outputs_mode_max_set(E_Randr_Crtc_Info *crtc_info);
static Eina_Bool _e_randr_crtcs_clone_crtc_removed(E_Randr_Crtc_Info *former_clone);
static void _e_randr_screen_primary_output_assign(E_Randr_Output_Info *removed);
static void _e_randr_output_info_hw_info_set(E_Randr_Output_Info *output_info);
static void _e_randr_output_hw_info_free(E_Randr_Output_Info *output_info);
static Eina_Bool _e_randr_outputs_are_clones(E_Randr_Output_Info *output_info, Eina_List *outputs);

EAPI E_Randr_Screen_Info *e_randr_screen_info = NULL;
static Eina_List *_e_randr_event_handlers = NULL;

EINTERN Eina_Bool
e_randr_init(void)
{
   return _e_randr_init();
}

EINTERN int
e_randr_shutdown(void)
{
   _e_randr_shutdown();
   return 1;
}

static Eina_Bool
_e_randr_init(void)
{
   int n;
   Ecore_X_Window *roots;
   Ecore_X_Window root;

   if(!(roots = ecore_x_window_root_list(&n))) return EINA_FALSE;
   /* first (and only) root window */
   root = roots[0];
   free(roots);

   if ((!ecore_x_randr_query()) || 
       !(e_randr_screen_info = _e_randr_screen_info_new()))
     goto ecore_x_randr_init_fail_free_screen;

   if ((e_randr_screen_info->randr_version = ecore_x_randr_version_get())) 
     e_randr_screen_info->root = root;
   if (e_randr_screen_info->randr_version == ECORE_X_RANDR_1_1)
     {
        if (!(e_randr_screen_info->rrvd_info.randr_info_11 = _e_randr_screen_info_11_new())) 
          goto ecore_x_randr_init_fail_free_screen;
        _e_randr_screen_info_11_set();
        //_e_randr_config_find_and_enable();
        return EINA_TRUE;
     }
   else if (e_randr_screen_info->randr_version >= ECORE_X_RANDR_1_2)
     {
        if (!(e_randr_screen_info->rrvd_info.randr_info_12 =  _e_randr_screen_info_12_new())) 
          goto ecore_x_randr_init_fail_free_screen;
        _e_randr_screen_info_12_set(e_randr_screen_info->rrvd_info.randr_info_12);
        _e_randr_event_listeners_add();
        if (!_e_randr_screen_outputs_init()) 
          goto ecore_x_randr_init_fail_free_screen;
        if (!_e_randr_screen_crtcs_init()) 
          goto ecore_x_randr_init_fail_free_screen;
        _e_randr_screen_primary_output_assign(NULL);
        return EINA_TRUE;
     }

   //FILO free stack in case we fail to allocate something/can't get hold of
   //necessary information
ecore_x_randr_init_fail_free_screen:
   if (e_randr_screen_info) 
     _e_randr_screen_info_free(e_randr_screen_info);

   return EINA_FALSE;
}

static void
_e_randr_shutdown(void)
{
   _e_randr_screen_info_free(e_randr_screen_info);
}

/**
 * @param nrequeste
 * @return Instance of E_Randr_Screen_Info or if memory couldn't be
 * allocated NULL.
 */
static E_Randr_Screen_Info *
_e_randr_screen_info_new(void)
{
   E_Randr_Screen_Info *ret = NULL;
   E_Randr_Screen_Info default_info =
     {
        .root = Ecore_X_Randr_Unset,
        .randr_version = Ecore_X_Randr_None,
        .rrvd_info.randr_info_11 = NULL
     };

   if (!(ret = malloc(sizeof(E_Randr_Screen_Info)))) return NULL;

   memcpy(ret, &default_info, sizeof(default_info));

   return ret;
}

/**
 * @param screen_info the screen info to free.
 */
static void
_e_randr_screen_info_free(E_Randr_Screen_Info *screen_info)
{
   if ((!screen_info) || !(screen_info->rrvd_info.randr_info_11)) return;
   switch (e_randr_screen_info->randr_version)
     {
      case ECORE_X_RANDR_1_1:
        _e_randr_screen_info_11_free(screen_info->rrvd_info.randr_info_11);
        break;
      case ECORE_X_RANDR_1_2:
      case ECORE_X_RANDR_1_3:
        _e_randr_screen_info_12_free(screen_info->rrvd_info.randr_info_12);
        break;
     }
   free(screen_info);
   screen_info = NULL;
}

/**
 * @return array of E_Randr_Screen_Info_11 elements, or in case not all could
 * be created or parameter 'nrequested'==0, NULL
 */
static E_Randr_Screen_Info_11 *
_e_randr_screen_info_11_new(void)
{
   E_Randr_Screen_Info_11 *ret = NULL;
   static const E_Randr_Screen_Info_11 default_info =
     {
        .sizes = NULL,
        .csize_index = Ecore_X_Randr_Unset,
        .corientation = Ecore_X_Randr_Unset,
        .orientations = Ecore_X_Randr_Unset,
        .rates = NULL,
        .current_rate = Ecore_X_Randr_Unset
     };

   if (!(ret = malloc(sizeof(E_Randr_Screen_Info_11)))) return NULL;

   ret = memcpy(ret, &default_info, sizeof(default_info));

   return ret;
}

/**
 * @param screen_info the screen info to be freed.
 */
static void
_e_randr_screen_info_11_free(E_Randr_Screen_Info_11 *screen_info)
{
   if (!screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_1)) 
     return;

   if (screen_info->sizes)
     {
        free(eina_list_nth(screen_info->sizes, 0));
        eina_list_free(screen_info->sizes);
     }
   if (screen_info->rates)
     {
       /* this may be leaking, but at least it will be valid */
       eina_list_free(eina_list_nth(screen_info->rates, 0));
       eina_list_free(screen_info->rates);
     }
   free(screen_info);
   screen_info = NULL;
}

/**
 * @return array of E_Randr_Screen_Info_12 elements, or in case not all could
 * be created or parameter 'nrequested'==0, NULL
 */
static E_Randr_Screen_Info_12 *
_e_randr_screen_info_12_new(void)
{
   E_Randr_Screen_Info_12 *ret = NULL;
   static const E_Randr_Screen_Info_12 default_info =
     {
        .min_size = {Ecore_X_Randr_Unset, Ecore_X_Randr_Unset},
        .max_size = {Ecore_X_Randr_Unset, Ecore_X_Randr_Unset},
        .current_size = {Ecore_X_Randr_Unset, Ecore_X_Randr_Unset},
        .crtcs = NULL,
        .outputs = NULL,
        .primary_output = NULL,
        .alignment = ECORE_X_RANDR_RELATIVE_ALIGNMENT_NONE
     };

   if (!(ret = malloc(sizeof(E_Randr_Screen_Info_12)))) return NULL;
   ret = memcpy(ret, &default_info, sizeof(default_info));

   return ret;
}

static Eina_Bool
_e_randr_screen_info_12_set(E_Randr_Screen_Info_12 *screen_info)
{
   E_RANDR_NO_12_RET(EINA_FALSE);

   ecore_x_randr_screen_size_range_get(e_randr_screen_info->root, 
                                       &screen_info->min_size.width, 
                                       &screen_info->min_size.height, 
                                       &screen_info->max_size.width , 
                                       &screen_info->max_size.height);
   ecore_x_randr_screen_current_size_get(e_randr_screen_info->root, 
                                         &screen_info->current_size.width , 
                                         &screen_info->current_size.height , 
                                         NULL, NULL);

   return EINA_TRUE;
}

static Eina_Bool
_e_randr_screen_info_11_set(void)
{
   E_RANDR_NO_11_RET(EINA_FALSE);
   
   E_Randr_Screen_Info_11 *screen_info_11 = e_randr_screen_info->rrvd_info.randr_info_11;
   Ecore_X_Randr_Screen_Size_MM *sizes = NULL;
   Ecore_X_Randr_Refresh_Rate *rates = NULL;
   Eina_List *rates_list;
   int i, j, nsizes, nrates;

   if (!(sizes = ecore_x_randr_screen_primary_output_sizes_get(e_randr_screen_info->root, &nsizes))) 
      return EINA_FALSE;
   for (i = 0; i < nsizes; i++)
      if (!(screen_info_11->sizes = eina_list_append(screen_info_11->sizes, &sizes[i])))
         goto _e_randr_screen_info_11_fill_fail_sizes;
   ecore_x_randr_screen_primary_output_current_size_get(e_randr_screen_info->root, NULL, NULL, NULL, NULL, &(screen_info_11->csize_index));
   screen_info_11->corientation = ecore_x_randr_screen_primary_output_orientation_get(e_randr_screen_info->root);
   screen_info_11->orientations = ecore_x_randr_screen_primary_output_orientations_get(e_randr_screen_info->root);
   for (i = 0; i < nsizes; i++)
     {
        rates_list = NULL;
        if (!(rates = ecore_x_randr_screen_primary_output_refresh_rates_get(e_randr_screen_info->root, i, &nrates)))
           return EINA_FALSE;
        for (j = 0; j < nrates; j++)
           if (!(rates_list = eina_list_append(rates_list, &rates[j])))
              goto _e_randr_screen_info_11_fill_fail_rates_list;
        if (!(screen_info_11->rates = eina_list_append(screen_info_11->rates, rates_list)))
           goto _e_randr_screen_info_11_fill_fail_rates;
     }
   screen_info_11->current_rate = ecore_x_randr_screen_primary_output_current_refresh_rate_get(e_randr_screen_info->root);
   
   return EINA_TRUE;
   
_e_randr_screen_info_11_fill_fail_rates_list:
   eina_list_free(rates_list);
_e_randr_screen_info_11_fill_fail_rates:
   free(rates);
_e_randr_screen_info_11_fill_fail_sizes:
   free(sizes);
   free(screen_info_11);
   return EINA_FALSE;
}

/**
 * @param screen_info the screen info to be freed.
 */
static void
_e_randr_screen_info_12_free(E_Randr_Screen_Info_12 *screen_info)
{
   Ecore_X_Randr_Mode_Info *mode_info;
   E_Randr_Crtc_Info *crtc_info;
   E_Randr_Output_Info *output_info;

   if (!screen_info || (e_randr_screen_info->randr_version < ECORE_X_RANDR_1_2)) return;

   if (e_randr_screen_info->randr_version >= ECORE_X_RANDR_1_2 && screen_info->crtcs)
     {
        EINA_LIST_FREE(screen_info->crtcs, crtc_info)
          _e_randr_crtc_info_free(crtc_info);
        free(eina_list_nth(screen_info->crtcs, 0));
     }

   if (e_randr_screen_info->randr_version >= ECORE_X_RANDR_1_2 && screen_info->outputs)
     {
        EINA_LIST_FREE(screen_info->outputs, output_info)
          _e_randr_output_info_free(output_info);
        free(eina_list_nth(screen_info->outputs, 0));
     }

   if (e_randr_screen_info->randr_version >= ECORE_X_RANDR_1_2 && screen_info->modes)
     {
        EINA_LIST_FREE(screen_info->modes, mode_info)
          ecore_x_randr_mode_info_free(mode_info);
     }

   _e_randr_event_listeners_remove();

   free (screen_info);
   screen_info = NULL;
}

/**
 * @brief allocates structs with and fills them with default values. The
 * returned pointer is allocated as one chunk of data since it won't change.
 * @param nrequested number of E_Randr_Crtc_Info to be created
 * @return array of E_Randr_Crtc_Info elements, or in case not all could
 * be created or parameter 'nrequested'==0, NULL
 */
static E_Randr_Crtc_Info *
_e_randr_crtc_info_new(int nrequested)
{
   E_Randr_Crtc_Info *ret = NULL;
   static E_Randr_Crtc_Info default_info =
     {
        .xid = Ecore_X_Randr_Unset,
        .geometry = {Ecore_X_Randr_Unset, Ecore_X_Randr_Unset, Ecore_X_Randr_Unset, Ecore_X_Randr_Unset},
        .panning = {Ecore_X_Randr_Unset, Ecore_X_Randr_Unset, Ecore_X_Randr_Unset, Ecore_X_Randr_Unset},
        .tracking = {Ecore_X_Randr_Unset, Ecore_X_Randr_Unset, Ecore_X_Randr_Unset, Ecore_X_Randr_Unset},
        .border = {Ecore_X_Randr_Unset, Ecore_X_Randr_Unset, Ecore_X_Randr_Unset, Ecore_X_Randr_Unset},
        .current_orientation = ECORE_X_RANDR_ORIENTATION_ROT_0,
        .orientations = Ecore_X_Randr_Unset,
        .gamma_ramps = NULL,
        .gamma_ramp_size = Ecore_X_Randr_Unset,
        .outputs = NULL,
        .possible_outputs = NULL
     };

   if (!(ret = malloc(sizeof(E_Randr_Crtc_Info) * nrequested))) return NULL;

   while (nrequested > 0)
     {
        memcpy(&ret[--nrequested], &default_info, sizeof(default_info));
     }

   return ret;
}

/**
 * @param crtc_info the crtc info to be freed.
 */
static void
_e_randr_crtc_info_free(E_Randr_Crtc_Info *crtc_info)
{
   if (!crtc_info) return;

   if (crtc_info->gamma_ramps) free(crtc_info->gamma_ramps);
   if (crtc_info->outputs) eina_list_free(crtc_info->outputs);
   if (crtc_info->possible_outputs) eina_list_free(crtc_info->possible_outputs);
}

/**
 * @brief allocates structs with and fills them with default values. The
 * returned pointer is allocated as one chunk of data since it won't change.
 * @param nrequested number of E_Randr_Output_Info to be created
 * @return E_Randr_Output_Info element, or it could not be
 * created, NULL
 */
static E_Randr_Output_Info *
_e_randr_output_info_new(int nrequested)
{
   E_Randr_Output_Info *ret = NULL;
   static E_Randr_Output_Info default_info =
     {
        .xid = Ecore_X_Randr_Unset,
        .name = NULL,
        .crtc = NULL,
        .possible_crtcs = NULL,
        .preferred_modes = NULL,
        .max_backlight = Ecore_X_Randr_Unset,
        .backlight_level = 0.0,
        .edid = NULL,
        .edid_length = 0,
        .size_mm = {Ecore_X_Randr_Unset, Ecore_X_Randr_Unset},
        .wired_clones = NULL,
        .signalformats = Ecore_X_Randr_Unset,
        .signalformat = Ecore_X_Randr_Unset,
        .connector_number = Ecore_X_Randr_Unset,
        .connector_type = Ecore_X_Randr_Unset,
        .connection_status = ECORE_X_RANDR_CONNECTION_STATUS_DISCONNECTED,
        .subpixel_order= Ecore_X_Randr_Unset,
        .compatible_outputs = NULL,
        .policy = ECORE_X_RANDR_OUTPUT_POLICY_NONE
     };

   if (!(ret = malloc(sizeof(E_Randr_Output_Info) * nrequested))) return NULL;

   while (nrequested > 0)
     {
        memcpy(&ret[--nrequested], &default_info, sizeof(default_info));
     }

   return ret;
}

/*
 * removes all traces of an output within the data.
 * @param output_info the output info to be freed.
 */
static void
_e_randr_output_info_free(E_Randr_Output_Info *output_info)
{
   Eina_List *iter;
   E_Randr_Crtc_Info *crtc_info;

   if (!output_info) return;
   if (output_info->name)
     {
        free(output_info->name);
        output_info->name = NULL;
     }
   _e_randr_output_hw_info_free(output_info);

   EINA_LIST_FOREACH(e_randr_screen_info->rrvd_info.randr_info_12->crtcs, iter, crtc_info)
     {
        crtc_info->outputs = eina_list_remove(crtc_info->outputs, output_info);
     }
}

static void
_e_randr_output_info_set(E_Randr_Output_Info *output_info)
{
   if (E_RANDR_NO_12 || !output_info) return;

   output_info->name = ecore_x_randr_output_name_get(e_randr_screen_info->root, output_info->xid, &output_info->name_length);
   output_info->connection_status = ecore_x_randr_output_connection_status_get(e_randr_screen_info->root, output_info->xid);
}

/*
 * fills a given crtc_info using its xid with
 * - geometry data (x,y,w,h)
 * - used outputs structs
 * - possible outputs structs
 * - mode
 * - connection status
 * - orientation
 */
static void
_e_randr_crtc_info_set(E_Randr_Crtc_Info *crtc_info)
{
   Ecore_X_Randr_Mode mode = 0;
   fprintf(stderr, "Fillng CRTC %d (%p)\n", crtc_info->xid, crtc_info);
   if (E_RANDR_NO_12 || !crtc_info) return;

   //get references to used and possible E_Randr_Output_Info structs
   _e_randr_crtc_outputs_refs_update(crtc_info);

   ecore_x_randr_crtc_geometry_get(e_randr_screen_info->root, crtc_info->xid, &crtc_info->geometry.x, &crtc_info->geometry.y, &crtc_info->geometry.w, &crtc_info->geometry.h);
   mode = ecore_x_randr_crtc_mode_get(e_randr_screen_info->root, crtc_info->xid);
   crtc_info->current_mode = _e_randr_mode_info_get(mode);
   fprintf(stderr, "CRTC %d apparently is in mode %d, trying to find it in the list of modes..\n", crtc_info->xid, mode);
   if (crtc_info->current_mode)
     fprintf(stderr, "found CRTC %d in mode %d\n", crtc_info->xid, crtc_info->current_mode->xid);
   crtc_info->current_orientation = ecore_x_randr_crtc_orientation_get(e_randr_screen_info->root, crtc_info->xid);
   crtc_info->outputs_common_modes = _e_randr_outputs_common_modes_get(crtc_info->outputs, NULL);
}

/*
 * looks up modes supported by an output and adds them - if they are not already
 * known by - to the screen's information struct ant the output_info itself
 */
static Eina_Bool
_e_randr_output_modes_add(E_Randr_Output_Info *output_info)
{
   Ecore_X_Randr_Mode *modes;
   Ecore_X_Randr_Mode_Info *mode_info;
   int nmodes, npreferred;
   Eina_List *iter;
   Eina_Bool added_yet = EINA_FALSE;

   if (E_RANDR_NO_12 || !(modes = ecore_x_randr_output_modes_get(e_randr_screen_info->root, output_info->xid, &nmodes, &npreferred))) return EINA_FALSE;

   //In case the monitor does not have any preferred mode at all
   if (nmodes > 0 && npreferred == 0) npreferred = 1;

   while (--nmodes >= 0)
     {
        added_yet = EINA_FALSE;
        EINA_LIST_FOREACH(e_randr_screen_info->rrvd_info.randr_info_12->modes, iter, mode_info)
          {
             if (mode_info && mode_info->xid == modes[nmodes])
               {
                  added_yet = EINA_TRUE;
                  break;
               }
          }
        if(!added_yet)
          {
             mode_info = ecore_x_randr_mode_info_get(e_randr_screen_info->root, modes[nmodes]);
             e_randr_screen_info->rrvd_info.randr_info_12->modes = eina_list_prepend(e_randr_screen_info->rrvd_info.randr_info_12->modes, mode_info);
          }
        output_info->modes = eina_list_prepend(output_info->modes, mode_info);
        if (nmodes < npreferred) output_info->preferred_modes = eina_list_prepend(output_info->preferred_modes, mode_info);

     }

   free(modes);
   return EINA_TRUE;
}

static Eina_Bool
_e_randr_screen_crtcs_init(void)
{
   Ecore_X_Randr_Crtc *crtcs = NULL;
   E_Randr_Crtc_Info *crtcs_info = NULL, *crtc = NULL;
   int i, ncrtcs;

   if (E_RANDR_NO_12 || !(crtcs = ecore_x_randr_crtcs_get(e_randr_screen_info->root, &ncrtcs))) return EINA_FALSE;

   if (!(crtcs_info = _e_randr_crtc_info_new(ncrtcs))) goto ecore_x_randr_screen_crtcs_init_fail_free_crtcs;
   for (i = 0; i < ncrtcs; i++)
     {
        fprintf (stderr, "E_RANDR: filling %d/%d (%d)\n", (i + 1), ncrtcs, crtcs[i]);
        crtcs_info[i].xid = crtcs[i];
        _e_randr_crtc_info_set(&crtcs_info[i]);
        if(!(e_randr_screen_info->rrvd_info.randr_info_12->crtcs = eina_list_append(e_randr_screen_info->rrvd_info.randr_info_12->crtcs, &crtcs_info[i]))) break;
     }
   if (i == ncrtcs)
     {
        //successfully initialized crtcs!
        free (crtcs);
        return EINA_TRUE;
     }
   EINA_LIST_FREE(e_randr_screen_info->rrvd_info.randr_info_12->crtcs, crtc)
     _e_randr_crtc_info_free(crtc);
   if (e_randr_screen_info->rrvd_info.randr_info_12->crtcs)
     {
        free(eina_list_nth(e_randr_screen_info->rrvd_info.randr_info_12->crtcs, 0));
     }

ecore_x_randr_screen_crtcs_init_fail_free_crtcs:
   free(crtcs);
   return EINA_FALSE;
}

static Eina_Bool
_e_randr_screen_outputs_init(void)
{
   Ecore_X_Randr_Output *outputs;
   E_Randr_Output_Info *outputs_info = NULL, *output_info = NULL;
   int noutputs = 0;
   if (E_RANDR_NO_12 || !(outputs = ecore_x_randr_outputs_get(e_randr_screen_info->root, &noutputs))) return EINA_FALSE;

   if (!(outputs_info = _e_randr_output_info_new(noutputs))) goto _e_randr_screen_outputs_init_fail_free_outputs;
   while (--noutputs >= 0)
     {
        outputs_info[noutputs].xid = outputs[noutputs];
        _e_randr_output_info_set(&outputs_info[noutputs]);
        if (outputs_info[noutputs].connection_status == ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED)
          _e_randr_output_info_hw_info_set(&outputs_info[noutputs]);

        if (!(e_randr_screen_info->rrvd_info.randr_info_12->outputs = eina_list_append(e_randr_screen_info->rrvd_info.randr_info_12->outputs, &outputs_info[noutputs]))) goto _e_randr_screen_outputs_init_fail_free_outputs_list;
     }

   free(outputs);
   return EINA_TRUE;

_e_randr_screen_outputs_init_fail_free_outputs_list:
   if (e_randr_screen_info->rrvd_info.randr_info_12->outputs)
     {
        EINA_LIST_FREE(e_randr_screen_info->rrvd_info.randr_info_12->outputs, output_info)
          free(output_info);
     }
_e_randr_screen_outputs_init_fail_free_outputs:
   free(outputs);
   return EINA_FALSE;
}

static Ecore_X_Randr_Mode_Info*
_e_randr_mode_info_get(Ecore_X_Randr_Mode mode)
{
   Eina_List *iter;
   Ecore_X_Randr_Mode_Info* mode_info;

   E_RANDR_NO_MODE_RET(mode, NULL);
   EINA_LIST_FOREACH(e_randr_screen_info->rrvd_info.randr_info_12->modes, iter, mode_info)
     {
        if (mode_info && mode_info->xid == mode) return mode_info;
     }
   return NULL;
}

static E_Randr_Output_Info*
_e_randr_output_info_get(Ecore_X_Randr_Output output)
{
   Eina_List *iter;
   E_Randr_Output_Info* output_info;

   E_RANDR_NO_OUTPUTS_RET(NULL);
   EINA_LIST_FOREACH(e_randr_screen_info->rrvd_info.randr_info_12->outputs, iter, output_info)
     {
        if (output_info && output_info->xid == output) return output_info;
     }
   return NULL;
}

static E_Randr_Crtc_Info*
_e_randr_crtc_info_get(Ecore_X_Randr_Crtc crtc)
{
   Eina_List *iter;
   E_Randr_Crtc_Info* crtc_info;

   E_RANDR_NO_CRTCS_RET(NULL);
   EINA_LIST_FOREACH(e_randr_screen_info->rrvd_info.randr_info_12->crtcs, iter, crtc_info)
     {
        if (crtc_info && crtc_info->xid == crtc) return crtc_info;
     }
   return NULL;
}

static Eina_Bool
_e_randr_event_cb(void *data __UNUSED__, int type, void *ev)
{
   E_Randr_Crtc_Info *crtc_info;
   Eina_Bool enabled;

   // FIXME: ecore_x_randr_screen_primary_output_orientation_get() should
   // get regularly called to push x into polling outputs so we get these
   // event callbacks - maybe do this every 100 mouse move events, or every
   // mouse release or event mouse in or out, and in addition add a "poller"
   // that runs every 256 ticks maybe?
   if (!e_randr_screen_info) return ECORE_CALLBACK_RENEW;
   if (type == ECORE_X_EVENT_RANDR_CRTC_CHANGE)
     {
        Ecore_X_Event_Randr_Crtc_Change *event = (Ecore_X_Event_Randr_Crtc_Change*) ev;
        /* available information:
           struct _Ecore_X_Event_Randr_Crtc_Change
           {
           Ecore_X_Window                win;
           Ecore_X_Randr_Crtc            crtc;
           Ecore_X_Randr_Mode            mode;
           Ecore_X_Randr_Orientation     orientation;
           int                           x;
           int                           y;
           int                           width;
           int                           height;
           };
           */
        crtc_info = _e_randr_crtc_info_get(event->crtc);
	if (!crtc_info) return ECORE_CALLBACK_RENEW;

        if (event->mode != Ecore_X_Randr_None)
          {
             //switched (on)
             if ((crtc_info->current_mode != _e_randr_mode_info_get(event->mode)))
               {
                  crtc_info->current_mode = _e_randr_mode_info_get(event->mode);
                  _e_randr_notify_crtc_mode_change(crtc_info);
               }
             else
               crtc_info->current_mode = _e_randr_mode_info_get(event->mode);
             crtc_info->current_orientation = event->orientation;
             crtc_info->geometry.x = event->geo.x;
             crtc_info->geometry.y = event->geo.y;
             crtc_info->geometry.w = event->geo.w;
             crtc_info->geometry.h = event->geo.h;
             //update screensize if necessary
             e_randr_screen_info->rrvd_info.randr_info_12->current_size.width = MAX((event->geo.x + event->geo.w), e_randr_screen_info->rrvd_info.randr_info_12->current_size.width);
             e_randr_screen_info->rrvd_info.randr_info_12->current_size.height = MAX((event->geo.y + event->geo.h), e_randr_screen_info->rrvd_info.randr_info_12->current_size.height);
          }
        else
          {
             //set the max mode common amongst outputs
             _e_randr_crtcs_clone_crtc_removed(crtc_info);

             //disabled
             crtc_info->current_orientation = event->orientation;
             crtc_info->geometry.x = 0;
             crtc_info->geometry.y = 0;
             crtc_info->geometry.w = 0;
             crtc_info->geometry.h = 0;
             crtc_info->current_mode = NULL;
             if (crtc_info->outputs) eina_list_free(crtc_info->outputs);
             crtc_info->outputs = NULL;

             //update screensize of necessary
             ecore_x_randr_screen_reset(e_randr_screen_info->root);
             ecore_x_randr_screen_current_size_get(e_randr_screen_info->root, &e_randr_screen_info->rrvd_info.randr_info_12->current_size.width, &e_randr_screen_info->rrvd_info.randr_info_12->current_size.height, NULL, NULL);
          }
     }
   else if (type == ECORE_X_EVENT_RANDR_OUTPUT_CHANGE)
     {
        Ecore_X_Event_Randr_Output_Change *event = ev;
        E_Randr_Output_Info* output_info = NULL;
        /* available information:
           struct _Ecore_X_Event_Randr_Output_Change
           {
           Ecore_X_Window                  win;
           Ecore_X_Randr_Output            output;
           Ecore_X_Randr_Crtc              crtc;
           Ecore_X_Randr_Mode              mode;
           Ecore_X_Randr_Orientation       orientation;
           Ecore_X_Randr_Connection_Status connection;
           Ecore_X_Render_Subpixel_Order   subpixel_order;
           };
           */
        fprintf(stderr, "E_RANDR: Output connected!: \n \
              E_RANDR: relative to win: %d\n \
              E_RANDR: relative to output: %d\n \
              E_RANDR: relative to crtc: %d\n \
              E_RANDR: relative to mode: %d\n \
              E_RANDR: relative to orientation: %d\n \
              E_RANDR: relative to connction: %d (connected = %d, disconnected = %d, unknown %d)\n \
              E_RANDR: relative to subpixel_order: %d\n",
                event->win, event->output, event->crtc, event->mode, event->orientation, event->connection, ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED, ECORE_X_RANDR_CONNECTION_STATUS_DISCONNECTED, ECORE_X_RANDR_CONNECTION_STATUS_UNKNOWN, event->subpixel_order);

        output_info = _e_randr_output_info_get(event->output);
        if (!output_info) return ECORE_CALLBACK_RENEW;
        
        if ((output_info->crtc = _e_randr_crtc_info_get(event->crtc)))
          {
             if (!eina_list_data_find(output_info->crtc->outputs, output_info))
                output_info->crtc->outputs = eina_list_append(output_info->crtc->outputs, output_info);
          }

        output_info->connection_status = event->connection;
        output_info->subpixel_order = event->subpixel_order;
        
        if (event->connection == ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED)
          {
             if (event->crtc)
                output_info->crtc = _e_randr_crtc_info_get(event->crtc);

             if (output_info && !output_info->crtc && !event->crtc && !event->mode)
               {
                  //Monitor was attached!
                  _e_randr_output_info_hw_info_set(output_info);
                  //make the crtcs aware of their possibly new output
                  _e_randr_crtcs_possible_output_update(output_info);
                  /*
                  if ((restore_info = _e_randr_config_find_suiting_config_12()))
                     //maybe we have a suiting configuration
                     //_e_randr_config_enable_12(restore_info);
                     ;
                  else
                  */
                     enabled = _e_randr_try_enable_output(output_info, EINA_FALSE); //maybe give a success message?
               }
             _e_randr_notify_output_change(output_info);
          }
        else if (event->connection == ECORE_X_RANDR_CONNECTION_STATUS_DISCONNECTED)
          {
             if (output_info->crtc)
               {
                  //remove output from CRTC
                  output_info->crtc->outputs = eina_list_remove(output_info->crtc->outputs, output_info);
                  if (output_info->crtc->current_mode)
                    {
                       //in case this output was enabled on some CRTC
                       if (eina_list_count(output_info->crtc->outputs) == 0)
                         {
                            //in case it was the only output running on this CRTC, disable
                            //it.
                            ecore_x_randr_crtc_mode_set(e_randr_screen_info->root, output_info->crtc->xid, NULL, Ecore_X_Randr_None, Ecore_X_Randr_None);
                            //crop the screen of course.
                            ecore_x_randr_screen_reset(e_randr_screen_info->root);
                         }
                       else
                          _e_randr_crtc_outputs_mode_max_set(output_info->crtc);
                    }
                  
                  if (e_randr_screen_info->rrvd_info.randr_info_12->primary_output && (output_info == e_randr_screen_info->rrvd_info.randr_info_12->primary_output))
                     _e_randr_screen_primary_output_assign(output_info);
                  //let's try to get a proper config for the new setup and crop the
                  //screen afterwards.
                  /*
                  if ((restore_info = _e_randr_config_find_suiting_config_12()))
		    {
		       //in case we didn't have, init it anyway...
		       //_e_randr_config_enable_12(restore_info);
                    }
                  */
               }
             _e_randr_notify_output_change(output_info);
             _e_randr_output_hw_info_free(output_info);
          }
     }
   else if (type == ECORE_X_EVENT_RANDR_OUTPUT_PROPERTY_NOTIFY)
     {
        //Ecore_X_Event_Randr_Output_Property_Notify *event = (Ecore_X_Event_Randr_Output_Property_Notify*) ev;
        /* available information:
           struct _Ecore_X_Event_Randr_Output_Property_Notify
           {
           Ecore_X_Window                win;
           Ecore_X_Randr_Output          output;
           Ecore_X_Atom                  property;
           Ecore_X_Time                  time;
           Ecore_X_Randr_Property_Change state;
           };
           */
     }
   e_randr_try_restore_configuration(e_randr_screen_info);
   return ECORE_CALLBACK_RENEW;
}

static void
_e_randr_event_listeners_add(void)
{
   if (E_RANDR_NO_12) return;
   ecore_x_randr_events_select(e_randr_screen_info->root, EINA_TRUE);
   _e_randr_event_handlers= eina_list_append(_e_randr_event_handlers, ecore_event_handler_add(ECORE_X_EVENT_RANDR_CRTC_CHANGE, _e_randr_event_cb, NULL));
   _e_randr_event_handlers= eina_list_append(_e_randr_event_handlers, ecore_event_handler_add(ECORE_X_EVENT_RANDR_OUTPUT_CHANGE, _e_randr_event_cb, NULL));
   _e_randr_event_handlers= eina_list_append(_e_randr_event_handlers, ecore_event_handler_add(ECORE_X_EVENT_RANDR_OUTPUT_PROPERTY_NOTIFY, _e_randr_event_cb, NULL));
}

static void
_e_randr_event_listeners_remove(void)
{
   Ecore_Event_Handler *_event_handler = NULL;
   EINA_LIST_FREE(_e_randr_event_handlers, _event_handler)
     ecore_event_handler_del(_event_handler);
}

static void
_e_randr_notify_crtc_mode_change(E_Randr_Crtc_Info *crtc_info)
{
   //   E_Notification *n;
   //   char buff[200];
   //
   if (crtc_info->current_mode)
     {
        //        snprintf(buff, 200, "New resolution is %dx%d. Click here for further information.", crtc_info->current_mode->width, crtc_info->current_mode->height);
        //        n = e_notification_full_new("RandRR", crtc_info->xid, NULL, "Resolution changed", buff, -1);
        //        //n = e_notification_full_new("RandRR", id, icon, function, body, timeout);
        //        e_notification_send(n, NULL, NULL);
        //        e_notification_unref(n);
     }
}
static void
_e_randr_notify_output_change(E_Randr_Output_Info *output_info)
{
   //   E_Notification *n;
   //   char buff[100];
   if (output_info->connection_status == ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED)
     {
        //        snprintf(buff, 100, "Output %s connected", output_info->name);
        //        n = e_notification_full_new("RandRR", output_info->xid, NULL, buff, "Click here for further information.", -1);
     }
   else
     {
        //        snprintf(buff, 100, "Output %s disconnected", output_info->name);
        //        n = e_notification_full_new("RandRR", output_info->xid, NULL, buff, "Click here to adjust screen setup.", -1);
     }

   //   //n = e_notification_full_new("RandRR", id, icon, function, body, timeout);
   //   e_notification_send(n, NULL, NULL);
   //   e_notification_unref(n);
}

/*
 * this retrieves a CRTC depending on a policy.
 * Note that this is enlightenment specific! Enlightenment doesn't 'allow' zones
 * to overlap. Thus we always use the output with the most extreme position
 * instead of trying to fill gaps like tetris. Though this could be done by
 * simply implementing another policy.
 */
static const E_Randr_Crtc_Info*
_e_randr_policy_crtc_get(E_Randr_Crtc_Info *but, E_Randr_Crtc_Info *hint __UNUSED__, Ecore_X_Randr_Output_Policy policy)
{
   Eina_List *iter;
   E_Randr_Crtc_Info *crtc_info, *ret = NULL;

   E_RANDR_NO_CRTCS_RET(NULL);

   //get any crtc that besides 'but' to start with
   EINA_LIST_FOREACH(e_randr_screen_info->rrvd_info.randr_info_12->crtcs, iter, crtc_info)
     {
        if (crtc_info != but)
          {
             ret = crtc_info;
             break;
          }
     }

   if (!ret && (policy != ECORE_X_RANDR_OUTPUT_POLICY_CLONE)) return NULL;

   switch (policy)
     {
      case ECORE_X_RANDR_OUTPUT_POLICY_ABOVE:
        EINA_LIST_FOREACH(e_randr_screen_info->rrvd_info.randr_info_12->crtcs, iter, crtc_info)
          {
             if (crtc_info && (crtc_info != but) && 
                 (crtc_info->geometry.y <= ret->geometry.y))
               ret = crtc_info;
           }
        break;
      case ECORE_X_RANDR_OUTPUT_POLICY_RIGHT:
        EINA_LIST_FOREACH(e_randr_screen_info->rrvd_info.randr_info_12->crtcs, iter, crtc_info)
          {
             if (crtc_info && (crtc_info != but) && 
                 ((crtc_info->geometry.x + crtc_info->geometry.w) >= 
                     (ret->geometry.x + ret->geometry.w)))
               ret = crtc_info;
           }
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_BELOW:
        EINA_LIST_FOREACH(e_randr_screen_info->rrvd_info.randr_info_12->crtcs, iter, crtc_info)
          {
             if (crtc_info && (crtc_info != but) && 
                 ((crtc_info->geometry.y + crtc_info->geometry.h) >= 
                     (ret->geometry.y + ret->geometry.h)))
               ret = crtc_info;
           }
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_LEFT:
        EINA_LIST_FOREACH(e_randr_screen_info->rrvd_info.randr_info_12->crtcs, iter, crtc_info)
          {
             if (crtc_info && (crtc_info != but) && 
                 (crtc_info->geometry.x <= ret->geometry.x))
               ret = crtc_info;
           }
        break;

      case ECORE_X_RANDR_OUTPUT_POLICY_CLONE:
        ret = (e_randr_screen_info->rrvd_info.randr_info_12->primary_output) ? e_randr_screen_info->rrvd_info.randr_info_12->primary_output->crtc : NULL;
        break;

      default:
        break;
     }
   return ret;
}

// Setup store functions
   Eina_Bool
_e_randr_copy_mode_info(Ecore_X_Randr_Mode_Info *dest, Ecore_X_Randr_Mode_Info *src)
{

   if (!dest || !src) return EINA_FALSE;

   dest->width = src->width;
   dest->height = src->height;
   dest->dotClock = src->dotClock;
   dest->hSyncStart = src->hSyncStart;
   dest->hSyncEnd = src->hSyncEnd;
   dest->hTotal = src->hTotal;
   dest->hSkew = src->hSkew;
   dest->vSyncStart = src->vSyncStart;
   dest->vSyncEnd = src->vSyncEnd;
   dest->vTotal = src->vTotal;
   if (src->nameLength > 0)
     {
        if (!(dest->name = malloc(src->nameLength + 1))) return EINA_FALSE;
        if (!strncpy(dest->name, src->name, src->nameLength)) goto _e_randr_copy_mode_info_fail_free_name;
     }
   dest->nameLength = src->nameLength;
   dest->modeFlags = src->modeFlags;

   return EINA_TRUE;

_e_randr_copy_mode_info_fail_free_name:
   free(dest->name);
   return EINA_FALSE;
}

   void
_e_randr_free_serialized_mode_info(Ecore_X_Randr_Mode_Info *mode_info)
{
   if (mode_info->name) free(mode_info->name);
}

   Eina_List
*_e_randr_create_outputs_policies_list(Eina_List *outputs)
{
   Eina_List *iter, *list = NULL;
   E_Randr_Output_Info *oi;
   E_Randr_Serialized_Output_Policy *sop;

   EINA_LIST_FOREACH(outputs, iter, oi)
     {
        if (!oi->name || (oi->name_length <= 0)) continue;
        if (!(sop = E_NEW(E_Randr_Serialized_Output_Policy, 1))
              || !(sop->name = malloc(oi->name_length + 1))
              || !(strncpy(sop->name, oi->name, oi->name_length)))
          goto _e_randr_create_outputs_policies_list_fail_free_list;
        sop->name_length = oi->name_length;
        sop->policy = oi->policy;
        if (!(list = eina_list_append(list, sop)))
          goto _e_randr_create_outputs_policies_list_fail_free_list;
     }

   return list;
_e_randr_create_outputs_policies_list_fail_free_list:
   EINA_LIST_FREE(list, sop)
     {
        if (sop->name) free(sop->name);
        free(sop);
     }
   return NULL;
}

void
_e_randr_free_serialized_output_policy(E_Randr_Serialized_Output_Policy *sop)
{
   if (!sop) return;
   if (sop->name) free(sop->name);
   free(sop);
}

   Eina_List
*_e_randr_update_serialized_outputs_policies(E_Randr_Screen_Info_12 *si_12, Eina_List *sops)
{
   E_Randr_Serialized_Output_Policy *sop;

   EINA_LIST_FREE(sops, sop)
     {
        _e_randr_free_serialized_output_policy(sop);
     }

   return _e_randr_create_outputs_policies_list(si_12->outputs);
}

   Eina_List
*_e_randr_create_possible_outputs_names_list(Eina_List *outputs)
{
   Eina_List *iter, *list = NULL;
   E_Randr_Output_Info *oi;
   char *name;

   if (!outputs) return NULL;

   EINA_LIST_FOREACH(outputs, iter, oi)
     {
        if (!oi->name || (oi->name_length <= 0)) continue;
        if (!(name = malloc(oi->name_length))
              || !strncpy(name, oi->name, oi->name_length)
              || !(list = eina_list_append(list, name))) goto _e_randr_create_possible_outputs_names_list_fail_free_list;
     }
   return list;

_e_randr_create_possible_outputs_names_list_fail_free_list:
   EINA_LIST_FREE(list, name)
     {
        if(name) free(name);
     }
   return NULL;
}

   E_Randr_Edid_Hash
*_e_randr_create_edid_hash(E_Randr_Output_Info *output_info)
{
   E_Randr_Edid_Hash *edid_hash;

   if (!output_info || (output_info->edid_hash.hash == 0) || !(edid_hash = malloc(sizeof(E_Randr_Edid_Hash)))) return NULL;

   edid_hash->hash = output_info->edid_hash.hash;

   return edid_hash;
}

   E_Randr_Serialized_Output
*_e_randr_create_serialized_output(E_Randr_Output_Info *output_info)
{
   E_Randr_Serialized_Output *so;
   char *name;

   if (!output_info || !output_info->name || (output_info->name_length <= 0) || !(so = malloc(sizeof(E_Randr_Serialized_Output)))) return NULL;

   if (!(name = malloc(output_info->name_length))
         || !strncpy(so->name, output_info->name, output_info->name_length))
     goto _e_randr_create_serialized_outputs_fail_free_so;
   so->name_length = output_info->name_length;
   so->edid_hash.hash = output_info->edid_hash.hash;
   so->backlight_level = output_info->backlight_level;

   return so;

_e_randr_create_serialized_outputs_fail_free_so:
   free(so);
   return NULL;
}

   void
_e_randr_free_serialized_output(E_Randr_Serialized_Output *so)
{
   if (so->name) free(so->name);
   free(so);
}

   E_Randr_Serialized_Crtc
*_e_randr_create_serialized_crtc(E_Randr_Crtc_Info *crtc_info)
{
   E_Randr_Serialized_Crtc *sc;
   E_Randr_Serialized_Output *so;
   E_Randr_Output_Info *output_info;
   Eina_List *iter;
   char *output_name;

   if (!(sc = E_NEW(E_Randr_Serialized_Crtc, 1))) return NULL;
   if(!_e_randr_copy_mode_info(&sc->mode_info, crtc_info->current_mode)) goto _e_randr_create_serialized_crtc_free_sc;
   if(!(sc->possible_outputs_names = _e_randr_create_possible_outputs_names_list(crtc_info->possible_outputs))) goto _e_randr_create_serialized_crtc_free_mode_sc;
   //Create list of serialized outputs
   EINA_LIST_FOREACH(crtc_info->outputs, iter, output_info)
     {
        if(!(so = _e_randr_create_serialized_output(output_info))
              || !(sc->serialized_outputs = eina_list_append(sc->serialized_outputs, so))) goto _e_randr_create_serialized_crtc_free_outputs_list_sc;
     }
   sc->pos.x = crtc_info->geometry.x;
   sc->pos.y = crtc_info->geometry.y;
   sc->orientation = crtc_info->current_orientation;

   return sc;

_e_randr_create_serialized_crtc_free_outputs_list_sc:
   EINA_LIST_FREE(sc->possible_outputs_names, output_name)
     {
        if (output_name) free(output_name);
     }
   EINA_LIST_FREE(sc->serialized_outputs, so)
     {
        if (so) _e_randr_free_serialized_output(so);
     }
_e_randr_create_serialized_crtc_free_mode_sc:
   _e_randr_free_serialized_mode_info(&sc->mode_info);
_e_randr_create_serialized_crtc_free_sc:
   E_FREE(sc);

   return NULL;
}

   void
_e_randr_free_serialized_crtc(E_Randr_Serialized_Crtc *sc)
{
   E_Randr_Serialized_Output *so;
   char *name;

   EINA_LIST_FREE(sc->serialized_outputs, so)
      _e_randr_free_serialized_output(so);
   _e_randr_free_serialized_mode_info(&sc->mode_info);
   EINA_LIST_FREE(sc->possible_outputs_names, name)
      free(name);
   free(sc);
}

   E_Randr_Serialized_Setup_11
*_e_randr_create_serialized_setup_11(E_Randr_Screen_Info_11 *screen_info_11)
{
   E_Randr_Serialized_Setup_11 *ss;
   Ecore_X_Randr_Screen_Size_MM* size;

   if (!(ss = malloc(sizeof(*ss)))) return NULL;
   if(!(size = (Ecore_X_Randr_Screen_Size_MM*)eina_list_data_get(eina_list_nth(screen_info_11->sizes, screen_info_11->csize_index)))) goto _e_randr_create_serialized_setup_11_failed_free_ss;
   ss->size.width = size->width;
   ss->size.height = size->height;
   ss->refresh_rate = screen_info_11->current_rate;
   ss->orientation = screen_info_11->corientation;

   return ss;

_e_randr_create_serialized_setup_11_failed_free_ss:
   free(ss);
   return NULL;
}

   E_Randr_Serialized_Setup_11
*_e_randr_update_serialized_setup_11(E_Randr_Serialized_Setup_11 *ss_11, E_Randr_Screen_Info_11 *si_11)
{
   Ecore_X_Randr_Screen_Size_MM* size;

   if (ss_11)
     {
        if(!(size = (Ecore_X_Randr_Screen_Size_MM*)eina_list_data_get(eina_list_nth(si_11->sizes, si_11->csize_index)))) return NULL;
        if (!memcpy(&ss_11->size, size, sizeof(Ecore_X_Randr_Screen_Size_MM)))
          goto _e_randr_update_serialized_setup_11_failed_free_ss;
        ss_11->refresh_rate = si_11->current_rate;
        ss_11->orientation = si_11->corientation;
     }
   else
     ss_11 = _e_randr_create_serialized_setup_11(si_11);

   return ss_11;

_e_randr_update_serialized_setup_11_failed_free_ss:
   free(ss_11);
   return NULL;
}

   E_Randr_Serialized_Setup_12
*_e_randr_create_serialized_setup_12(E_Randr_Screen_Info_12 *screen_info_12)
{
   E_Randr_Serialized_Setup_12 *ss;
   Eina_List *iter;
   E_Randr_Crtc_Info *ci;
   E_Randr_Output_Info *oi;
   E_Randr_Serialized_Crtc *sc;
   E_Randr_Edid_Hash *edid_hash;

   if (!(ss = E_NEW(E_Randr_Serialized_Setup_12, 1))) return NULL;

   ss->timestamp = ecore_time_get();

   //Add CRTCs and their configuration
   EINA_LIST_FOREACH(screen_info_12->crtcs, iter, ci)
     {
        //ignore disabled crtcs for now
        if (!ci->current_mode) continue;

        if (!(sc = _e_randr_create_serialized_crtc(ci))
              || !(ss->serialized_crtcs = eina_list_append(ss->serialized_crtcs, sc)))
          goto _e_randr_create_serialized_setup_12_failed_free_list_ss;
     }

   /*
    * Add EDID hashes of connected and enabled
    * outputs for easier comparison during
    * setup restoration
    */
   EINA_LIST_FOREACH(screen_info_12->outputs, iter, oi)
     {
        if ((oi->connection_status != ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED) || !oi->crtc || !oi->crtc->current_mode)
          continue;
        if (!(edid_hash = _e_randr_create_edid_hash(oi)) || !(ss->serialized_edid_hashes = eina_list_append(ss->serialized_edid_hashes, edid_hash)))
          goto _e_randr_create_serialized_setup_12_failed_free_output_list_crtc_list_ss;
     }

   return ss;

_e_randr_create_serialized_setup_12_failed_free_output_list_crtc_list_ss:
   EINA_LIST_FREE(ss->serialized_edid_hashes, edid_hash)
     {
        if (edid_hash) free(edid_hash);
   }
_e_randr_create_serialized_setup_12_failed_free_list_ss:
   EINA_LIST_FREE(ss->serialized_crtcs, sc)
     {
        _e_randr_free_serialized_crtc(sc);
     }
   free(ss);
   return NULL;
}

E_Randr_Serialized_Setup_12
*_e_randr_find_matching_serialized_setup(Eina_List *setups_12, E_Randr_Screen_Info_12 *si_12)
{
   E_Randr_Serialized_Setup_12 *ss_12;
   Eina_List *setups_iter, *r_iter, *s_iter;
   Eina_Bool found = EINA_FALSE;
   E_Randr_Edid_Hash *edid_hash;
   E_Randr_Output_Info *oi;

   if (!setups_12 || !si_12) return NULL;

   EINA_LIST_FOREACH(setups_12, setups_iter, ss_12)
     {
        EINA_LIST_FOREACH(si_12->outputs, r_iter, oi)
          {
             //skip disconnected/-abled monitors
             if ((oi->connection_status != ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED) || !oi->crtc || !oi->crtc->current_mode)
               continue;
             found = EINA_FALSE;
             EINA_LIST_FOREACH(ss_12->serialized_edid_hashes, s_iter, edid_hash)
               {
                  if (oi->edid_hash.hash == edid_hash->hash)
                    {
                       found = EINA_TRUE;
                       break;
                    }
               }
             if (!found)
               break;
          }
        if (found)
          break;
     }
   if (found)
     return ss_12;

   return NULL;
}

   void
_e_randr_free_serialized_setup_12(E_Randr_Serialized_Setup_12 *ss_12)
{
   E_Randr_Serialized_Crtc *sc;
   E_Randr_Edid_Hash *edid_hash;

   if (!ss_12) return;

   EINA_LIST_FREE(ss_12->serialized_crtcs, sc)
     {
        if (!sc) continue;
        _e_randr_free_serialized_crtc(sc);
     }
   EINA_LIST_FREE(ss_12->serialized_edid_hashes, edid_hash)
      if (edid_hash) free(edid_hash);

   free(ss_12);
}

   Eina_List
*_e_randr_update_serialized_setup_12(Eina_List *setups_12, E_Randr_Screen_Info_12 *si_12)
{
   E_Randr_Serialized_Setup_12 *ss_12;

   if (setups_12)
     {
        /*
         * try to find the setup with the same monitors
         * connected in order to replace it
         */
        if((ss_12 = _e_randr_find_matching_serialized_setup(setups_12, si_12)))
          _e_randr_free_serialized_setup_12(ss_12);
     }
   ss_12 = _e_randr_create_serialized_setup_12(si_12);
   setups_12 = eina_list_append(setups_12, ss_12);

   return setups_12;
}

   E_Randr_Serialized_Setup
*_e_randr_create_serialized_setup(E_Randr_Screen_Info *screen_info)
{
   E_Randr_Serialized_Setup *ss;
   E_Randr_Serialized_Setup_12 *ss_12;

   if (!(ss = E_NEW(E_Randr_Serialized_Setup, 1))) return NULL;
   if ((screen_info->randr_version == ECORE_X_RANDR_1_1) && !(ss->serialized_setup_11 = _e_randr_create_serialized_setup_11(screen_info->rrvd_info.randr_info_11))) goto _e_randr_create_serialized_setup_failed_free_ss;
   if ((screen_info->randr_version >= ECORE_X_RANDR_1_2))
     {
        if (!(ss_12 = _e_randr_create_serialized_setup_12(screen_info->rrvd_info.randr_info_12))
              || !(ss->serialized_setups_12 = eina_list_append(ss->serialized_setups_12, ss_12)))
          goto _e_randr_create_serialized_setup_failed_free_ss;
     }

   return ss;

_e_randr_create_serialized_setup_failed_free_ss:
   free(ss);
   return NULL;
}

   EAPI void
e_randr_store_configuration(E_Randr_Screen_Info *screen_info)
{
   if (E_RANDR_NO_11) return;

   if (!e_config->randr_serialized_setup)
     {
        e_config->randr_serialized_setup = _e_randr_create_serialized_setup(screen_info);
        e_config_save_queue();
        return;
     }

   if (screen_info->randr_version == ECORE_X_RANDR_1_1)
     {
        e_config->randr_serialized_setup->serialized_setup_11 = _e_randr_update_serialized_setup_11(e_config->randr_serialized_setup->serialized_setup_11, screen_info->rrvd_info.randr_info_11);
     }
   else if (screen_info->randr_version >= ECORE_X_RANDR_1_2)
     {
        e_config->randr_serialized_setup->serialized_setups_12 = _e_randr_update_serialized_setup_12(e_config->randr_serialized_setup->serialized_setups_12, screen_info->rrvd_info.randr_info_12);

        //Also, update output policies
        e_config->randr_serialized_setup->serialized_outputs_policies = _e_randr_update_serialized_outputs_policies(screen_info->rrvd_info.randr_info_12, e_config->randr_serialized_setup->serialized_outputs_policies);
     }
   e_config_save_queue();
}

//setup restore functions
   EAPI Eina_Bool
e_randr_try_restore_configuration(E_Randr_Screen_Info *si)
{
   if (!e_config || !e_config->randr_serialized_setup) return EINA_FALSE;

   if (si->randr_version == ECORE_X_RANDR_1_1)
     return _e_randr_try_restore_11(si->rrvd_info.randr_info_11);
   else if (si->randr_version >= ECORE_X_RANDR_1_2)
     return _e_randr_try_restore_12(si->rrvd_info.randr_info_12);

   return EINA_FALSE;
}

   Eina_Bool
_e_randr_try_restore_11(E_Randr_Screen_Info_11 *si_11)
{
   E_Manager *man;
   Eina_List *iter;
   Ecore_X_Randr_Screen_Size_MM *stored_size, *size;
   int i = 0;

   if (!e_config->randr_serialized_setup->serialized_setup_11) return EINA_FALSE;
   stored_size = &e_config->randr_serialized_setup->serialized_setup_11->size;
   EINA_LIST_FOREACH(si_11->sizes, iter, size)
     {
        if ((stored_size->width == size->width)
              && (stored_size->height == size->height)
              && (stored_size->width_mm == size->width_mm)
              && (stored_size->height_mm == size->height_mm))
              {
                 man = e_manager_current_get();
                 return ecore_x_randr_screen_primary_output_size_set(man->root, i);
              }
        i++;
     }

   return EINA_FALSE;
}

   E_Randr_Crtc_Info
*_e_randr_find_matching_crtc(Eina_List *crtcs, E_Randr_Serialized_Crtc *sc)
{
   Eina_List *iter, *s_name_iter, *p_output_iter;
   E_Randr_Crtc_Info *ci;
   E_Randr_Output_Info *oi;
   char *s_output_name;

   EINA_LIST_FOREACH(crtcs, iter, ci)
     {
        if (eina_list_count(ci->possible_outputs) != eina_list_count(sc->possible_outputs_names))
          continue;
        EINA_LIST_FOREACH(sc->possible_outputs_names, s_name_iter, s_output_name)
           {
              EINA_LIST_FOREACH(ci->possible_outputs, p_output_iter, oi)
                {
                   if (!strncmp(s_output_name, oi->name, oi->name_length))
                     return ci;
                }
           }
     }
   return NULL;
}

   Eina_List
*_e_randr_find_matching_outputs(Eina_List *sois, Eina_List *ois)
{
   Eina_List *r_output_iter, *s_output_iter, *list = NULL;
   E_Randr_Output_Info *oi;
   E_Randr_Serialized_Output *so;

   EINA_LIST_FOREACH(sois, s_output_iter, so)
     {
        EINA_LIST_FOREACH(ois, r_output_iter, oi)
          {
             if (so->edid_hash.hash == oi->edid_hash.hash)
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

   Ecore_X_Randr_Mode_Info
*_e_randr_find_matching_mode_info(Eina_List *modes, Ecore_X_Randr_Mode_Info *mode)
{
   Eina_List *iter;
   Ecore_X_Randr_Mode_Info *mi = NULL;

   EINA_LIST_FOREACH(modes, iter, mi)
     {
        if ((mode->width == mi->width)
              && (mode->height == mi->height)
              && (mode->dotClock == mi->dotClock)
              && (mode->hSyncStart == mi->hSyncStart)
              && (mode->hSyncEnd == mi->hSyncEnd)
              && (mode->hTotal == mi->hTotal)
              && (mode->hSkew == mi->hSkew)
              && (mode->vSyncStart == mi->vSyncStart)
              && (mode->vSyncEnd == mi->vSyncEnd)
              && (mode->vTotal == mi->vTotal)
              && (mode->nameLength == mi->nameLength)
              && !strncpy(mode->name, mi->name, mode->nameLength)
              && (mode->modeFlags == mi->modeFlags))
          return mi;
     }
   return NULL;
}

   Eina_Bool
_e_randr_try_restore_12(E_Randr_Screen_Info_12 *si_12)
{
   E_Randr_Serialized_Setup_12 *ss_12;
   E_Randr_Serialized_Crtc *sc;
   E_Randr_Crtc_Info *ci;
   Ecore_X_Randr_Output *outputs_array;
   Ecore_X_Randr_Mode_Info *mi;
   Eina_List *iter, *outputs_list;
   Eina_Bool ret = EINA_TRUE;
   E_Manager *man;

   if(!(ss_12 = _e_randr_find_matching_serialized_setup(e_config->randr_serialized_setup->serialized_setups_12, si_12))) return EINA_FALSE;

   man = e_manager_current_get();

   EINA_LIST_FOREACH(ss_12->serialized_crtcs, iter, sc)
     {
        ci = _e_randr_find_matching_crtc(si_12->crtcs, sc);
        outputs_list = _e_randr_find_matching_outputs(si_12->outputs, sc->serialized_outputs);
        outputs_array = _e_randr_outputs_to_array(outputs_list);
        mi = _e_randr_find_matching_mode_info(si_12->modes, &sc->mode_info);
        ret &= ecore_x_randr_crtc_mode_set(man->root, ci->xid, outputs_array, eina_list_count(outputs_list), mi->xid);
        ret &= ecore_x_randr_crtc_pos_set(man->root, ci->xid, sc->pos.x, sc->pos.y);
     }
   return ret;
}

//Utility functions
   static Ecore_X_Randr_Output *
_e_randr_outputs_to_array(Eina_List *outputs_info)
{
   Ecore_X_Randr_Output *ret = NULL;
   E_Randr_Output_Info *output_info;
   Eina_List *output_iter;
   int i = 0;

   if (!outputs_info || !(ret = malloc(sizeof(Ecore_X_Randr_Output) * eina_list_count(outputs_info)))) return NULL;
   EINA_LIST_FOREACH(outputs_info, output_iter, output_info)
      /* output_info == NULL should _not_ be possible! */
     ret[i++] = output_info ? output_info->xid : Ecore_X_Randr_None;
   return ret;
}

/*
 * Try to enable this output on an unoccupied CRTC. 'Force' in this context
 * means, that if there are only occupied CRTCs, we disable another output to
 * enable this one. If not forced we will - if we don't find an unoccupied CRTC
 * - try to share the output of a CRTC with other outputs already using it
 *   (clone).
 */
static Eina_Bool
_e_randr_try_enable_output(E_Randr_Output_Info *output_info, Eina_Bool force)
{
   if (!output_info) return EINA_FALSE;
   else if (output_info->crtc && output_info->crtc->current_mode) return EINA_TRUE;

   Eina_List *iter, *outputs_list = NULL;
   E_Randr_Crtc_Info *crtc_info, *usable_crtc = NULL;
   E_Randr_Output_Info *primary_output;
   Ecore_X_Randr_Output *outputs;
   Ecore_X_Randr_Mode_Info *mode_info;
   Eina_Bool ret = EINA_FALSE;

   /*
    * Try to find a usable crtc for this output. Either unused or forced.
    */
   EINA_LIST_FOREACH(output_info->possible_crtcs, iter, crtc_info)
     {
        if (crtc_info && (!crtc_info->current_mode || force))
          {
             usable_crtc = crtc_info;
             break;
          }
     }

   /*
    * apparently we don't have a CRTC to make use of the device
    */
   if (!usable_crtc) return EINA_FALSE;

   //get the CRTC we will refer to, dependend on policy
   switch (output_info->policy)
     {
      case ECORE_X_RANDR_OUTPUT_POLICY_NONE:
         return EINA_TRUE;
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
        if ((primary_output = e_randr_screen_info->rrvd_info.randr_info_12->primary_output))
          {
             if (primary_output->crtc && primary_output->crtc->current_mode && eina_list_data_find(output_info->modes, primary_output->crtc->current_mode))
               {
                /*
                 * mode currently used by primary output's CRTC is also supported by the new output
                 */
                  if (_e_randr_outputs_are_clones(output_info, primary_output->crtc->outputs))
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
                       outputs_list = eina_list_clone(primary_output->crtc->outputs);
                       outputs_list = eina_list_append(outputs_list, output_info);
                       outputs = _e_randr_outputs_to_array(outputs_list);
                       ret = ecore_x_randr_crtc_mode_set(e_randr_screen_info->root, primary_output->crtc->xid, outputs, eina_list_count(outputs_list), primary_output->crtc->current_mode->xid);
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
                       ret = ecore_x_randr_crtc_mode_set(e_randr_screen_info->root, usable_crtc->xid, &output_info->xid, 1, primary_output->crtc->current_mode->xid);
                       return (ret && ecore_x_randr_crtc_pos_relative_set(e_randr_screen_info->root, usable_crtc->xid, primary_output->crtc->xid, ECORE_X_RANDR_OUTPUT_POLICY_CLONE, e_randr_screen_info->rrvd_info.randr_info_12->alignment));

                  }
             }
             else
               {
                /*
                 * 2b. geometrically identical mode
                 */
                  if (primary_output->crtc && (mode_info = _e_randr_mode_geo_identical_find(output_info->modes, primary_output->crtc->current_mode)))
                    {
                       ret = ecore_x_randr_crtc_mode_set(e_randr_screen_info->root, usable_crtc->xid, &output_info->xid, 1, mode_info->xid);
                       return (ret && ecore_x_randr_crtc_pos_relative_set(e_randr_screen_info->root, usable_crtc->xid, primary_output->crtc->xid, ECORE_X_RANDR_OUTPUT_POLICY_CLONE, e_randr_screen_info->rrvd_info.randr_info_12->alignment));
                  }
                  /*
                   * 3.  Find the highest resolution mode common to enable on primary output's CRTC and the new one.
                   */
                  if (((outputs_list = eina_list_append(outputs_list, primary_output)) && (outputs_list = eina_list_append(outputs_list, output_info))))
                    {
		       if (primary_output->crtc)
			 {
			    if((mode_info = _e_randr_outputs_common_mode_max_get(outputs_list, primary_output->crtc->current_mode)))
			      {
				 fprintf(stderr, "Will try to set mode: %dx%d for primary and clone.\n", mode_info->width, mode_info->height);
				 ret = ecore_x_randr_crtc_mode_set(e_randr_screen_info->root, primary_output->crtc->xid, ((Ecore_X_Randr_Output*)Ecore_X_Randr_Unset), Ecore_X_Randr_Unset, mode_info->xid);
				 ret = (ret && ecore_x_randr_crtc_mode_set(e_randr_screen_info->root, usable_crtc->xid, &output_info->xid, 1, mode_info->xid));
				 ret = (ret && ecore_x_randr_crtc_pos_relative_set(e_randr_screen_info->root, usable_crtc->xid, primary_output->crtc->xid, ECORE_X_RANDR_OUTPUT_POLICY_CLONE, e_randr_screen_info->rrvd_info.randr_info_12->alignment));
			      }
			 }
                       eina_list_free(outputs_list);
                  }
             }

        }
        else
          fprintf(stderr, "Couldn't get primary output!\n");
         /*
          * 4. FAIL
          */
        break;

      default:
        if ((usable_crtc && (!usable_crtc->current_mode)) || force)
          {
             //enable and position according to used policies
             if(!(mode_info = ((Ecore_X_Randr_Mode_Info*)eina_list_nth(output_info->preferred_modes, 0))))
               {
                  fprintf(stderr, "E_RANDR: Could not enable output(%d), as it has no preferred (and there for none at all) modes.!\n", output_info->xid);
                  ret = EINA_FALSE;
                  break;
               }
             if((ret = ecore_x_randr_crtc_mode_set(e_randr_screen_info->root, usable_crtc->xid, &output_info->xid, 1, mode_info->xid)))
               {
                  usable_crtc->geometry.w = mode_info->width;
                  usable_crtc->geometry.h = mode_info->height;
                  usable_crtc->geometry.x = 0;
                  usable_crtc->geometry.y = 0;

                  ret &= _e_randr_crtc_move_policy(usable_crtc);
                }
           }
     }
   ecore_x_randr_screen_reset(e_randr_screen_info->root);
   return ret;
}

/*
 * updates all crtcs information regarding a new output
 */
static void
_e_randr_crtcs_possible_output_update(E_Randr_Output_Info *output_info)
{
   Eina_List *iter;
   E_Randr_Crtc_Info *crtc_info;
   Ecore_X_Randr_Output *outputs = NULL;
   int noutputs = 0;

   EINA_LIST_FOREACH(e_randr_screen_info->rrvd_info.randr_info_12->crtcs, iter, crtc_info)
     {
        if (!eina_list_data_find(crtc_info->possible_outputs, output_info))
          {
             if ((outputs = ecore_x_randr_crtc_possible_outputs_get(e_randr_screen_info->root, crtc_info->xid, &noutputs)))
               {
                  while (--noutputs >= 0)
                    {
                       if (outputs[noutputs] == output_info->xid)
                         {
                            crtc_info->possible_outputs = eina_list_append(crtc_info->possible_outputs, output_info);
                            break;
                         }
                    }
                  free(outputs);
               }
          }
     }
}

/*
 * setup a crtc's current (possible) outputs references
 */
static void
_e_randr_crtc_outputs_refs_update(E_Randr_Crtc_Info *crtc_info)
{
   Ecore_X_Randr_Output		*outputs;
   E_Randr_Output_Info		*output_info;
   int				i, noutputs;

   //get references to output_info structs which are related to this CRTC
   if (e_randr_screen_info->rrvd_info.randr_info_12->outputs && (outputs = ecore_x_randr_crtc_outputs_get(e_randr_screen_info->root, crtc_info->xid, &noutputs)))
     {
        for(i = 0; i < noutputs; i++)
          {
             if (!(output_info = _e_randr_output_info_get(outputs[i])) || eina_list_data_find(crtc_info->outputs, output_info) || (ecore_x_randr_output_crtc_get(e_randr_screen_info->root, outputs[i]) != crtc_info->xid)) continue;
             if(!(crtc_info->outputs = eina_list_append(crtc_info->outputs, output_info))) fprintf(stderr, "E_RANDR: could not add output(%d) to CRTC's(%d) output list!\n", output_info->xid, crtc_info->xid);
             output_info->crtc = crtc_info;
          }
        free(outputs);
     }
   //get references to possible output_info structs which are related to this CRTC
   if (e_randr_screen_info->rrvd_info.randr_info_12->outputs && (outputs = ecore_x_randr_crtc_possible_outputs_get(e_randr_screen_info->root, crtc_info->xid, &noutputs)))
     {
        for(i = 0; i < noutputs; i++)
          {
             if (!(output_info = _e_randr_output_info_get(outputs[i])) || eina_list_data_find(crtc_info->possible_outputs, output_info)) continue;
             if(!(crtc_info->possible_outputs = eina_list_append(crtc_info->possible_outputs, output_info))) fprintf(stderr, "E_RANDR: could not add output(%d) to CRTC's(%d) possible output list!\n", output_info->xid, crtc_info->xid);
          }
        free(outputs);
     }
}

/*
 * reconfigure screen setup according to policy. This is only required if all
 * CRTCs' positions might be affected by the another screens' movement. This includes 'virtual' moves,
 * which means that e.g. when a crtc should be placed at a position < 0, all
 * other crtcs are accordingly moved instead, so the result is the same.
 */
static Eina_Bool
_e_randr_crtc_move_policy(E_Randr_Crtc_Info *new_crtc)
{
   const E_Randr_Crtc_Info *crtc_rel;
   E_Randr_Output_Info *last_output = NULL;
   int dx = Ecore_X_Randr_None, dy = Ecore_X_Randr_None;
   Eina_Bool ret = EINA_TRUE;

   //use the policy of the new crtc's last output
   last_output = (E_Randr_Output_Info*)eina_list_data_get(eina_list_last(new_crtc->outputs));
   if (!last_output) return EINA_FALSE;
   //get the crtc we will place our's relative to. If it's NULL, this is the
   //only output attached, work done.
   if(!(crtc_rel = _e_randr_policy_crtc_get(new_crtc, NULL, last_output->policy))) return EINA_TRUE;

   //following is policy dependend.
   switch (last_output->policy)
     {
      case ECORE_X_RANDR_OUTPUT_POLICY_ABOVE:
        dy = (crtc_rel->geometry.y - new_crtc->geometry.h);
        if (dy < 0)
          {
             //virtual move (move other CRTCs as nessesary)
             dy = -dy;
             ret = ecore_x_randr_move_all_crtcs_but(e_randr_screen_info->root,
                                                    &new_crtc->xid,
                                                    1,
                                                    dx,
                                                    dy);
           }
        break;
      case ECORE_X_RANDR_OUTPUT_POLICY_LEFT:
        dx = (crtc_rel->geometry.x - new_crtc->geometry.w);
        if (dx < 0)
          {
             //virtual move (move other CRTCs as nessesary)
             dx = -dx;
             ret = ecore_x_randr_move_all_crtcs_but(e_randr_screen_info->root,
                                                    &new_crtc->xid,
                                                    1,
                                                    dx,
                                                    dy);
           }
        break;
      default:
        break;
     }
   ret &= ecore_x_randr_crtc_pos_relative_set(e_randr_screen_info->root, new_crtc->xid, crtc_rel->xid, last_output->policy, e_randr_screen_info->rrvd_info.randr_info_12->alignment);
   return ret;
}

/*
 * returns the highest resolution mode common ammongst the given outputs,
 * optionally limited by max_size_mode. If none is found, NULL is returned.
 */
static Ecore_X_Randr_Mode_Info *
_e_randr_outputs_common_mode_max_get(Eina_List *outputs, Ecore_X_Randr_Mode_Info *max_size_mode)
{
   Eina_List *all_modes = NULL, *iter, *output_iter, *right;
   E_Randr_Output_Info *output_info;
   Ecore_X_Randr_Mode_Info *mode_info;
   int outputs_mode_found;

   //create a list of all available modes
   EINA_LIST_FOREACH(outputs, iter, output_info)
     {
        right = eina_list_clone(output_info->modes);
        all_modes = eina_list_merge(all_modes, right);
     }

   if (max_size_mode)
     {
        //remove all modes that are larger than max_size_mode
        EINA_LIST_FOREACH(all_modes, iter, mode_info)
          {
             if (_modes_size_sort_cb((void*)max_size_mode, (void*)mode_info) < 0)
               all_modes = eina_list_remove(all_modes, mode_info);
          }
     }

   //sort modes by their sizes
   all_modes = eina_list_sort(all_modes, eina_list_count(all_modes), _modes_size_sort_cb);
   EINA_LIST_REVERSE_FOREACH(all_modes, iter, mode_info)
     {
        outputs_mode_found = 0;
        EINA_LIST_FOREACH(outputs, output_iter, output_info)
          {
             if (eina_list_data_find(output_info->modes, mode_info))
               outputs_mode_found++;
          }
        if (outputs_mode_found == (int)eina_list_count(outputs))
          break;
        mode_info = NULL;
     }
   return mode_info;
}

/*
static int
_crtcs_size_sort_cb(const void *d1, const void *d2)
{
   E_Randr_Crtc_Info *crtc1 = ((E_Randr_Crtc_Info*)d1), *crtc2 = ((E_Randr_Crtc_Info*)d2);

   return ((crtc1->geometry.w * crtc1->geometry.h) - (crtc2->geometry.w * crtc2->geometry.h));
}
*/

static int
_outputs_size_sort_cb(const void *d1, const void *d2)
{
   E_Randr_Output_Info *output1 = ((E_Randr_Output_Info*)d1), *output2 = ((E_Randr_Output_Info*)d2);

   return (output1 && output1->crtc && output1->crtc->current_mode && output2 && output2->crtc && output2->crtc->current_mode) ?  ((output1->crtc->current_mode->width * output1->crtc->current_mode->height) - (output2->crtc->current_mode->width * output2->crtc->current_mode->height)) : 0;
}

static int
_modes_size_sort_cb(const void *d1, const void *d2)
{
   Ecore_X_Randr_Mode_Info *mode1 = ((Ecore_X_Randr_Mode_Info*)d1), *mode2 = ((Ecore_X_Randr_Mode_Info*)d2);

   return ((mode1->width * mode1->height) - (mode2->width * mode2->height));
}

/*
 * returns a mode within a given list of modes that is gemetrically identical.
 * If none is found, NULL is returned.
 */
static Ecore_X_Randr_Mode_Info *
_e_randr_mode_geo_identical_find(Eina_List *modes, Ecore_X_Randr_Mode_Info *mode)
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

/*
 * reconfigures a CRTC enabling the highest resolution amongst its outputs,
 * without touching any other CRTC currently activated
 */
static Eina_Bool
_e_randr_crtc_outputs_mode_max_set(E_Randr_Crtc_Info *crtc_info)
{
   Ecore_X_Randr_Mode_Info *mode_info;
   Eina_List *iter;
   Eina_Bool ret = EINA_TRUE;
   Ecore_X_Randr_Output *outputs;

   if (!crtc_info || !crtc_info->outputs || !crtc_info->outputs_common_modes) return EINA_FALSE;

   EINA_LIST_REVERSE_FOREACH(crtc_info->outputs_common_modes, iter, mode_info)
     {
        if (!_e_randr_crtc_mode_intersects_crtcs(crtc_info, mode_info))
          break;
     }
   if (!mode_info)
     {
        eina_list_free(crtc_info->outputs_common_modes);
        return EINA_FALSE;
     }
   if ((outputs = _e_randr_outputs_to_array(crtc_info->outputs)))
     {
        ret = ecore_x_randr_crtc_mode_set(e_randr_screen_info->root, crtc_info->xid, outputs, eina_list_count(crtc_info->outputs), mode_info->xid);
        free(outputs);
     }
   eina_list_free(crtc_info->outputs_common_modes);

   ecore_x_randr_screen_reset(e_randr_screen_info->root);

   return ret;
}

/*
 * returns EINA_TRUE if given CRTC would intersect with other CRTCs if set to
 * given mode
 */
static Eina_Bool
_e_randr_crtc_mode_intersects_crtcs(E_Randr_Crtc_Info *crtc_info, Ecore_X_Randr_Mode_Info *mode)
{
   Eina_List *iter;
   E_Randr_Crtc_Info *tmp;

   EINA_LIST_FOREACH(e_randr_screen_info->rrvd_info.randr_info_12->crtcs, iter, tmp)
     {
        if ((tmp == crtc_info) || 
            ((tmp->geometry.w <= 0) || (tmp->geometry.h <= 0))) 
          continue;
        if (E_INTERSECTS(crtc_info->geometry.x, crtc_info->geometry.y, 
                         mode->width, mode->height, tmp->geometry.x, 
                         tmp->geometry.y, tmp->geometry.w, tmp->geometry.h)
            && ((crtc_info->geometry.x != tmp->geometry.x) && 
                (crtc_info->geometry.y != tmp->geometry.y)))
          return EINA_TRUE;
     }
   return EINA_FALSE;
}

/*
 * returns a list of modes common ammongst the given outputs,
 * optionally limited by max_size_mode. If none are found, NULL is returned.
 */
static Eina_List *
_e_randr_outputs_common_modes_get(Eina_List *outputs, Ecore_X_Randr_Mode_Info *max_size_mode)
{
   Eina_List *common_modes = NULL, *iter, *output_iter, *right;
   E_Randr_Output_Info *output_info;
   Ecore_X_Randr_Mode_Info *mode_info;
   int outputs_mode_found;

   if (!outputs) return NULL;

   //create a list of all available modes
   EINA_LIST_FOREACH(outputs, iter, output_info)
     {
        right = eina_list_clone(output_info->modes);
        common_modes = eina_list_merge(common_modes, right);
     }

   if (max_size_mode)
     {
        //remove all modes that are larger than max_size_mode
        EINA_LIST_FOREACH(common_modes, iter, mode_info)
          {
             if (_modes_size_sort_cb((void*)max_size_mode, (void*)mode_info) < 0)
               common_modes = eina_list_remove(common_modes, mode_info);
          }
     }

   //sort modes desc. by their sizes
   for (iter = eina_list_last(common_modes); iter;)
     {
        mode_info = iter->data;
        iter = iter->prev;
        outputs_mode_found = 0;
        EINA_LIST_FOREACH(outputs, output_iter, output_info)
          {
             if (eina_list_data_find(output_info->modes, mode_info))
               outputs_mode_found++;
          }
        if (outputs_mode_found != (int)eina_list_count(outputs))
          common_modes = eina_list_remove(common_modes, mode_info);
     }
   return common_modes;
}

/*
 * reconfigure all CRTCs that had a given CRTC as a clone
 */
static Eina_Bool
_e_randr_crtcs_clone_crtc_removed(E_Randr_Crtc_Info *former_clone)
{
   Eina_List *iter;
   E_Randr_Crtc_Info *crtc_info;

   if (!former_clone) return EINA_FALSE;

   EINA_LIST_FOREACH(e_randr_screen_info->rrvd_info.randr_info_12->crtcs, iter, crtc_info)
     {
        if ((crtc_info == former_clone) || ((crtc_info->geometry.w <= 0) || (crtc_info->geometry.h <= 0))) continue;
        if ((former_clone->geometry.x == crtc_info->geometry.x) && (former_clone->geometry.y == crtc_info->geometry.y) && (former_clone->geometry.w == crtc_info->geometry.w) && (former_clone->geometry.h == crtc_info->geometry.h))
          {
             if (!_e_randr_crtc_outputs_mode_max_set(crtc_info)) return EINA_FALSE;
          }
     }

   return EINA_TRUE;

}

static void
_e_randr_screen_primary_output_assign(E_Randr_Output_Info *removed)
{
   Eina_List *iter;
   E_Randr_Output_Info *primary_output = NULL, *output_info;

   E_RANDR_NO_OUTPUTS_RET();

   if (e_randr_screen_info->rrvd_info.randr_info_12->primary_output && (removed != e_randr_screen_info->rrvd_info.randr_info_12->primary_output)) return;
   if (!(primary_output = _e_randr_output_info_get(ecore_x_randr_primary_output_get(e_randr_screen_info->root))))
     {
        primary_output = eina_list_data_get(eina_list_last(e_randr_screen_info->rrvd_info.randr_info_12->outputs));

        EINA_LIST_FOREACH(e_randr_screen_info->rrvd_info.randr_info_12->outputs, iter, output_info)
          {
             if (output_info->connection_status == ECORE_X_RANDR_CONNECTION_STATUS_DISCONNECTED || !output_info->crtc || !output_info->crtc->current_mode) continue;
             if ((!primary_output->crtc || !primary_output->crtc->current_mode) || _outputs_size_sort_cb(output_info, primary_output) > 0) primary_output = output_info;
          }
        if (!primary_output->crtc || !primary_output->crtc->current_mode) primary_output = NULL;
     }
   e_randr_screen_info->rrvd_info.randr_info_12->primary_output = primary_output;
}

static void
_e_randr_output_info_hw_info_set(E_Randr_Output_Info *output_info)
{
   Ecore_X_Randr_Output *outputs;
   Ecore_X_Randr_Crtc *crtcs;
   E_Randr_Output_Info *output;
   E_Randr_Crtc_Info *crtc;
   int i, num;

   _e_randr_output_modes_add(output_info);
   output_info->edid = ecore_x_randr_output_edid_get(e_randr_screen_info->root, output_info->xid, &output_info->edid_length);
   if (output_info->edid_length > 0)
     output_info->edid_hash.hash = eina_hash_superfast((char*)output_info->edid, output_info->edid_length);
   //get the outputs we can use on the same CRTC alongside this one.
   if ((outputs = ecore_x_randr_output_clones_get(e_randr_screen_info->root, output_info->xid, &num)))
     {
        for (i = 0; i < num; i++)
          {
             if ((output = _e_randr_output_info_get(outputs[i])))
               output_info->clones = eina_list_append(output_info->clones, output);
          }
        free(outputs);
     }

   //get the CRTCs which are usable with this output.
   if ((crtcs = ecore_x_randr_output_possible_crtcs_get(e_randr_screen_info->root, output_info->xid, &num)))
     {
        for (i = 0; i < num; i++)
          {
             fprintf(stderr, "E_RANDR: possible CRTC: %d\n", crtcs[i]);
             if ((crtc = _e_randr_crtc_info_get(crtcs[i])))
               {
                  fprintf(stderr, "E_RANDR: \tfound the suiting struct at %p\n", crtc);
                  output_info->possible_crtcs = eina_list_append(output_info->possible_crtcs, crtc);
               }
          }
        free(crtcs);
     }
   else
     {
        fprintf(stderr, "E_RANDR: Output %x does not have a single possible CRTC.\n", output_info->xid);
     }
}

/*
 * free the hardware specifig parts of the information
 * removes all traces of an output within the data.
 * @param output_info the output info to be freed.
 */
static void
_e_randr_output_hw_info_free(E_Randr_Output_Info *output_info)
{
   E_Randr_Crtc_Info *crtc_info;
   Eina_List *iter;
   if (!output_info) return;

   if (output_info->modes)
     {
        eina_list_free(output_info->modes);
        output_info->modes = NULL;
     }
   if (output_info->preferred_modes)
     {
        eina_list_free(output_info->preferred_modes);
        output_info->preferred_modes = NULL;
     }
   if (output_info->edid)
     {
        free(output_info->edid);
        output_info->edid = NULL;
     }
   if (output_info->wired_clones)
     {
        eina_list_free(output_info->wired_clones);
        output_info->wired_clones = NULL;
     }
   if (output_info->compatible_outputs)
     {
        eina_list_free(output_info->compatible_outputs);
        output_info->compatible_outputs = NULL;
     }
   if (output_info->possible_crtcs)
     {
        eina_list_free(output_info->possible_crtcs);
        output_info->possible_crtcs = NULL;
     }
   if (output_info->clones)
     {
        eina_list_free(output_info->clones);
        output_info->clones = NULL;
     }

   EINA_LIST_FOREACH(e_randr_screen_info->rrvd_info.randr_info_12->crtcs, iter, crtc_info)
     {
        crtc_info->possible_outputs = eina_list_remove(crtc_info->possible_outputs, output_info);
     }
}

/*
 * checks whether a given output is a common clone of the given list's outputs
 */
static Eina_Bool
_e_randr_outputs_are_clones(E_Randr_Output_Info *output_info, Eina_List *outputs)
{
   E_Randr_Output_Info *output;
   Eina_List *iter;

   if (!outputs || !output_info) return EINA_FALSE;

   EINA_LIST_FOREACH(output_info->clones, iter, output)
     {
        if (!eina_list_data_find(output_info->clones, output))
          return EINA_FALSE;
     }
   return EINA_TRUE;
}
