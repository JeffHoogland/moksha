/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define E_TYPEDEFS 1
#include "evry.h"

#undef E_TYPEDEFS
#include "evry.h"

typedef struct _Config Config;
typedef struct _Source_Config Source_Config;

struct _Config
{
  /* position */
  double rel_x, rel_y;
  /* size */
  int width, height;

  /* generic plugin config */
  Eina_List *sources;

  int scroll_animate;
  double scroll_speed;

  int auto_select_first;
};

struct _Source_Config
{
  const char *name;

  /* minimum input chars to query this source */
  int min_query;
};

  

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

EAPI int  evry_plug_apps_init(void);
EAPI int  evry_plug_apps_shutdown(void);

EAPI int  evry_plug_border_init(void);
EAPI int  evry_plug_border_shutdown(void);

EAPI int  evry_plug_border_act_init(void);
EAPI int  evry_plug_border_act_shutdown(void);

EAPI int  evry_plug_config_init(void);
EAPI int  evry_plug_config_shutdown(void);

EAPI int  evry_plug_tracker_init(void);
EAPI int  evry_plug_tracker_shutdown(void);

EAPI int  evry_plug_dir_browse_init(void);
EAPI int  evry_plug_dir_browse_shutdown(void);

extern Config *evry_conf;

#endif
