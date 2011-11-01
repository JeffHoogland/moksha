#ifdef E_TYPEDEFS

typedef struct _E_Randr_Crtc_Info E_Randr_Crtc_Info;
typedef struct _E_Randr_Edid_Hash E_Randr_Edid_Hash;
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

EAPI void e_randr_store_configuration(E_Randr_Screen_Info *screen_info);

#else
#ifndef E_RANDR_H
#define E_RANDR_H

struct _E_Randr_Crtc_Info
{
   Ecore_X_ID xid;
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

struct _E_Randr_Output_Info
{
   Ecore_X_ID xid;
   char *name;
   int name_length;
   E_Randr_Crtc_Info *crtc;
   Eina_List *wired_clones;
   Ecore_X_Randr_Signal_Format signalformats;
   Ecore_X_Randr_Signal_Format signalformat;
   int connector_number;
   Ecore_X_Randr_Connector_Type connector_type;
   Ecore_X_Randr_Connection_Status connection_status;
   Ecore_X_Randr_Output_Policy policy;
   /*
    * Attached Monitor specific:
    */
   Eina_List *modes;
   Eina_List *preferred_modes;
   Eina_List *clones;
   Eina_List *possible_crtcs;
   Ecore_X_Randr_Screen_Size size_mm;
   unsigned char *edid;
   unsigned long edid_length;
   E_Randr_Edid_Hash edid_hash;
   int max_backlight;
   double backlight_level;
   Ecore_X_Render_Subpixel_Order subpixel_order;
   Eina_List *compatible_outputs;
};

struct _E_Randr_Screen_Info_11
{
   //List of Ecore_X_Randr_Screen_Size_MM*
   Eina_List *sizes;
   int csize_index;
   Ecore_X_Randr_Orientation corientation;
   Ecore_X_Randr_Orientation orientations;
   //List of Ecore_X_Randr_Refresh_Rate*
   Eina_List *rates;
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
   char *name;
   int name_length;
   Ecore_X_Randr_Output_Policy policy;
};

struct _E_Randr_Serialized_Output
{
   char *name;
   int name_length;
   E_Randr_Edid_Hash edid_hash;
   double backlight_level;
};

struct _E_Randr_Serialized_Crtc
{
   //List of E_Randr_Serialized_Output objects that were used on the same output
   Eina_List *serialized_outputs;
   //the serialized mode_info misses its xid value
   Ecore_X_Randr_Mode_Info mode_info;
   Evas_Coord_Point pos;
   //List of all possible outputs' names
   //e.g. "LVDS", "CRT-0", "VGA"
   Eina_List *possible_outputs_names;
   Ecore_X_Randr_Orientation orientation;
};

struct _E_Randr_Serialized_Setup_12
{
   double timestamp;
   //List of E_Randr_Serialized_Crtc objects
   Eina_List *serialized_crtcs;
   /*
    * List of E_Randr_Edid_Hash elements of monitors,
    * that were enabled, when the setup was stored
    */
   Eina_List *serialized_edid_hashes;
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
   Eina_List *serialized_outputs_policies;
};

EINTERN Eina_Bool e_randr_init(void);
EINTERN int e_randr_shutdown(void);

EAPI extern E_Randr_Screen_Info *e_randr_screen_info;

#endif
#endif
