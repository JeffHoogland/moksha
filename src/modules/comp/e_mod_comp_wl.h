#ifdef E_TYPEDEFS
#else
# ifndef E_MOD_COMP_WL_H
#  define E_MOD_COMP_WL_H

#  include <pixman.h>
#  include <GLES2/gl2.h>
#  include <GLES2/gl2ext.h>
#  include <EGL/egl.h>
#  include <EGL/eglext.h>
#  include <wayland-server.h>

//# define LOGFNS 1

# ifdef LOGFNS
#  include <stdio.h>
#  define LOGFN(fl, ln, fn) printf("-E-COMP-WL: %25s: %5i - %s\n", fl, ln, fn);
# else
#  define LOGFN(fl, ln, fn)
# endif

typedef enum _Wayland_Visual Wayland_Visual;
typedef enum _Wayland_Shell_Surface_Type Wayland_Shell_Surface_Type;
typedef struct _Wayland_Frame_Callback Wayland_Frame_Callback;
typedef struct _Wayland_Surface Wayland_Surface;
typedef struct _Wayland_Shell Wayland_Shell;
typedef struct _Wayland_Shell_Surface Wayland_Shell_Surface;
typedef struct _Wayland_Compositor Wayland_Compositor;
typedef struct _Wayland_Mode Wayland_Mode;
typedef struct _Wayland_Output Wayland_Output;
typedef struct _Wayland_Input Wayland_Input;
typedef struct _Wayland_Region Wayland_Region;

enum _Wayland_Visual
{
   WAYLAND_NONE_VISUAL,
   WAYLAND_ARGB_VISUAL,
   WAYLAND_RGB_VISUAL
};

enum _Wayland_Shell_Surface_Type
{
   SHELL_SURFACE_NONE,
   SHELL_SURFACE_PANEL,
   SHELL_SURFACE_BACKGROUND,
   SHELL_SURFACE_LOCK,
   SHELL_SURFACE_SCREENSAVER,
   SHELL_SURFACE_TOPLEVEL,
   SHELL_SURFACE_TRANSIENT,
   SHELL_SURFACE_FULLSCREEN,
   SHELL_SURFACE_POPUP
};

struct _Wayland_Frame_Callback
{
   struct wl_resource resource;
   struct wl_list link;
};

struct _Wayland_Surface
{
   struct wl_surface surface;
   struct wl_buffer *buffer;
   struct wl_list link;
   struct wl_list buffer_link;
   struct wl_listener buffer_destroy_listener;
   struct wl_list frame_callbacks;

   pixman_region32_t damage, opaque, clip, input;
   GLuint texture, saved_texture;

   int32_t x, y, w, h;
   uint32_t visual, pitch;

   EGLImageKHR image;

   E_Win *win;
   Ecore_X_Window input_win;
};

struct _Wayland_Shell
{
   void (*lock) (Wayland_Shell *shell);
   void (*unlock) (Wayland_Shell *shell);
   void (*map) (Wayland_Shell *shell, Wayland_Surface *surface, int32_t width, int32_t height);
   void (*configure) (Wayland_Shell *shell, Wayland_Surface *surface, int32_t x, int32_t y, int32_t width, int32_t height);
   void (*destroy) (Wayland_Shell *shell);
};

struct _Wayland_Shell_Surface 
{
   struct wl_resource resource;
   Wayland_Surface *surface;
   struct wl_listener surface_destroy_listener;
   Wayland_Shell_Surface *parent;
   struct wl_list link;
   int32_t saved_x, saved_y;
   Wayland_Shell_Surface_Type type;

   char *title, *clas;

   struct 
     {
        struct wl_pointer_grab grab;
        uint32_t timestamp;
        int32_t x, y;
     } popup;
};

struct _Wayland_Compositor
{
   struct wl_list surfaces;

   struct 
     {
        EGLDisplay display;
        EGLContext context;
        EGLConfig config;
     } egl;

   PFNGLEGLIMAGETARGETRENDERBUFFERSTORAGEOESPROC
     image_target_renderbuffer_storage;
   PFNGLEGLIMAGETARGETTEXTURE2DOESPROC image_target_texture_2d;
   PFNEGLCREATEIMAGEKHRPROC create_image;
   PFNEGLDESTROYIMAGEKHRPROC destroy_image;
   PFNEGLBINDWAYLANDDISPLAYWL bind_display;
   PFNEGLUNBINDWAYLANDDISPLAYWL unbind_display;
   Eina_Bool has_bind : 1;

   void (*destroy) (void);
};

struct _Wayland_Mode
{
   uint32_t flags, refresh;
   int32_t w, h;
   struct wl_list link;
};

struct _Wayland_Output
{
   int32_t x, y, w, h;
   char *make, *model;
   uint32_t subpixel, flags;

   Wayland_Mode mode;

   struct wl_list link;
   struct wl_list frame_callbacks;
};

struct _Wayland_Input
{
   struct wl_input_device input_device;
   int32_t hotspot_x, hotspot_y;
   struct wl_list link;
   uint32_t modifier_state;
};

struct _Wayland_Region
{
   struct wl_resource resource;
   pixman_region32_t region;
};

struct wl_shell
{
   Wayland_Shell shell;

   Eina_Bool locked : 1;
   Eina_Bool prepare_event_sent : 1;

//   Wayland_Shell_Surface *lock_surface;
   struct wl_listener lock_surface_listener;
   struct wl_list hidden_surfaces;

   struct 
     {
        struct wl_resource *desktop_shell;
        struct wl_client *client;
     } child;
};

Eina_Bool e_mod_comp_wl_init(void);
void e_mod_comp_wl_shutdown(void);
uint32_t e_mod_comp_wl_time_get(void);
Ecore_X_Pixmap e_mod_comp_wl_pixmap_get(Ecore_X_Window win);

extern struct wl_display *_wl_disp;

# endif
#endif
