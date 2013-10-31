#ifdef E_TYPEDEFS

typedef struct _E_Randr_Output_Config E_Randr_Output_Config;
typedef struct _E_Randr_Crtc_Config E_Randr_Crtc_Config;
typedef struct _E_Randr_Config E_Randr_Config;

#else
# ifndef E_RANDR_H
#  define E_RANDR_H

#define E_RANDR_VERSION_1_1 ((1 << 16) | 1)
#define E_RANDR_VERSION_1_2 ((1 << 16) | 2)
#define E_RANDR_VERSION_1_3 ((1 << 16) | 3)
#define E_RANDR_VERSION_1_4 ((1 << 16) | 4)

#define E_RANDR_CONFIG_FILE_EPOCH 1
#define E_RANDR_CONFIG_FILE_GENERATION 4
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
   unsigned int *clones; // array of clones (each element of type ecore_x_randr output id (xid)
   unsigned long clone_count; // number of clones
   unsigned char connected; // connection status 0 == connected, 1 == disconnected
   unsigned char exists; // is this output present in X ?
};

struct _E_Randr_Crtc_Config
{
   unsigned int xid; // ecore_x_randr crtc id (xid)
   int x, y, width, height; // geometry
   unsigned int orient; // value of the ecore_x_randr_orientation
   unsigned int mode; // ecore_x_randr mode id (xid)
   unsigned char exists; // is this crtc present in X ?
   Eina_List *outputs; // list of outputs for this crtc
};

struct _E_Randr_Config
{
   int version; // INTERNAL CONFIG VERSION

   struct
     {
        int width, height; // geometry
     } screen;

   Eina_List *crtcs;

   int poll_interval;
   unsigned char restore;
   unsigned long config_timestamp;
   int primary;
};

EINTERN Eina_Bool e_randr_init(void);
EINTERN int e_randr_shutdown(void);

EAPI Eina_Bool e_randr_config_save(void);

extern EAPI E_Randr_Config *e_randr_cfg;

# endif
#endif
