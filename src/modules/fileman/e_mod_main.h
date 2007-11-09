/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

/* Increment for Major Changes */
#define MOD_CONFIG_FILE_EPOCH 0x0001
/* Increment for Minor Changes (ie: user doesn't need a new config) */
#define MOD_CONFIG_FILE_GENERATION 0x0101
#define MOD_CONFIG_FILE_VERSION ((MOD_CONFIG_FILE_EPOCH << 16) | MOD_CONFIG_FILE_GENERATION)

typedef struct _Config Config;

#define E_TYPEDEFS 1
#include "e_fwin.h"

#undef E_TYPEDEFS
#include "e_fwin.h"

struct _Config 
{
   int config_version;
   
   struct {
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
   } view;
   /* display of icons */
   struct {
      struct {
	 int           w, h;
      } icon;
      struct {
	 int           w, h;
      } list;
      struct {
	 unsigned char w;
	 unsigned char h;
      } fixed;
      struct {
	 unsigned char show;
      } extension;
      const char      *key_hint;
   } icon;
   /* how to sort files */
   struct {
      struct {
	 unsigned char    no_case;
	 struct {
	    unsigned char first;
	    unsigned char last;
	 } dirs;
      } sort;
   } list;
   /* control how you can select files */
   struct {
      unsigned char    single;
      unsigned char    windows_modifiers;
   } selection;
   /* the background - if any, and how to handle it */
   /* FIXME: not implemented yet */
   struct {
      const char      *background;
      const char      *frame;
      const char      *icons;
      unsigned char    fixed;
   } theme;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

extern Config *fileman_config;

#endif
