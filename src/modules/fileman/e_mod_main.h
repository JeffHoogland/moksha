#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "e.h"

/* Increment for Major Changes */
#define MOD_CONFIG_FILE_EPOCH      0x0001
/* Increment for Minor Changes (ie: user doesn't need a new config) */
#define MOD_CONFIG_FILE_GENERATION 0x0117
#define MOD_CONFIG_FILE_VERSION    ((MOD_CONFIG_FILE_EPOCH << 16) | MOD_CONFIG_FILE_GENERATION)

typedef struct _Config Config;

typedef struct Fileman_Path
{
   const char *dev, *path;
   unsigned int zone;
   E_Fm2_View_Mode desktop_mode;
} Fileman_Path;

struct _Config
{
   int config_version;

   struct
   {
      E_Fm2_View_Mode mode;
      unsigned char   open_dirs_in_place;
      unsigned char   selector;
      unsigned char   single_click;
      unsigned char   no_subdir_jump;
      unsigned char   no_subdir_drop;
      unsigned char   always_order;
      unsigned char   link_drop;
      unsigned char   fit_custom_pos;
      unsigned char   show_full_path;
      unsigned char   show_desktop_icons;
      unsigned char   show_toolbar;
      unsigned char   show_sidebar;
      unsigned char   desktop_navigation;
      unsigned char   menu_shows_files;
      int spring_delay;
   } view;
   struct
   {
      double delay;
      double size;
      Eina_Bool enable;
   } tooltip;
   /* display of icons */
   struct
   {
      struct
      {
         int w, h;
      } icon;
      struct
      {
         int w, h;
      } list;
      struct
      {
         unsigned char w;
         unsigned char h;
      } fixed;
      struct
      {
         unsigned char show;
      } extension;
      const char *key_hint;
      unsigned int max_thumb_size;
   } icon;
   /* how to sort files */
   struct
   {
      struct
      {
         unsigned char no_case;
         unsigned char extension;
         unsigned char size;
         unsigned char mtime;
         struct
         {
            unsigned char first;
            unsigned char last;
         } dirs;
      } sort;
   } list;
   /* control how you can select files */
   struct
   {
      unsigned char single;
      unsigned char windows_modifiers;
   } selection;
   /* the background - if any, and how to handle it */
   /* FIXME: not implemented yet */
   struct
   {
      const char   *background;
      const char   *frame;
      const char   *icons;
      unsigned char fixed;
   } theme;
   Eina_List *paths; // Fileman_Path
};

extern Config *fileman_config;
Fileman_Path *e_mod_fileman_path_find(E_Zone *zone);

E_Menu *e_mod_menu_add(E_Menu *m, const char *path);

E_Config_Dialog *e_int_config_fileman(E_Container *con, const char *params __UNUSED__);
E_Config_Dialog *e_int_config_mime_edit(E_Config_Mime_Icon *data, void *data2);
E_Config_Dialog *e_int_config_mime(E_Container *con, const char *params __UNUSED__);
void e_int_config_mime_edit_done(void *data);

void e_fileman_dbus_init(void);
void e_fileman_dbus_shutdown(void);

int  e_fwin_init          (void);
int  e_fwin_shutdown      (void);
void e_fwin_new           (E_Container *con, const char *dev, const char *path);
void e_fwin_zone_new      (E_Zone *zone, void *path);
void e_fwin_zone_shutdown (E_Zone *zone);
void e_fwin_all_unsel     (void *data);
void e_fwin_reload_all    (void);
int  e_fwin_zone_find     (E_Zone *zone);

Eina_Bool e_fwin_nav_init(void);
Eina_Bool e_fwin_nav_shutdown(void);

/**
 * @addtogroup Optional_Fileman
 * @{
 *
 * @defgroup Module_Fileman File Manager
 *
 * Basic file manager with list and grid view, shows thumbnails, can
 * copy, cut, paste, delete and rename files.
 *
 * @see Module_Fileman_Opinfo
 * @}
 */

#endif
