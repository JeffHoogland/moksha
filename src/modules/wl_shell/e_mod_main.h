#ifndef E_MOD_MAIN_H
# define E_MOD_MAIN_H

# define SLOGFNS 1

# ifdef SLOGFNS
#  include <stdio.h>
#  define SLOGFN(fl, ln, fn) printf("-E-SHELL: %25s: %5i - %s\n", fl, ln, fn);
# else
#  define SLOGFN(fl, ln, fn)
# endif

# include <wayland-server.h>
# include "e_desktop_shell_protocol.h"

typedef enum _E_Shell_Surface_Type E_Shell_Surface_Type;
typedef struct _E_Shell_Surface E_Shell_Surface;

struct wl_shell
{
   E_Compositor *compositor;
   E_Shell shell;

   E_Layer fullscreen_layer;
   E_Layer panel_layer;
   E_Layer toplevel_layer;
   E_Layer background_layer;
   E_Layer lock_layer;

   Eina_Bool locked : 1;
   Eina_Bool prepare_event_sent : 1;

   E_Shell_Surface *lock_surface;
   struct wl_listener lock_surface_listener;

   struct wl_list backgrounds, panels;
};

enum _E_Shell_Surface_Type
{
   E_SHELL_SURFACE_NONE,
   E_SHELL_SURFACE_PANEL,
   E_SHELL_SURFACE_BACKGROUND,
   E_SHELL_SURFACE_LOCK,
   E_SHELL_SURFACE_SCREENSAVER,
   E_SHELL_SURFACE_TOPLEVEL,
   E_SHELL_SURFACE_TRANSIENT,
   E_SHELL_SURFACE_FULLSCREEN,
   E_SHELL_SURFACE_MAXIMIZED,
   E_SHELL_SURFACE_POPUP
};

struct _E_Shell_Surface
{
   struct wl_resource resource;
   E_Surface *surface;
   struct wl_listener surface_destroy_listener;
   E_Shell_Surface *parent;
   E_Shell_Surface_Type type;
   int sx, sy;
   Eina_Bool saved_pos_valid : 1;
   struct 
     {
        E_Transform transform;
        E_Matrix rotation;
     } rotation;
   struct 
     {
        struct wl_pointer_grab grab;
        unsigned int timestamp;
        int x, y;
        E_Transform parent_transform;
        Eina_Bool initial_up : 1;
     } popup;
   struct 
     {
        enum wl_shell_surface_fullscreen_method type;
        E_Transform transform;
        unsigned int framerate;
        E_Surface *black_surface;
     } fullscreen;

   E_Output *output, *fullscreen_output;
   struct wl_list link;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

#endif
