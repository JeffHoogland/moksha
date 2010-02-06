#ifndef E_ILLUME_H
# define E_ILLUME_H

# include "e.h"

# define E_ILLUME_LAYOUT_API_VERSION 1
# define E_ILLUME_LAYOUT_POLICY_TYPE 0xE0b200b

typedef enum _E_Illume_Animation_Class 
{
   E_ILLUME_ANIM_APP, 
     E_ILLUME_ANIM_KEYBOARD, 
     E_ILLUME_ANIM_TOP_SHELF, 
     E_ILLUME_ANIM_BOTTOM_PANEL, 
     E_ILLUME_ANIM_DIALOG, 
     E_ILLUME_ANIM_QUICKPANEL, 
     E_ILLUME_ANIM_OTHER
} E_Illume_Animation_Class;

typedef struct _E_Illume_Layout_Api E_Illume_Layout_Api;
typedef struct _E_Illume_Layout_Policy E_Illume_Layout_Policy;
typedef struct _E_Illume_Config E_Illume_Config;
typedef struct _E_Illume_Config_Zone E_Illume_Config_Zone;

struct _E_Illume_Layout_Api 
{
   int version;
   const char *label, *name;
};

struct _E_Illume_Layout_Policy 
{
   E_Object e_obj_inherit;

   E_Illume_Layout_Api *api;

   void *handle;

   struct 
     {
        void *(*init) (E_Illume_Layout_Policy *p);
        int (*shutdown) (E_Illume_Layout_Policy *p);
        void (*config) (E_Container *con, const char *params);

        void (*border_add) (E_Border *bd);
        void (*border_del) (E_Border *bd);
        void (*border_focus_in) (E_Border *bd);
        void (*border_focus_out) (E_Border *bd);
        void (*border_activate) (E_Border *bd);
        void (*border_property_change) (E_Border *bd, Ecore_X_Event_Window_Property *event);
        void (*zone_layout) (E_Zone *zone);
        void (*zone_move_resize) (E_Zone *zone);
        void (*drag_start) (E_Border *bd);
        void (*drag_end) (E_Border *bd);
     } funcs;
};

struct _E_Illume_Config 
{
   int version;

   struct 
     {
        struct 
          {
             int duration;
          } kbd, softkey, quickpanel;
     } sliding;

   struct 
     {
        const char *name;
        struct 
          {
             const char *class;
             const char *name;
             const char *title;
             int win_type;
             struct 
               {
                  int class, name, title, win_type;
               } match;
          } vkbd, softkey, home, indicator;
        Eina_List *zones;
     } policy;

   // Not User Configurable. Placeholders
   const char *mod_dir;
   E_Config_Dialog *cfd;
};

struct _E_Illume_Config_Zone 
{
   int id;
   struct 
     {
        int dual, side;
     } mode;
};

EAPI Eina_List *e_illume_layout_policies_get(void);
EAPI E_Illume_Config_Zone *e_illume_zone_config_get(int id);

EAPI int e_illume_border_is_dialog(E_Border *bd);
EAPI int e_illume_border_is_keyboard(E_Border *bd);
EAPI int e_illume_border_is_bottom_panel(E_Border *bd);
EAPI int e_illume_border_is_top_shelf(E_Border *bd);
EAPI int e_illume_border_is_home(E_Border *bd);
EAPI int e_illume_border_is_conformant(E_Border *bd);
EAPI int e_illume_border_is_quickpanel(E_Border *bd);
EAPI int e_illume_border_is_valid(E_Border *bd);

EAPI Eina_List *e_illume_border_valid_borders_get(E_Zone *zone);
EAPI E_Border *e_illume_border_valid_border_get(E_Zone *zone);
EAPI int e_illume_border_valid_count_get(E_Zone *zone);
EAPI Eina_List *e_illume_border_quickpanel_borders_get(E_Zone *zone);
EAPI int e_illume_border_quickpanel_count_get(E_Zone *zone);

EAPI E_Border *e_illume_border_at_xy_get(E_Zone *zone, int x, int y);
EAPI E_Border *e_illume_border_in_region_get(E_Zone *zone, int x, int y, int w, int h);
EAPI E_Border *e_illume_border_keyboard_get(E_Zone *zone);
EAPI E_Border *e_illume_border_top_shelf_get(E_Zone *zone);
EAPI E_Border *e_illume_border_bottom_panel_get(E_Zone *zone);

EAPI void e_illume_border_top_shelf_pos_get(E_Zone *zone, int *x, int *y);
EAPI void e_illume_border_top_shelf_size_get(E_Zone *zone, int *w, int *h);
EAPI void e_illume_border_bottom_panel_pos_get(E_Zone *zone, int *x, int *y);
EAPI void e_illume_border_bottom_panel_size_get(E_Zone *zone, int *w, int *h);

EAPI void e_illume_border_slide_to(E_Border *bd, int x, int y, E_Illume_Animation_Class aclass);
EAPI void e_illume_border_min_get(E_Border *bd, int *mw, int *mh);
EAPI void e_illume_border_max_get(E_Border *bd, int *mw, int *mh);

EAPI void e_illume_border_app1_safe_region_get(E_Zone *zone, int *x, int *y, int *w, int *h);
EAPI void e_illume_border_app2_safe_region_get(E_Zone *zone, int *x, int *y, int *w, int *h);
EAPI void e_illume_kbd_safe_app_region_get(E_Zone *zone, int *x, int *y, int *w, int *h);

extern EAPI E_Illume_Config *il_cfg;
extern EAPI int E_ILLUME_EVENT_POLICY_CHANGE;

#endif
