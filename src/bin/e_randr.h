#ifdef E_TYPEDEFS

typedef struct _E_Randr_Output_Config E_Randr_Output_Config;
typedef struct _E_Randr_Crtc_Config E_Randr_Crtc_Config;
typedef struct _E_Randr_Config E_Randr_Config;

#else
# ifndef E_RANDR_H
#  define E_RANDR_H

#define E_RANDR_CONFIG_FILE_EPOCH 1
#define E_RANDR_CONFIG_FILE_GENERATION 1
#define E_RANDR_CONFIG_FILE_VERSION \
   ((E_RANDR_CONFIG_FILE_EPOCH * 1000000) + E_RANDR_CONFIG_FILE_GENERATION)

struct _E_Randr_Output_Config
{
   unsigned int xid; // ecore_x_randr output id (xid)
   unsigned int crtc; // ecore_x_randr crtc id (xid)
   unsigned int policy; // value of the ecore_x_randr_output_policy
   unsigned char primary; // flag to indicate if primary output
   unsigned long edid_count; // monitor's edid length
   unsigned char *edid; // monitor's edid
   double timestamp; // config timestamp
};

struct _E_Randr_Crtc_Config
{
   unsigned int xid; // ecore_x_randr crtc id (xid)
   int x, y, width, height; // geometry
   unsigned int orient; // value of the ecore_x_randr_orientation
   unsigned int mode; // ecore_x_randr mode id (xid)
   Eina_List *outputs; // list of outputs for this crtc
   double timestamp; // config timestamp
};

struct _E_Randr_Config
{
   /* RANDR CONFIG
    * 
    * Screen:
    *   width, height (int);
    * 
    * list of crtcs
    *   each crtc having:
    *     unsigned int crtc_id (Ecore_X_ID);
    *     int x, y, w, h; (Eina_Rectangle);
    *     unsigned int orientation (Ecore_X_Randr_Orienation);
    *     unsigned int mode_id (Ecore_X_ID);
    *     list of outputs
    *       each output having:
    *         unsigned int output_id (Ecore_X_ID);
    *         unsigned int crtc_id (Ecore_X_ID);
    *         unsigned int output_policy;
    */

   int version; // INTERNAL CONFIG VERSION

   struct
     {
        int width, height; // geometry
        double timestamp; // config timestamp
     } screen;

   Eina_List *crtcs;
};

EINTERN Eina_Bool e_randr_init(void);
EINTERN int e_randr_shutdown(void);

EAPI Eina_Bool e_randr_config_save(void);

extern EAPI E_Randr_Config *e_randr_cfg;

# endif
#endif
