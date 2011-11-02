#ifdef E_TYPEDEFS

typedef struct _E_Randr_Crtc_Info E_Randr_Crtc_Info;
typedef struct _E_Randr_Output_Info E_Randr_Output_Info;
typedef struct _E_Randr_Screen_Info_11 E_Randr_Screen_Info_11;
typedef struct _E_Randr_Screen_Info_12 E_Randr_Screen_Info_12;
typedef union _E_Randr_Screen_RRVD_Info E_Randr_Screen_RRVD_Info;
typedef struct _E_Randr_Screen_Info E_Randr_Screen_Info;
typedef struct _E_Randr_Output_Edid_Hash E_Randr_Output_Edid_Hash;
typedef struct _E_Randr_Output_Restore_Info E_Randr_Output_Restore_Info;
typedef struct _E_Randr_Crtc_Restore_Info E_Randr_Crtc_Restore_Info;
typedef struct _E_Randr_Screen_Restore_Info_11 E_Randr_Screen_Restore_Info_11;
typedef struct _E_Randr_Screen_Restore_Info_12 E_Randr_Screen_Restore_Info_12;
typedef union _E_Randr_Screen_Restore_Info_Union E_Randr_Screen_Restore_Info_Union;
typedef struct _E_Randr_Screen_Restore_Info E_Randr_Screen_Restore_Info;

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
   Ecore_X_Randr_Output_Policy output_policy;
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
struct _E_Randr_Output_Edid_Hash {
   int hash;
};

struct _E_Randr_Output_Restore_Info
{
   E_Randr_Output_Edid_Hash edid_hash;
   double backlight_level;
};

struct _E_Randr_Crtc_Restore_Info
{
   Eina_Rectangle geometry;
   Ecore_X_Randr_Orientation orientation;
   //list of the outputs;
   Eina_List *outputs;
};

struct _E_Randr_Screen_Restore_Info_11 
{
   Ecore_X_Randr_Screen_Size size;
   Ecore_X_Randr_Refresh_Rate refresh_rate;
   Ecore_X_Randr_Orientation orientation;
};

struct _E_Randr_Screen_Restore_Info_12 
{
   Eina_List *outputs_edid_hashes;
   int noutputs;
   Eina_List *crtcs;
   Ecore_X_Randr_Output_Policy output_policy;
   Ecore_X_Randr_Relative_Alignment alignment;
};

union _E_Randr_Screen_Restore_Info_Union 
{
   E_Randr_Screen_Restore_Info_11 *restore_info_11;
   Eina_List *restore_info_12;
};

struct _E_Randr_Screen_Restore_Info 
{
   int randr_version;
   E_Randr_Screen_Restore_Info_Union rrvd_restore_info;
};

EINTERN Eina_Bool e_randr_init(void);
EINTERN int e_randr_shutdown(void);

extern E_Randr_Screen_Info *e_randr_screen_info;

#endif
#endif
