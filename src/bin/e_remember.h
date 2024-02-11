#ifdef E_TYPEDEFS

EAPI extern int E_EVENT_REMEMBER_UPDATE;
EAPI extern E_Config_DD *e_remember_edd;

typedef struct E_Event_Remember_Update
{
   void *border;
} E_Event_Remember_Update;
typedef struct _E_Remember E_Remember;

#define E_REMEMBER_MATCH_NAME             (1 << 0)
#define E_REMEMBER_MATCH_CLASS            (1 << 1)
#define E_REMEMBER_MATCH_TITLE            (1 << 2)
#define E_REMEMBER_MATCH_ROLE             (1 << 3)
#define E_REMEMBER_MATCH_TYPE             (1 << 4)
#define E_REMEMBER_MATCH_TRANSIENT        (1 << 5)

#define E_REMEMBER_APPLY_POS              (1 << 0)
#define E_REMEMBER_APPLY_SIZE             (1 << 1)
#define E_REMEMBER_APPLY_LAYER            (1 << 2)
#define E_REMEMBER_APPLY_LOCKS            (1 << 3)
#define E_REMEMBER_APPLY_BORDER           (1 << 4)
#define E_REMEMBER_APPLY_STICKY           (1 << 5)
#define E_REMEMBER_APPLY_DESKTOP          (1 << 6)
#define E_REMEMBER_APPLY_SHADE            (1 << 7)
#define E_REMEMBER_APPLY_ZONE             (1 << 8)
#define E_REMEMBER_APPLY_RUN              (1 << 9)
#define E_REMEMBER_APPLY_SKIP_WINLIST     (1 << 10)
#define E_REMEMBER_APPLY_SKIP_PAGER       (1 << 11)
#define E_REMEMBER_APPLY_SKIP_TASKBAR     (1 << 12)
#define E_REMEMBER_APPLY_ICON_PREF        (1 << 13)
#define E_REMEMBER_SET_FOCUS_ON_START     (1 << 14)
#define E_REMEMBER_APPLY_FULLSCREEN       (1 << 15)
#define E_REMEMBER_APPLY_OFFER_RESISTANCE (1 << 16)
#define E_REMEMBER_APPLY_XKB              (1 << 17)

#define E_REMEMBER_INTERNAL_DIALOGS       (1 << 0)
#define E_REMEMBER_INTERNAL_FM_WINS       (1 << 1)

#else
#ifndef E_REMEMBER_H
#define E_REMEMBER_H

struct _E_Remember
{
   int           match;
   int           no_reopen;
   unsigned char apply_first_only;
   unsigned char keep_settings;
   int           used_count;
   const char   *name;
   const char   *class;
   const char   *title;
   const char   *role;
   int           type;
   unsigned char transient;
   int           apply;
   int           max_score;
   struct
   {
      int           pos_x, pos_y;
      int           res_x, res_y;
      int           pos_w, pos_h;
      int           w, h;
      int           layer;
      unsigned int maximize; //just direction

      unsigned char lock_user_location;
      unsigned char lock_client_location;
      unsigned char lock_user_size;
      unsigned char lock_client_size;
      unsigned char lock_user_stacking;
      unsigned char lock_client_stacking;
      unsigned char lock_user_iconify;
      unsigned char lock_client_iconify;
      unsigned char lock_user_desk;
      unsigned char lock_client_desk;
      unsigned char lock_user_sticky;
      unsigned char lock_client_sticky;
      unsigned char lock_user_shade;
      unsigned char lock_client_shade;
      unsigned char lock_user_maximize;
      unsigned char lock_client_maximize;
      unsigned char lock_user_fullscreen;
      unsigned char lock_client_fullscreen;
      unsigned char lock_border;
      unsigned char lock_close;
      unsigned char lock_focus_in;
      unsigned char lock_focus_out;
      unsigned char lock_life;

      const char   *border;

      unsigned char sticky;
      unsigned char shaded;
      unsigned char fullscreen;
      unsigned char skip_winlist;
      unsigned char skip_pager;
      unsigned char skip_taskbar;
      unsigned char icon_preference;
      unsigned char offer_resistance;

      int           desk_x, desk_y;
      int           zone;
      int           head;
      const char   *command;
      const char   *desktop_file;
      int           xkb;
      const char   *cl_name;
      const char   *cl_model;
      const char   *cl_variant;
      const char   *cl_option;
   } prop;
};

EINTERN int      e_remember_init(E_Startup_Mode mode);
EINTERN int      e_remember_shutdown(void);
EAPI E_Remember *e_remember_new(void);
EAPI int         e_remember_usable_get(E_Remember *rem);
EAPI void        e_remember_use(E_Remember *rem);
EAPI void        e_remember_unuse(E_Remember *rem);
EAPI void        e_remember_del(E_Remember *rem);
EAPI E_Remember *e_remember_find(E_Border *bd);
EAPI E_Remember *e_remember_find_usable(E_Border *bd);
EAPI void        e_remember_match_update(E_Remember *rem);
EAPI void        e_remember_update(E_Border *bd);
EAPI int         e_remember_default_match_set(E_Remember *rem, E_Border *bd);
EAPI void        e_remember_internal_save(void);
#endif
#endif
