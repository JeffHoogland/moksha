#ifndef E_MOD_COMP_CFDATA_H
#define E_MOD_COMP_CFDATA_H

typedef struct _Config Config;
typedef struct _Match  Match;

struct _Config
{
   const char   *shadow_file;
   const char   *shadow_style;
   int           engine;
   int           max_unmapped_pixels;
   int           max_unmapped_time;
   int           min_unmapped_time;
   int           fps_average_range;
   unsigned char fps_corner;
   unsigned char fps_show;
   unsigned char use_shadow;
   unsigned char indirect;
   unsigned char texture_from_pixmap;
   unsigned char lock_fps;
   unsigned char loose_sync;
   unsigned char efl_sync;
   unsigned char grab;
   unsigned char vsync;
   unsigned char keep_unmapped;
   unsigned char send_flush;
   unsigned char send_dump;
   unsigned char nocomp_fs;
   unsigned char smooth_windows;
   double        first_draw_delay;

   struct
   {
      Eina_List *popups;    // used for e popups
      Eina_List *borders;    // used for borders
      Eina_List *overrides;    // used for client menus, tooltips etc.
      Eina_List *menus;    // used for e menus
   } match;
};

struct _Match
{
   const char *title; // glob - used for borders, NULL if not to be used
   const char *name; // glob - used for borders, overrides, popups, NULL if not to be used
   const char *clas; // glob - used for borders, overrides, NULL if not to be used
   const char *role; // glob - used for borders

   const char *shadow_style; // shadow style to use

   int         primary_type; // Ecore_X_Window_Type - used for borders, overrides, first one found - ECORE_X_WINDOW_TYPE_UNKNOWN if not to be used
   char        borderless; // used for borders, 0 == dont use, 1 == borderless, -1 == not borderless
   char        dialog; // used for borders, 0 == don't use, 1 == dialog, -1 == not dialog
   char        accepts_focus; // used for borders, 0 == don't use, 1 == accepts focus, -1 == does not accept focus
   char        vkbd; // used for borders, 0 == don't use, 1 == is vkbd, -1 == not vkbd
   char        quickpanel; // used for borders, 0 == don't use, 1 == is quickpanel, -1 == not quickpanel
   char        argb; // used for borders, overrides, popups, menus, 0 == don't use, 1 == is argb, -1 == not argb
   char        fullscreen; // used for borders, 0 == don't use, 1 == is fullscreen, -1 == not fullscreen
   char        modal; // used for borders, 0 == don't use, 1 == is modal, -1 == not modal
};

EAPI void    e_mod_comp_cfdata_edd_init(E_Config_DD **conf_edd, E_Config_DD **match_edd);
EAPI Config *e_mod_comp_cfdata_config_new(void);
EAPI void    e_mod_cfdata_config_free(Config *cfg);

#endif
