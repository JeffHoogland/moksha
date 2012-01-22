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

# define LOGFNS 1

# ifdef LOGFNS
#  include <stdio.h>
#  define LOGFN(fl, ln, fn) printf("-E-COMP-WL: %25s: %5i - %s\n", fl, ln, fn);
# else
#  define LOGFN(fl, ln, fn)
# endif

typedef enum _Wayland_Shell_Surface_Type Wayland_Shell_Surface_Type;
typedef enum _Wayland_Visual Wayland_Visual;
typedef struct _Wayland_Matrix Wayland_Matrix;
typedef struct _Wayland_Vector Wayland_Vector;
typedef struct _Wayland_Shader Wayland_Shader;
typedef struct _Wayland_Mode Wayland_Mode;
typedef struct _Wayland_Compositor Wayland_Compositor;
typedef struct _Wayland_Output Wayland_Output;
typedef struct _Wayland_Frame_Callback Wayland_Frame_Callback;
typedef struct _Wayland_Transform Wayland_Transform;
typedef struct _Wayland_Surface Wayland_Surface;
typedef struct _Wayland_Input Wayland_Input;
typedef struct _Wayland_Shell_Surface Wayland_Shell_Surface;
typedef struct _Wayland_Shell Wayland_Shell;

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

enum _Wayland_Visual
{
   WAYLAND_NONE_VISUAL,
   WAYLAND_ARGB_VISUAL,
   WAYLAND_RGB_VISUAL
};

struct _Wayland_Matrix
{
   GLfloat d[16];
};

struct _Wayland_Vector
{
   GLfloat f[4];
};

struct _Wayland_Shader
{
   GLuint program;
   GLuint vertex_shader;
   GLuint fragment_shader;
   GLuint proj_uniform;
   GLuint tex_uniform;
   GLuint alpha_uniform;
   GLuint color_uniform;
};

struct _Wayland_Mode
{
   uint32_t flags, refresh;
   int32_t width, height;
   struct wl_list link;
};

struct _Wayland_Compositor
{
   struct wl_shm *shm;

   struct 
     {
        EGLDisplay display;
        EGLContext context;
        EGLConfig config;
     } egl;

   Wayland_Shader texture_shader;
   Wayland_Shader solid_shader;

   struct wl_array vertices, indices;
   struct wl_list surfaces;

   struct wl_event_source *idle_source;

   PFNEGLBINDWAYLANDDISPLAYWL bind_display;
   PFNEGLUNBINDWAYLANDDISPLAYWL unbind_display;
   Eina_Bool has_bind : 1;

   uint32_t current_alpha;

   void (*destroy) (void);
};

struct _Wayland_Output
{
   int x, y, width, height;
   char *make, *model;
   uint32_t subpixel;

   EGLSurface egl_surface;

   Wayland_Mode mode;
   Wayland_Matrix matrix;

   Eina_Bool repaint_needed : 1;
   Eina_Bool repaint_scheduled : 1;

   struct wl_event_source *finish_frame_timer;

   struct wl_list link;
   struct wl_list frame_callbacks;

   pixman_region32_t region, prev_damage;

   struct wl_buffer *scanout_buffer;
   struct wl_listener scanout_buffer_destroy_listener;
   struct wl_buffer *pending_scanout_buffer;
   struct wl_listener pending_scanout_buffer_destroy_listener;

   int (*prepare_render) (Wayland_Output *output);
   int (*present) (Wayland_Output *output);
   int (*prepare_scanout_surface) (Wayland_Output *output, Wayland_Surface *ws);
};

struct _Wayland_Frame_Callback
{
   struct wl_resource resource;
   struct wl_list link;
};

struct _Wayland_Transform
{
   Wayland_Matrix matrix, inverse;
};

struct _Wayland_Surface
{
   struct wl_surface surface;
   GLuint texture, saved_texture;
   pixman_region32_t damage, opaque;
   int32_t x, y, width, height;
   int32_t pitch;

   Wayland_Transform *transform;
   uint32_t visual, alpha;

   struct wl_list link;
   struct wl_list buffer_link;
   struct wl_list frame_callbacks;

   struct wl_buffer *buffer;
   struct wl_listener buffer_destroy_listener;
};

struct _Wayland_Input
{
   struct wl_input_device input_device;
   Wayland_Surface *sprite;
   int32_t hotspot_x, hotspot_y;
   struct wl_list link;
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
   Wayland_Shell_Surface_Type type;

   int32_t saved_x, saved_y;

   struct 
     {
        struct wl_grab grab;
        uint32_t timestmap;
        int32_t x, y;
        int32_t initial_up;
     } popup;

   struct wl_list link;
};

struct wl_shell
{
   Wayland_Shell shell;

   Eina_Bool locked : 1;
   Eina_Bool prepare_event_sent : 1;

   Wayland_Shell_Surface *lock_surface;
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

void e_mod_comp_wl_matrix_init(Wayland_Matrix *matrix);
void e_mod_comp_wl_matrix_translate(Wayland_Matrix *matrix, GLfloat x, GLfloat y, GLfloat z);
void e_mod_comp_wl_matrix_scale(Wayland_Matrix *matrix, GLfloat x, GLfloat y, GLfloat z);
void e_mod_comp_wl_matrix_multiply(Wayland_Matrix *m, const Wayland_Matrix *n);
void e_mod_comp_wl_matrix_transform(Wayland_Matrix *matrix, Wayland_Vector *v);
void e_mod_comp_wl_buffer_attach(struct wl_buffer *buffer, struct wl_surface *surface);
void e_mod_comp_wl_buffer_post_release(struct wl_buffer *buffer);

extern struct wl_display *_wl_disp;

# endif
#endif
