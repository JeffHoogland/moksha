#ifdef E_TYPEDEFS

typedef struct _E_Randr_Crtc_Info E_Randr_Crtc_Info;
typedef struct _E_Randr_Edid_Hash E_Randr_Edid_Hash;
typedef struct _E_Randr_Monitor_Info E_Randr_Monitor_Info;
typedef struct _E_Randr_Output_Info E_Randr_Output_Info;
typedef struct _E_Randr_Screen_Info_11 E_Randr_Screen_Info_11;
typedef struct _E_Randr_Screen_Info_12 E_Randr_Screen_Info_12;
typedef union _E_Randr_Screen_RRVD_Info E_Randr_Screen_RRVD_Info;
typedef struct _E_Randr_Screen_Info E_Randr_Screen_Info;
typedef struct _E_Randr_Serialized_Output_Policy E_Randr_Serialized_Output_Policy;
typedef struct _E_Randr_Serialized_Output E_Randr_Serialized_Output;
typedef struct _E_Randr_Serialized_Crtc E_Randr_Serialized_Crtc;
typedef struct _E_Randr_Serialized_Setup_11 E_Randr_Serialized_Setup_11;
typedef struct _E_Randr_Serialized_Setup_12 E_Randr_Serialized_Setup_12;
typedef struct _E_Randr_Serialized_Setup E_Randr_Serialized_Setup;

typedef enum _E_Randr_Configuration_Store_Modifier
{
   E_RANDR_CONFIGURATION_STORE_POLICIES = (1 << 0),
   E_RANDR_CONFIGURATION_STORE_RESOLUTIONS = (1 << 1),
   E_RANDR_CONFIGURATION_STORE_ARRANGEMENT = (1 << 2),
   E_RANDR_CONFIGURATION_STORE_ORIENTATIONS = (1 << 3),
   E_RANDR_CONFIGURATION_STORE_ALL = (
         E_RANDR_CONFIGURATION_STORE_POLICIES
         | E_RANDR_CONFIGURATION_STORE_RESOLUTIONS
         | E_RANDR_CONFIGURATION_STORE_ARRANGEMENT
         | E_RANDR_CONFIGURATION_STORE_ORIENTATIONS)
} E_Randr_Configuration_Store_Modifier;

EAPI void e_randr_store_configuration(E_Randr_Configuration_Store_Modifier modifier);
EAPI void e_randr_11_store_configuration(E_Randr_Configuration_Store_Modifier modifier);

#else
#ifndef E_RANDR_H
#define E_RANDR_H

struct _E_Randr_Crtc_Info
{
   Ecore_X_Randr_Crtc xid;
   Eina_Rectangle geometry;
   Eina_Rectangle panning;
   Eina_Rectangle tracking;
   Eina_Rectangle border;
   Ecore_X_Randr_Orientation current_orientation;
   Ecore_X_Randr_Orientation orientations;
   Ecore_X_Randr_Crtc_Gamma **gamma_ramps;
   int gamma_ramp_size;
   Eina_List *outputs;
   Eina_List *possible_outputs;
   Eina_List *outputs_common_modes;
   Ecore_X_Randr_Mode_Info *current_mode;
};

struct _E_Randr_Edid_Hash
{
   int hash;
};

struct _E_Randr_Monitor_Info
{
   Eina_List *modes;
   Eina_List *preferred_modes;
   Ecore_X_Randr_Screen_Size size_mm;
   unsigned char *edid;
   unsigned long edid_length;
   E_Randr_Edid_Hash edid_hash;
   int max_backlight;
   double backlight_level;
};

struct _E_Randr_Output_Info
{
   Ecore_X_Randr_Output xid;
   Eina_Stringshare *name;
   int name_length;
   E_Randr_Crtc_Info *crtc;
   Eina_List *wired_clones;
   Ecore_X_Randr_Signal_Format signalformats;
   Ecore_X_Randr_Signal_Format signalformat;
   int connector_number;
   Ecore_X_Randr_Connector_Type connector_type;
   Ecore_X_Randr_Connection_Status connection_status;
   Ecore_X_Randr_Output_Policy policy;
   Eina_List *possible_crtcs;
   Eina_List *compatibility_list;
   Ecore_X_Render_Subpixel_Order subpixel_order;
   /*
    * Attached Monitor specific:
    */
   E_Randr_Monitor_Info *monitor;
};

struct _E_Randr_Screen_Info_11
{
   Ecore_X_Randr_Screen_Size_MM *sizes;
   int nsizes;
   int csize_index;
   Ecore_X_Randr_Orientation corientation;
   Ecore_X_Randr_Orientation orientations;
   Ecore_X_Randr_Refresh_Rate **rates;
   int *nrates; // size is nsizes
   Ecore_X_Randr_Refresh_Rate current_rate;
};

struct _E_Randr_Screen_Info_12
{
   Ecore_X_Randr_Screen_Size min_size;
   Ecore_X_Randr_Screen_Size max_size;
   Ecore_X_Randr_Screen_Size current_size;
   Eina_List *modes;
   Eina_List *crtcs;
   Eina_List *outputs;
   E_Randr_Output_Info *primary_output;
   Ecore_X_Randr_Relative_Alignment alignment;
};

//RRVD == RandR(R) Version Depended
union _E_Randr_Screen_RRVD_Info
{
   E_Randr_Screen_Info_11 *randr_info_11;
   E_Randr_Screen_Info_12 *randr_info_12;
};

struct _E_Randr_Screen_Info
{
   Ecore_X_Window root;
   int randr_version;
   E_Randr_Screen_RRVD_Info rrvd_info;
};

//Following stuff is just for configuration purposes

struct _E_Randr_Serialized_Output_Policy
{
   Eina_Stringshare *name;
   Ecore_X_Randr_Output_Policy policy;
};

struct _E_Randr_Serialized_Output
{
   Eina_Stringshare *name;
   double backlight_level;
};

struct _E_Randr_Serialized_Crtc
{
   int index;
   //List of E_Randr_Serialized_Output objects that were used on the same output
   Eina_List *outputs;
   Evas_Coord_Point pos;
   Ecore_X_Randr_Orientation orientation;
   //the serialized mode_info
   Ecore_X_Randr_Mode_Info *mode_info;
};

struct _E_Randr_Serialized_Setup_12
{
   double timestamp;
   //List of E_Randr_Serialized_Crtc objects
   Eina_List *crtcs;
   /*
    * List of E_Randr_Edid_Hash elements of all connected monitors
    */
   Eina_List *edid_hashes;
};

struct _E_Randr_Serialized_Setup_11
{
   Ecore_X_Randr_Screen_Size_MM size;
   Ecore_X_Randr_Refresh_Rate refresh_rate;
   Ecore_X_Randr_Orientation orientation;
};

struct _E_Randr_Serialized_Setup
{
   E_Randr_Serialized_Setup_11 *serialized_setup_11;
   //List of E_Randr_Serialized_Setup_12 objects
   Eina_List *serialized_setups_12;
   //List of E_Randr_Serialized_Output_Policy objects
   Eina_List *outputs_policies;
};

EINTERN Eina_Bool e_randr_init(void);
EAPI Eina_Bool e_randr_screen_info_refresh(void);
EINTERN int e_randr_shutdown(void);
EINTERN Eina_Bool e_randr_try_restore_configuration(void);
EINTERN E_Randr_Serialized_Setup *e_randr_serialized_setup_new(void);
EINTERN void e_randr_serialized_setup_free(E_Randr_Serialized_Setup *ss);
EINTERN void e_randr_11_serialized_setup_free(E_Randr_Serialized_Setup_11 *ss_11);
EINTERN void e_randr_12_serialized_setup_free(E_Randr_Serialized_Setup_12 *ss_12);
EINTERN void e_randr_12_serialized_output_policy_free(E_Randr_Serialized_Output_Policy *policy);

EINTERN Eina_Bool e_randr_12_try_enable_output(E_Randr_Output_Info *output_info, Ecore_X_Randr_Output_Policy policy, Eina_Bool force);
EINTERN void e_randr_12_ask_dialog_new(E_Randr_Output_Info *oi);

EAPI extern E_Randr_Screen_Info e_randr_screen_info;

#endif
#endif
