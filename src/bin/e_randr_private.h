#ifdef E_TYPEDEFS

#else
#ifndef E_RANDR_PRIVATE_H
#define E_RANDR_PRIVATE_H

#include "e.h"

#define ECORE_X_RANDR_1_1   ((1 << 16) | 1)
#define ECORE_X_RANDR_1_2   ((1 << 16) | 2)
#define ECORE_X_RANDR_1_3   ((1 << 16) | 3)

#define Ecore_X_Randr_Unset -1
#define Ecore_X_Randr_None   0

#define E_RANDR_11_NO                ((e_randr_screen_info.randr_version < ECORE_X_RANDR_1_1) || !e_randr_screen_info.rrvd_info.randr_info_11)
#define E_RANDR_12_NO                ((e_randr_screen_info.randr_version < ECORE_X_RANDR_1_2) || !e_randr_screen_info.rrvd_info.randr_info_12)
#define E_RANDR_12_NO_CRTCS          (E_RANDR_12_NO || !e_randr_screen_info.rrvd_info.randr_info_12->crtcs)
#define E_RANDR_12_NO_CRTC(crtc)     (E_RANDR_12_NO || !e_randr_screen_info.rrvd_info.randr_info_12->crtcs || (crtc == Ecore_X_Randr_None))
#define E_RANDR_12_NO_OUTPUTS        (E_RANDR_12_NO || !e_randr_screen_info.rrvd_info.randr_info_12->outputs)
#define E_RANDR_12_NO_OUTPUT(output) (E_RANDR_12_NO || !e_randr_screen_info.rrvd_info.randr_info_12->outputs || (output == Ecore_X_Randr_None))
#define E_RANDR_12_NO_MODES          (E_RANDR_12_NO || !e_randr_screen_info.rrvd_info.randr_info_12->modes)
#define E_RANDR_12_NO_MODE(mode)     (E_RANDR_12_NO || !e_randr_screen_info.rrvd_info.randr_info_12->modes || (mode == Ecore_X_Randr_None))

// Generic
Eina_Bool               _try_restore_configuration(void);

// RandRR == 1.1
E_Randr_Screen_Info_11 *_11_screen_info_new(void);
void                    _11_screen_info_free(E_Randr_Screen_Info_11 *screen_info_11);
Eina_Bool               _11_screen_info_refresh(void);
Eina_Bool               _11_try_restore_configuration(void);
void                    _11_store_configuration(E_Randr_Configuration_Store_Modifier modifier);

// RandRR >= 1.2
E_Randr_Screen_Info_12 *_12_screen_info_new(void);
void                    _12_screen_info_free(E_Randr_Screen_Info_12 *screen_info_12);
Eina_Bool               _12_screen_info_refresh(void);
void                    _12_policies_restore(void);
void                    _12_event_listeners_add(void);
void                    _12_event_listeners_remove(void);
// Retrieval functions
Ecore_X_Randr_Mode_Info *_12_screen_info_mode_info_get(const Ecore_X_Randr_Mode mode);
E_Randr_Crtc_Info       *_12_screen_info_crtc_info_get(const Ecore_X_Randr_Crtc crtc);
E_Randr_Output_Info     *_12_screen_info_output_info_get(const Ecore_X_Randr_Output output);
Eina_Bool                _12_screen_info_edid_is_available(const E_Randr_Edid_Hash *hash);
// (Re)store data
E_Randr_Serialized_Setup_12 *_12_serialized_setup_new(void);
void                         _12_serialized_setup_free(E_Randr_Serialized_Setup_12 *ss_12);
Eina_Bool                    _12_try_restore_configuration(void);
void                         _12_store_configuration(E_Randr_Configuration_Store_Modifier modifier);

// >= 1.2 Substructures helper functions
E_Randr_Monitor_Info    *_monitor_info_new(E_Randr_Output_Info *output_info);
void                     _monitor_info_free(E_Randr_Monitor_Info *monitor_info);
void                     _monitor_modes_refs_set(E_Randr_Monitor_Info *mi, Ecore_X_Randr_Output output);
E_Randr_Output_Info     *_output_info_new(Ecore_X_Randr_Output output);
void                     _output_info_free(E_Randr_Output_Info *output_info);
void                     _output_refs_set(E_Randr_Output_Info *output_info);
Ecore_X_Randr_Output    *_outputs_to_array(Eina_List *outputs_info);
Eina_List               *_outputs_common_modes_get(Eina_List *outputs, Ecore_X_Randr_Mode_Info *max_size_mode);

E_Randr_Crtc_Info *      _crtc_info_new(Ecore_X_Randr_Crtc crtc);
void                     _crtc_info_free(E_Randr_Crtc_Info *crtc_info);
void                     _crtc_refs_set(E_Randr_Crtc_Info *crtc_info);
void                     _crtc_outputs_refs_set(E_Randr_Crtc_Info *crtc_info);
const E_Randr_Crtc_Info *_crtc_according_to_policy_get(E_Randr_Crtc_Info *but, Ecore_X_Randr_Output_Policy policy);

#endif
#endif
