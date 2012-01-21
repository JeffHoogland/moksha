#include "e.h"
#include "e_mod_main.h"
#include "e_mod_comp.h"
#ifdef HAVE_WAYLAND
# include "e_mod_comp_wayland.h"
#endif

#define WL_OUTPUT_FLIPPED 0x01

/* local typedefs */
typedef enum _Wayland_Shell_Surface_Type Wayland_Shell_Surface_Type;
typedef struct _Wayland_Output Wayland_Output;
typedef struct _Wayland_Input Wayland_Input;
typedef struct _Wayland_Shell Wayland_Shell;
typedef struct _Wayland_Surface Wayland_Surface;
typedef struct _Wayland_Shell_Surface Wayland_Shell_Surface;
typedef struct _Wayland_Frame_Callback Wayland_Frame_Callback;
typedef struct _Wayland_Shader Wayland_Shader;

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

struct _Wayland_Output
{
   int x, y, width, height;
   char *make, *model;
   uint32_t subpixel, flags;

   struct 
     {
        Eina_Bool needed : 1;
        Eina_Bool scheduled : 1;
     } repaint;

   struct 
     {
        uint32_t flags;
        int32_t w, h;
        uint32_t refresh;
        /* link ?? struct wl_list link */
     } mode;

   EGLSurface egl_surface;

   struct wl_object *output;
   struct wl_event_source *frame_timer;

   pixman_region32_t region, previous_damage;

   Eina_Bool (*prepare_render) (Wayland_Output *output);
   Eina_Bool (*present) (Wayland_Output *output);
   Eina_Bool (*prepare_scanout_surface) (Wayland_Output *output, Wayland_Surface *surface);
   Eina_Bool (*set_hardware_cursor) (Wayland_Output *output, Wayland_Input *input_dev);
   void (*destroy) (Wayland_Output *output);
};

struct _Wayland_Shell
{
   Eina_Bool locked : 1;
   struct wl_surface *lock_surface;
   struct wl_listener lock_listener;

   void (*lock) (Wayland_Shell *shell);
   void (*unlock) (Wayland_Shell *shell);
   void (*map) (Wayland_Shell *shell, Wayland_Surface *surface, int32_t width, int32_t height);
   void (*configure) (Wayland_Shell *shell, Wayland_Surface *surface, int32_t x, int32_t y, int32_t width, int32_t height);
   void (*destroy) (Wayland_Shell *shell);
};

struct _Wayland_Surface
{
   struct wl_surface surface;

   GLuint texture, saved_texture;

   pixman_region32_t damage, opaque;

   int32_t x, y, width, height;

   struct wl_list buffer_link;
   struct wl_buffer *buffer;
   struct wl_listener buffer_destroy_listener;
   struct wl_list link;

   uint32_t visual;
};

struct _Wayland_Shell_Surface
{
   struct wl_resource resource;
   Wayland_Surface *surface;
   struct wl_listener surface_destroy_listener;
   Wayland_Shell_Surface *parent;
   int32_t saved_x, saved_y;
   Wayland_Output *output;
   struct wl_list link;
   Wayland_Shell_Surface_Type type;
   /* TODO: Handle popup */
};

struct _Wayland_Input
{
   struct wl_input_device input_dev;
   Wayland_Surface *sprite;
   int32_t hotspot_x, hotspot_y;
};

struct _Wayland_Frame_Callback
{
   struct wl_resource resource;
   struct wl_list link;
};

struct _Wayland_Shader
{
   GLuint program;
   GLuint vertex, fragment;
   struct 
     {
        GLuint proj, tex, alpha, color;
     } uniform;
};

struct wl_shell
{
   Wayland_Shell shell;

   struct 
     {
        struct wl_client *client;
        struct wl_resource *desktop_shell;
     } child;

   Wayland_Shell_Surface *lock_surface;
   struct wl_listener lock_surface_listener;
   struct wl_list hidden_surface_list;

   Eina_Bool locked : 1;
   Eina_Bool prepare_event_sent : 1;
};

/* local function prototypes */
static Eina_Bool _e_mod_comp_wayland_backend_init(void);
static void _e_mod_comp_wayland_backend_shutdown(void);

static Eina_Bool _e_mod_comp_wayland_egl_init(void);
static void _e_mod_comp_wayland_egl_shutdown(void);

static Eina_Bool _e_mod_comp_wayland_compositor_init(void);
static void _e_mod_comp_wayland_compositor_shutdown(void);
static void _e_mod_comp_wayland_compositor_bind(struct wl_client *client, void *data __UNUSED__, uint32_t version __UNUSED__, uint32_t id);
static void _e_mod_comp_wayland_compositor_repick(void);

static Eina_Bool _e_mod_comp_wayland_output_init(void);
static void _e_mod_comp_wayland_output_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id);
static int _e_mod_comp_wayland_output_frame_handle(void *data);
static void _e_mod_comp_wayland_output_repaint(void *data);
static void _e_mod_comp_wayland_output_destroy(Wayland_Output *output);
static void _e_mod_comp_wayland_repaint_output(Wayland_Output *output);
static Eina_Bool _e_mod_comp_wayland_output_prepare_render(Wayland_Output *output);
static Eina_Bool _e_mod_comp_wayland_output_present(Wayland_Output *output);
static Eina_Bool _e_mod_comp_wayland_output_prepare_scanout_surface(Wayland_Output *output __UNUSED__, Wayland_Surface *surface __UNUSED__);
static Eina_Bool _e_mod_comp_wayland_output_set_hardware_cursor(Wayland_Output *output __UNUSED__, Wayland_Input *input __UNUSED__);

static Eina_Bool _e_mod_comp_wayland_input_init(void);
static void _e_mod_comp_wayland_input_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id);
static void _e_mod_comp_wayland_input_unbind(struct wl_resource *resource);
static void _e_mod_comp_wayland_input_attach(struct wl_client *client, struct wl_resource *resource, uint32_t timestamp, struct wl_resource *buffer_resource, int32_t x, int32_t y);
static void _e_mod_comp_wayland_input_repick(void);

static Eina_Bool _e_mod_comp_wayland_shell_init(void);
static void _e_mod_comp_wayland_shell_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id);
static void _e_mod_comp_wayland_shell_get_shell_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource);
static void _e_mod_comp_wayland_shell_lock(Wayland_Shell *base);
static void _e_mod_comp_wayland_shell_unlock(Wayland_Shell *base);
static void _e_mod_comp_wayland_shell_map(Wayland_Shell *base, Wayland_Surface *surface, int32_t width, int32_t height);
static void _e_mod_comp_wayland_shell_configure(Wayland_Shell *base, Wayland_Surface *surface, int32_t x, int32_t y, int32_t width, int32_t height);
static void _e_mod_comp_wayland_shell_destroy(Wayland_Shell *base);
static void _e_mod_comp_wayland_shell_surface_destroy_handle(struct wl_listener *listener, struct wl_resource *resource __UNUSED__, uint32_t timestamp);

static int _e_mod_comp_wayland_idle_handle(void *data __UNUSED__);
static Eina_Bool _e_mod_comp_wayland_fd_handle(void *data, Ecore_Fd_Handler *hdl __UNUSED__);

static void _e_mod_comp_wayland_surface_create(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static void _e_mod_comp_wayland_surface_destroy(struct wl_client *client __UNUSED__, struct wl_resource *resource);
static void _e_mod_comp_wayland_surface_attach(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *buffer_resource, int32_t x, int32_t y);
static void _e_mod_comp_wayland_surface_damage(struct wl_client *client __UNUSED__, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height);
static void _e_mod_comp_wayland_surface_frame(struct wl_client *client, struct wl_resource *resource, uint32_t callback);
static void _e_mod_comp_wayland_surface_configure(Wayland_Surface *surface, int32_t x, int32_t y, int32_t width, int32_t height);
static Eina_Bool _e_mod_comp_wayland_surface_move(Wayland_Surface *surface, Wayland_Input *input, uint32_t timestamp);
static Eina_Bool _e_mod_comp_wayland_surface_resize(Wayland_Surface *surface, Wayland_Input *input, uint32_t timestamp, uint32_t edges);
static Wayland_Surface *_e_mod_comp_wayland_create_surface(int32_t x, int32_t y, int32_t width, int32_t height);
static void _e_mod_comp_wayland_surface_damage_below(Wayland_Surface *surface);

static void _e_mod_comp_wayland_destroy_shell_surface(struct wl_resource *resource);
static void _e_mod_comp_wayland_destroy_surface(struct wl_resource *resource);
static void _e_mod_comp_wayland_destroy_buffer(struct wl_listener *listener, struct wl_resource *resource __UNUSED__, uint32_t timestamp __UNUSED__);
static void _e_mod_comp_wayland_destroy_frame(struct wl_resource *resource);
static void _e_mod_comp_wayland_damage_surface(Wayland_Surface *surface, int32_t x, int32_t y, int32_t width, int32_t height);

static void _e_mod_comp_wayland_shm_buffer_created(struct wl_buffer *buffer);
static void _e_mod_comp_wayland_shm_buffer_damaged(struct wl_buffer *buffer, int32_t x __UNUSED__, int32_t y __UNUSED__, int32_t width __UNUSED__, int32_t height __UNUSED__);
static void _e_mod_comp_wayland_shm_buffer_destroyed(struct wl_buffer *buffer);

static void _e_mod_comp_wayland_buffer_attach(struct wl_buffer *buffer, struct wl_surface *surface);

static void _e_mod_comp_wayland_shell_surface_move(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_resource, uint32_t timestamp);
static void _e_mod_comp_wayland_shell_surface_resize(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_resource, uint32_t timestamp, uint32_t edges);
static void _e_mod_comp_wayland_shell_surface_set_toplevel(struct wl_client *client __UNUSED__, struct wl_resource *resource);
static void _e_mod_comp_wayland_shell_surface_set_transient(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *parent_resource, int x, int y, uint32_t flags __UNUSED__);
static void _e_mod_comp_wayland_shell_surface_set_fullscreen(struct wl_client *client __UNUSED__, struct wl_resource *resource);
static void _e_mod_comp_wayland_shell_surface_set_popup(struct wl_client *client, struct wl_resource *resource, struct wl_resource *input_resource, uint32_t timestamp, struct wl_resource *parent_resource, int32_t x, int32_t y, uint32_t flags);
static Eina_Bool _e_mod_comp_wayland_shell_surface_type_reset(Wayland_Shell_Surface *shell_surface);

static Wayland_Shell_Surface *_e_mod_comp_wayland_get_shell_surface(Wayland_Surface *surface);

/* wayland interfaces */
static const struct wl_compositor_interface _wl_comp_interface = 
{
   _e_mod_comp_wayland_surface_create,
};

static const struct wl_shm_callbacks _wl_shm_callbacks = 
{
   _e_mod_comp_wayland_shm_buffer_created, 
   _e_mod_comp_wayland_shm_buffer_damaged, 
   _e_mod_comp_wayland_shm_buffer_destroyed
};

static const struct wl_input_device_interface _wl_input_interface = 
{
   _e_mod_comp_wayland_input_attach,
};
static const struct wl_shell_interface _wl_shell_interface = 
{
   _e_mod_comp_wayland_shell_get_shell_surface
};
static const struct wl_surface_interface _wl_surface_interface = 
{
   _e_mod_comp_wayland_surface_destroy, 
   _e_mod_comp_wayland_surface_attach, 
   _e_mod_comp_wayland_surface_damage,
   _e_mod_comp_wayland_surface_frame
};
static const struct wl_shell_surface_interface _wl_shell_surface_interface = 
{
   _e_mod_comp_wayland_shell_surface_move, 
   _e_mod_comp_wayland_shell_surface_resize, 
   _e_mod_comp_wayland_shell_surface_set_toplevel, 
   _e_mod_comp_wayland_shell_surface_set_transient, 
   _e_mod_comp_wayland_shell_surface_set_fullscreen, 
   _e_mod_comp_wayland_shell_surface_set_popup
};

/* private variables */
static struct wl_display *_wl_disp;
static struct wl_compositor *_wl_comp;
static struct wl_shm *_wl_shm;
static struct wl_event_source *_wl_idle_timer;
static struct wl_event_source *_wl_event_source;
static struct wl_list _wl_comp_surface_list;

static Ecore_Fd_Handler *_wl_fd_handler = NULL;
static EGLDisplay _wl_egl_disp;
static EGLContext _wl_egl_context;
static EGLConfig _wl_egl_config;
static Wayland_Output *_wl_output;
static Wayland_Input *_wl_input;
static Wayland_Shell *_wl_shell;

Eina_Bool 
e_mod_comp_wayland_init(void)
{
   /* init wayland display */
   if (!(_wl_disp = wl_display_create()))
     {
        EINA_LOG_ERR("Failed to create wayland display\n");
        return EINA_FALSE;
     }

   /* TODO: Add signal handlers for SIGTERM, etc */

   /* INIT BACKEND */
   if (!_e_mod_comp_wayland_backend_init())
     {
        EINA_LOG_ERR("Failed to init wayland backend\n");
        if (_wl_disp) wl_display_terminate(_wl_disp);
        _wl_disp = NULL;
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

void 
e_mod_comp_wayland_shutdown(void)
{
   if (!_wl_disp) return;
   _e_mod_comp_wayland_backend_shutdown();
}

void 
e_mod_comp_wayland_repaint(void)
{
   struct wl_event_loop *loop;

   loop = wl_display_get_event_loop(_wl_disp);

   if (_wl_output)
     {
        _wl_output->repaint.needed = EINA_TRUE;
        if (_wl_output->repaint.scheduled) return;
        wl_event_loop_add_idle(loop, 
                               _e_mod_comp_wayland_output_repaint, _wl_output);
        _wl_output->repaint.scheduled = EINA_TRUE;
     }
}

/* local functions */
static Eina_Bool 
_e_mod_comp_wayland_backend_init(void)
{
   struct wl_event_loop *loop;
   int fd = 0;

   /* init egl */
   if (!_e_mod_comp_wayland_egl_init()) return EINA_FALSE;

   /* init compositor */
   if (!_e_mod_comp_wayland_compositor_init()) return EINA_FALSE;

   /* init output */
   if (!_e_mod_comp_wayland_output_init()) return EINA_FALSE;

   /* init input */
   if (!_e_mod_comp_wayland_input_init()) return EINA_FALSE;

   loop = wl_display_get_event_loop(_wl_disp);
   fd = wl_event_loop_get_fd(loop);

   _wl_fd_handler = 
     ecore_main_fd_handler_add(fd, ECORE_FD_READ, 
                               _e_mod_comp_wayland_fd_handle, 
                               _wl_disp, NULL, NULL);

   /* init shell */
   if (!_e_mod_comp_wayland_shell_init()) return EINA_FALSE;

   if (wl_display_add_socket(_wl_disp, NULL))
     {
        EINA_LOG_ERR("Failed to add display socket\n");
        /* _e_mod_comp_wayland_input_shutdown(); */
        /* _e_mod_comp_wayland_output_shutdown(); */
        _e_mod_comp_wayland_compositor_shutdown();
        return EINA_FALSE;
     }

   /* wake the compositor */
   wl_event_source_timer_update(_wl_idle_timer, 300 * 1000);

   /* run the display */
   wl_event_loop_dispatch(loop, 0);

   /* wl_display_run(_wl_disp); */

   return EINA_TRUE;
}

static void 
_e_mod_comp_wayland_backend_shutdown(void)
{
   /* remove main event source */
   if (_wl_event_source)
     wl_event_source_remove(_wl_event_source);

   /* input destroy */
   if (_wl_input)
     {
        wl_input_device_release(&_wl_input->input_dev);
        free(_wl_input);
     }

   /* compositor destroy */
   _e_mod_comp_wayland_compositor_shutdown();

   /* shutdown egl */
   _e_mod_comp_wayland_egl_shutdown();
}

static Eina_Bool 
_e_mod_comp_wayland_egl_init(void)
{
   EGLint major, minor, n;
   const char *extensions;
   EGLint cfg_attribs[] = 
     {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT, 
        EGL_RED_SIZE, 1, 
        EGL_GREEN_SIZE, 1, 
        EGL_BLUE_SIZE, 1, 
        EGL_DEPTH_SIZE, 1, 
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE
     };
   static const EGLint ctxt_attribs[] = 
     {
        EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
     };

   /* NB: Due to the way mesa init's egl, we need to pass a 'NULL' display here 
    * in order to avoid a segfault due to xlib vs xcb of ecore_x */
   if (!(_wl_egl_disp = eglGetDisplay(NULL)))
     {
        EINA_LOG_ERR("Failed to create Egl Display\n");
        return EINA_FALSE;
     }
   if (!eglInitialize(_wl_egl_disp, &major, &minor))
     {
        EINA_LOG_ERR("Failed to init Egl\n");
        return EINA_FALSE;
     }

   /* init extensions */
   extensions = eglQueryString(_wl_egl_disp, EGL_EXTENSIONS);
   if (!strstr(extensions, "EGL_KHR_surfaceless_gles2"))
     {
        EINA_LOG_ERR("EGL_KHR_surfaceless_gles2 extension not available\n");
        return EINA_FALSE;
     }

   if (!eglBindAPI(EGL_OPENGL_ES_API))
     {
        EINA_LOG_ERR("Failed to bind EGL_OPENGL_ES_API\n");
        return EINA_FALSE;
     }

   if ((!eglChooseConfig(_wl_egl_disp, cfg_attribs, &_wl_egl_config, 1, &n) || 
        (n == 0)))
     {
        EINA_LOG_ERR("Failed to choose egl config\n");
        return EINA_FALSE;
     }

   _wl_egl_context = 
     eglCreateContext(_wl_egl_disp, _wl_egl_config, EGL_NO_CONTEXT, ctxt_attribs);
   if (!_wl_egl_context)
     {
        EINA_LOG_ERR("Failed to create egl context\n");
        return EINA_FALSE;
     }

   if (!eglMakeCurrent(_wl_egl_disp, EGL_NO_SURFACE, EGL_NO_SURFACE, 
                       _wl_egl_context))
     {
        EINA_LOG_ERR("Failed to make context current\n");
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static void 
_e_mod_comp_wayland_egl_shutdown(void)
{
   if (!_wl_egl_disp) return;
   eglMakeCurrent(_wl_egl_disp, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
   eglTerminate(_wl_egl_disp);
   eglReleaseThread();
}

static Eina_Bool 
_e_mod_comp_wayland_compositor_init(void)
{
   struct wl_event_loop *loop;

   memset(&_wl_comp, 0, sizeof(_wl_comp));
   if (!wl_display_add_global(_wl_disp, &wl_compositor_interface, _wl_comp, 
                              _e_mod_comp_wayland_compositor_bind))
     {
        EINA_LOG_ERR("Failed to init wayland compositor\n");
        return EINA_FALSE;
     }

   _wl_shm = wl_shm_init(_wl_disp, &_wl_shm_callbacks);

   wl_list_init(&_wl_comp_surface_list);

   /* TODO: Maybe need egl wl bind display ?? */

   wl_data_device_manager_init(_wl_disp);

   glActiveTexture(GL_TEXTURE0);

   /* TODO: Shader init */

   loop = wl_display_get_event_loop(_wl_disp);

   _wl_idle_timer = 
     wl_event_loop_add_timer(loop, _e_mod_comp_wayland_idle_handle, NULL);
   wl_event_source_timer_update(_wl_idle_timer, 300 * 1000);

   e_mod_comp_wayland_repaint();

   return EINA_TRUE;
}

static void 
_e_mod_comp_wayland_compositor_shutdown(void)
{
   if (_wl_idle_timer)
     wl_event_source_remove(_wl_idle_timer);

   if (_wl_output) _wl_output->destroy(_wl_output);
   if (_wl_shm) wl_shm_finish(_wl_shm);
}

static void 
_e_mod_comp_wayland_compositor_bind(struct wl_client *client, void *data __UNUSED__, uint32_t version __UNUSED__, uint32_t id)
{
   wl_client_add_object(client, &wl_compositor_interface, 
                        &_wl_comp_interface, id, _wl_comp);
}

static void 
_e_mod_comp_wayland_compositor_repick(void)
{
   _e_mod_comp_wayland_input_repick();
}

static Eina_Bool 
_e_mod_comp_wayland_output_init(void)
{
   struct wl_event_loop *loop;
   Ecore_X_Window *roots;
   int rnum = 0, rx = 0, ry = 0, rw, rh;

   roots = ecore_x_window_root_list(&rnum);
   if ((!roots) || (rnum <= 0))
     {
        EINA_LOG_ERR("Failed to get root window list\n");
        return EINA_FALSE;
     }

   /* get root window properties */
   ecore_x_window_size_get(roots[0], &rw, &rh);
   free(roots);

   /* init output */
   /* FIXME: Handle more than one root window ?? */
   if (!(_wl_output = malloc(sizeof(Wayland_Output))))
     {
        EINA_LOG_ERR("Failed to malloc wayland output\n");
        return EINA_FALSE;
     }

   _wl_output->mode.flags = (WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED);
   _wl_output->mode.w = rw;
   _wl_output->mode.h = rh;
   _wl_output->mode.refresh = 60;

   /* set variables */
   _wl_output->x = rx;
   _wl_output->y = ry;
   _wl_output->width = rw;
   _wl_output->height = rh;
   _wl_output->flags = WL_OUTPUT_FLIPPED;

   /* FIXME: Handle weston_output_move ?? */

   if (!wl_display_add_global(_wl_disp, &wl_output_interface, _wl_output, 
                              _e_mod_comp_wayland_output_bind))
     {
        EINA_LOG_ERR("Failed to add wayland output\n");
        return EINA_FALSE;
     }

   /* TODO: Fix me */
   /* _wl_output->egl_surface =  */
   /*   eglCreateWindowSurface(_wl_egl_disp, _wl_egl_config, _wl_output->window, NULL); */
   /* eglMakeCurrent(_wl_egl_disp, _wl_output->egl_surface, _wl_output->egl_surface,  */
   /*                _wl_egl_context); */

   loop = wl_display_get_event_loop(_wl_disp);
   _wl_output->frame_timer = 
     wl_event_loop_add_timer(loop, _e_mod_comp_wayland_output_frame_handle, 
                             _wl_output);

   _wl_output->prepare_render = _e_mod_comp_wayland_output_prepare_render;
   _wl_output->present = _e_mod_comp_wayland_output_present;
   _wl_output->prepare_scanout_surface = 
     _e_mod_comp_wayland_output_prepare_scanout_surface;
   _wl_output->set_hardware_cursor = 
     _e_mod_comp_wayland_output_set_hardware_cursor;
   _wl_output->destroy = _e_mod_comp_wayland_output_destroy;

   return EINA_TRUE;
}

static void 
_e_mod_comp_wayland_output_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id)
{
   Wayland_Output *output;
   struct wl_resource *resource;

   if (!(output = data)) return;

   resource = 
     wl_client_add_object(client, &wl_output_interface, NULL, id, data);

   wl_resource_post_event(resource, WL_OUTPUT_GEOMETRY, 
                          output->x, output->y, output->width, output->height, 
                          output->subpixel, output->make, output->model);

   /* TODO: iterate mode list */

   wl_resource_post_event(resource, WL_OUTPUT_MODE, output->mode.flags, 
                          output->mode.w, output->mode.h, output->mode.refresh);
}

static int 
_e_mod_comp_wayland_output_frame_handle(void *data)
{
   Wayland_Output *output;
   uint32_t sec;
   struct timeval val;

   if (!(output = data)) return 0;

   gettimeofday(&val, NULL);
   sec = val.tv_sec * 1000 + val.tv_usec / 1000;

   /* TODO: weston output finish frame */

   return 1;
}

static void 
_e_mod_comp_wayland_output_repaint(void *data)
{
   Wayland_Output *output;

   if (!(output = data)) return;

   _e_mod_comp_wayland_repaint_output(output);

   output->repaint.needed = EINA_FALSE;
   output->repaint.scheduled = EINA_TRUE;
   if (output->present) output->present(output);

   /* TODO: Loop output frame callback list */
   /* call resource_post_event */
   /* call resource destroy */

   /* TODO: Handle animation frame */
}

static void 
_e_mod_comp_wayland_output_destroy(Wayland_Output *output)
{
   if (!output) return;

   if (output->frame_timer) wl_event_source_remove(output->frame_timer);

   if (output->egl_surface)
     eglDestroySurface(_wl_egl_disp, output->egl_surface);

   free(output);
}

static void 
_e_mod_comp_wayland_repaint_output(Wayland_Output *output)
{
   Wayland_Surface *ws;
   pixman_region32_t opaque, new_damage, total_damage, repaint;

   printf("Comp Wayland Output Repaint\n");
   if (!output) return;

   output->prepare_render(output);

   glViewport(0, 0, output->width, output->height);

   /* gluseprogram, gluniformmatrix, etc */

   /* TODO: Set cursor */

   pixman_region32_init(&new_damage);
   pixman_region32_init(&opaque);

   wl_list_for_each(ws, &_wl_comp_surface_list, link)
     {
        pixman_region32_subtract(&ws->damage, &ws->damage, &opaque);
        pixman_region32_union(&new_damage, &new_damage, &ws->damage);
        pixman_region32_union(&opaque, &opaque, &ws->opaque);
     }

   pixman_region32_init(&total_damage);
   pixman_region32_union(&total_damage, &new_damage, &output->previous_damage);
   pixman_region32_intersect(&output->previous_damage, &new_damage, 
                             &output->region);

   pixman_region32_fini(&opaque);
   pixman_region32_fini(&new_damage);

   ws = container_of(_wl_comp_surface_list.next, Wayland_Surface, link);
   /* TODO: Scanout_surface */

   /* TODO: Handle fullscreen */
   /* if (ws->type == SHELL_SURFACE_FULLSCREEN) */
   /*   { */
   /*      if ((ws->width < output->width) ||  */
   /*          (ws->height < output->height)) */
   /*        glClear(GL_COLOR_BUFFER_BIT); */
   /*   } */
   /* else */
     {
        wl_list_for_each(ws, &_wl_comp_surface_list, link)
          {
             pixman_region32_copy(&ws->damage, &total_damage);
             pixman_region32_subtract(&total_damage, &total_damage, &ws->opaque);
          }
        wl_list_for_each_reverse(ws, &_wl_comp_surface_list, link) 
          {
             pixman_region32_init(&repaint);
             pixman_region32_intersect(&repaint, &output->region, &ws->damage);
             /* TODO: surface draw */
             pixman_region32_subtract(&ws->damage, &ws->damage, &output->region);
             pixman_region32_fini(&repaint);
          }
     }

   pixman_region32_fini(&total_damage);
}

static Eina_Bool 
_e_mod_comp_wayland_output_prepare_render(Wayland_Output *output)
{
   printf("Comp Wayland Output Prepare Render\n");
   if (!output->egl_surface) return EINA_FALSE;

   if (!eglMakeCurrent(_wl_egl_disp, output->egl_surface, output->egl_surface, 
                       _wl_egl_context))
     return EINA_FALSE;

   return EINA_TRUE;
}

static Eina_Bool 
_e_mod_comp_wayland_output_present(Wayland_Output *output)
{
   printf("Comp Wayland Output Present\n");

   if (_e_mod_comp_wayland_output_prepare_render(output)) 
     return EINA_FALSE;

   if (!output->egl_surface) return EINA_FALSE;
   eglSwapBuffers(_wl_egl_disp, output->egl_surface);
   wl_event_source_timer_update(output->frame_timer, 10);

   return EINA_TRUE;
}

static Eina_Bool 
_e_mod_comp_wayland_output_prepare_scanout_surface(Wayland_Output *output __UNUSED__, Wayland_Surface *surface __UNUSED__)
{
   return EINA_FALSE;
}

static Eina_Bool 
_e_mod_comp_wayland_output_set_hardware_cursor(Wayland_Output *output __UNUSED__, Wayland_Input *input __UNUSED__)
{
   return EINA_FALSE;
}

static Eina_Bool 
_e_mod_comp_wayland_input_init(void)
{
   if (!(_wl_input = malloc(sizeof(Wayland_Input))))
     {
        EINA_LOG_ERR("Failed to malloc wayland input\n");
        return EINA_FALSE;
     }

   wl_input_device_init(&_wl_input->input_dev);

   wl_display_add_global(_wl_disp, &wl_input_device_interface, 
                         _wl_input, _e_mod_comp_wayland_input_bind);

   return EINA_TRUE;
}

static void 
_e_mod_comp_wayland_input_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id)
{
   struct wl_input_device *dev;
   struct wl_resource *resource;

   if (!(dev = data)) return;

   resource = 
     wl_client_add_object(client, &wl_input_device_interface, 
                          &_wl_input_interface, id, data);

   wl_list_insert(&dev->resource_list, &resource->link);
   resource->destroy = _e_mod_comp_wayland_input_unbind;
}

static void 
_e_mod_comp_wayland_input_unbind(struct wl_resource *resource)
{
   printf("Comp Module Wayland Input Unbind\n");
   wl_list_remove(&resource->link);
   free(resource);
}

static void 
_e_mod_comp_wayland_input_attach(struct wl_client *client, struct wl_resource *resource, uint32_t timestamp, struct wl_resource *buffer_resource, int32_t x, int32_t y)
{
   Wayland_Input *wi;
   struct wl_buffer *buffer;

   printf("Comp Module Wayland Input Attach\n");

   wi = resource->data;
   if (timestamp < wi->input_dev.pointer_focus_time) return;
   if (!wi->input_dev.pointer_focus) return;
   if (wi->input_dev.pointer_focus->resource.client != client) return;

   if (wi->sprite)
     _e_mod_comp_wayland_surface_damage_below(wi->sprite);

   if (!buffer_resource)
     {
        if (wi->sprite) 
          {
             _e_mod_comp_wayland_destroy_surface(&wi->sprite->surface.resource);
             wi->sprite = NULL;
          }
        return;
     }

   if (!wi->sprite)
     {
        wi->sprite = 
          _e_mod_comp_wayland_create_surface(wi->input_dev.x, 
                                             wi->input_dev.y, 32, 32);
        wl_list_init(&wi->sprite->link);
     }

   buffer = buffer_resource->data;
   _e_mod_comp_wayland_buffer_attach(buffer, &wi->sprite->surface);

   wi->hotspot_x = x;
   wi->hotspot_y = y;
   wi->sprite->width = buffer->width;
   wi->sprite->height = buffer->height;
   wi->sprite->x = wi->input_dev.x - wi->hotspot_x;
   wi->sprite->y = wi->input_dev.y - wi->hotspot_y;

   _e_mod_comp_wayland_damage_surface(wi->sprite, 0, 0, 32, 32);
}

static void 
_e_mod_comp_wayland_input_repick(void)
{
   /* TODO: Code me (compositor.c: 347) */
}

static Eina_Bool 
_e_mod_comp_wayland_shell_init(void)
{
   /* init shell */
   if (!(_wl_shell = malloc(sizeof(Wayland_Shell))))
     {
        EINA_LOG_ERR("Failed to malloc wayland shell\n");
        return EINA_FALSE;
     }

//   wl_list_init(&);

   _wl_shell->lock = _e_mod_comp_wayland_shell_lock;
   _wl_shell->unlock = _e_mod_comp_wayland_shell_unlock;
   _wl_shell->map = _e_mod_comp_wayland_shell_map;
   _wl_shell->configure = _e_mod_comp_wayland_shell_configure;
   _wl_shell->destroy = _e_mod_comp_wayland_shell_destroy;

   /* FIXME: Maybe handle shell_configure to setup screensaver ? */

   if (!wl_display_add_global(_wl_disp, &wl_shell_interface, _wl_shell, 
                              _e_mod_comp_wayland_shell_bind))
     {
        EINA_LOG_ERR("Failed to add wayland shell\n");
        return EINA_FALSE;
     }

   /* TODO: Maybe add desktop_shell interface ?? */
   /* if (!wl_display_add_global(_wl_disp, &wl_shell_interface, _wl_shell,  */
   /*                            _e_mod_comp_wayland_shell_bind)) */
   /*   { */
   /*      EINA_LOG_ERR("Failed to add wayland shell\n"); */
   /*      return EINA_FALSE; */
   /*   } */

   /* TODO: Maybe add screensaver interface ?? */

   return EINA_TRUE;
}

static void 
_e_mod_comp_wayland_shell_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id)
{
   Wayland_Shell *shell;

   if (!(shell = data)) return;
   wl_client_add_object(client, &wl_shell_interface, 
                        &_wl_shell_interface, id, shell);
}

static void 
_e_mod_comp_wayland_shell_get_shell_surface(struct wl_client *client, struct wl_resource *resource, uint32_t id, struct wl_resource *surface_resource)
{
   Wayland_Surface *ws;
   Wayland_Shell_Surface *wss;

   printf("Comp Module Wayland Shell Get Shell Surface\n");

   ws = surface_resource->data;

   if (_e_mod_comp_wayland_get_shell_surface(ws))
     {
        wl_resource_post_error(surface_resource, 
                               WL_DISPLAY_ERROR_INVALID_OBJECT, 
                               "get_shell_surface already requested");
        return;
     }

   if (!(wss = calloc(1, sizeof(Wayland_Shell_Surface))))
     {
        wl_resource_post_no_memory(resource);
        return;
     }

   wss->resource.destroy = _e_mod_comp_wayland_destroy_shell_surface;
   wss->resource.object.id = id;
   wss->resource.object.interface = &wl_shell_surface_interface;
   wss->resource.object.implementation = 
     (void (**)(void)) &_wl_shell_surface_interface;
   wss->resource.data = wss;

   wss->surface = ws;
   wss->surface_destroy_listener.func = 
     _e_mod_comp_wayland_shell_surface_destroy_handle;

   wl_list_insert(ws->surface.resource.destroy_listener_list.prev, 
                  &wss->surface_destroy_listener.link);
   wl_list_init(&wss->link);

   wss->type = 0;
   wl_client_add_resource(client, &wss->resource);
}

static void 
_e_mod_comp_wayland_shell_lock(Wayland_Shell *base)
{
   Wayland_Surface *cur, *tmp;
   struct wl_shell *shell;
   struct wl_list *surface_list;

   printf("Comp Module Wayland Shell Lock\n");
   shell = container_of(base, struct wl_shell, shell);
   surface_list = &_wl_comp_surface_list;

   if (shell->locked) return;
   shell->locked = EINA_TRUE;

   /* TODO: Handle hidden surfaces */
   if (!wl_list_empty(&shell->hidden_surface_list))
     EINA_LOG_ERR("Hidden surface list not empty\n");

   /* TODO: loop surface list */
   wl_list_for_each_safe(cur, tmp, surface_list, link)
     {
        if (!cur->surface.resource.client) continue;
        /* TODO: Get shell surface type */
        /* cur->output = NULL; */
        wl_list_remove(&cur->link);
        wl_list_insert(shell->hidden_surface_list.prev, &cur->link);
     }

   /* TODO: Handle screensaver surfaces ? */

   /* skip input device sprites */

   /* compositor repick for pointer focus */

   /* reset keyboard focus */
}

static void 
_e_mod_comp_wayland_shell_unlock(Wayland_Shell *base)
{
   struct wl_shell *shell;

   printf("Comp Module Wayland Shell Unlock\n");

   shell = container_of(base, struct wl_shell, shell);
   if ((!shell->locked) || (shell->lock_surface))
     {
        /* TODO: Wake compositor */
        return;
     }

   /* TODO: Desktop shell ?? */

   if (shell->prepare_event_sent) return;
   wl_resource_post_event(shell->child.desktop_shell, 1);
   shell->prepare_event_sent = EINA_TRUE;
}

static void 
_e_mod_comp_wayland_shell_map(Wayland_Shell *base, Wayland_Surface *surface, int32_t width, int32_t height)
{
   struct wl_shell *shell;
   struct wl_list *list;
   Wayland_Shell_Surface *wss;
   Eina_Bool do_configure = EINA_FALSE;

   printf("Comp Module Wayland Shell Map\n");

   shell = container_of(base, struct wl_shell, shell);
   wss = _e_mod_comp_wayland_get_shell_surface(surface);
   if (shell->locked) 
     {
        list = &shell->hidden_surface_list;
        do_configure = EINA_FALSE;
     }
   else 
     {
        list = &_wl_comp_surface_list;
        do_configure = EINA_TRUE;
     }
   surface->width = width;
   surface->height = height;

   /* initial position */
   switch (wss->type)
     {
      case SHELL_SURFACE_TOPLEVEL:
        surface->x = 10 + random() % 400;
        surface->y = 10 + random() % 400;
        break;
      case SHELL_SURFACE_SCREENSAVER:
      case SHELL_SURFACE_FULLSCREEN:
        /* TODO: Center on output */
        break;
      case SHELL_SURFACE_LOCK:
        /* TODO: Center on output */
        break;
      default:
        break;
     }

   /* stacking order */
   switch (wss->type)
     {
      case SHELL_SURFACE_BACKGROUND:
        wl_list_insert(_wl_comp_surface_list.prev, &surface->link);
        do_configure = EINA_TRUE;
        break;
      case SHELL_SURFACE_PANEL:
        wl_list_insert(list, &surface->link);
        break;
      case SHELL_SURFACE_LOCK:
        wl_list_insert(&_wl_comp_surface_list, &surface->link);
        _e_mod_comp_wayland_compositor_repick();
        /* TODO: wake */
        do_configure = EINA_TRUE;
        break;
      case SHELL_SURFACE_SCREENSAVER:
        do_configure = EINA_FALSE;
        break;
      default:
        /* NB: We skip 'panel' here */
        wl_list_insert(list, &surface->link);
        break;
     }

   /* TODO: Handle popups */

   surface->width = width;
   surface->height = height;
   if (do_configure)
     {
        _e_mod_comp_wayland_surface_configure(surface, surface->x, surface->y, 
                                              width, height);
        _e_mod_comp_wayland_compositor_repick();
     }

   switch (wss->type)
     {
      case SHELL_SURFACE_TOPLEVEL:
      case SHELL_SURFACE_TRANSIENT:
      case SHELL_SURFACE_FULLSCREEN:
        if (!shell->locked)
          {
             /* TODO: activate */
          }
        break;
      default:
        break;
     }

   if (wss->type == SHELL_SURFACE_TOPLEVEL)
     {
        /* TODO: Use zoom effect ?? */
     }
}

static void 
_e_mod_comp_wayland_shell_configure(Wayland_Shell *base, Wayland_Surface *surface, int32_t x, int32_t y, int32_t width, int32_t height)
{
   Wayland_Shell_Surface *wss;
   struct wl_shell *shell;
   Eina_Bool do_configure = EINA_FALSE;

   printf("Comp Module Wayland Shell Configure\n");
   shell = container_of(base, struct wl_shell, shell);
   do_configure = !shell->locked;
   wss = _e_mod_comp_wayland_get_shell_surface(surface);

   surface->width = width;
   surface->height = height;

   switch (wss->type)
     {
      case SHELL_SURFACE_SCREENSAVER:
        do_configure = !do_configure;
        break;
      case SHELL_SURFACE_FULLSCREEN:
        /* TODO: center on output */
        break;
      default:
        break;
     }

   if (do_configure)
     _e_mod_comp_wayland_surface_configure(surface, x, y, width, height);
}

static void 
_e_mod_comp_wayland_shell_destroy(Wayland_Shell *base)
{
   struct wl_shell *shell;

   printf("Comp Module Wayland Shell Destroy\n");
   shell = container_of(base, struct wl_shell, shell);
   if (shell->child.client)
     wl_client_destroy(shell->child.client);
   free(shell);
}

static void 
_e_mod_comp_wayland_shell_surface_destroy_handle(struct wl_listener *listener, struct wl_resource *resource __UNUSED__, uint32_t timestamp)
{
   Wayland_Shell_Surface *wss;

   wss = container_of(listener, Wayland_Shell_Surface, surface_destroy_listener);
   wss->surface = NULL;
   wl_resource_destroy(&wss->resource, timestamp);
}

static int 
_e_mod_comp_wayland_idle_handle(void *data __UNUSED__)
{
   printf("Comp Module Wayland Idle Handle\n");
   return EINA_TRUE;
}

static Eina_Bool 
_e_mod_comp_wayland_fd_handle(void *data, Ecore_Fd_Handler *hdl __UNUSED__)
{
   struct wl_display *disp;
   struct wl_event_loop *loop;

   if (!(disp = data)) return ECORE_CALLBACK_RENEW;
   if (disp != _wl_disp) return ECORE_CALLBACK_RENEW;

   printf("Wayland FD Updated. Process Events\n");

   loop = wl_display_get_event_loop(disp);
   wl_event_loop_dispatch(loop, -1);

   return ECORE_CALLBACK_RENEW;
}

static void 
_e_mod_comp_wayland_surface_create(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
   Wayland_Surface *ws;

   printf("Comp Create Surface\n");

   if (!(ws = _e_mod_comp_wayland_create_surface(0, 0, 0, 0)))
     {
        wl_resource_post_no_memory(resource);
        return;
     }

   ws->surface.resource.client = NULL;
   ws->surface.resource.destroy = _e_mod_comp_wayland_destroy_surface;
   ws->surface.resource.object.id = id;
   ws->surface.resource.object.interface = &wl_surface_interface;
   ws->surface.resource.object.implementation = 
     (void (**)(void))&_wl_surface_interface;
   ws->surface.resource.data = ws;

   wl_client_add_resource(client, &ws->surface.resource);
}

static void 
_e_mod_comp_wayland_surface_destroy(struct wl_client *client __UNUSED__, struct wl_resource *resource)
{
   printf("Comp Surface Destroy\n");
   wl_resource_destroy(resource, ecore_x_current_time_get());
}

static void 
_e_mod_comp_wayland_surface_attach(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *buffer_resource, int32_t x, int32_t y)
{
   Wayland_Surface *ws;
   struct wl_buffer *buffer;

   printf("Comp Surface Attach\n");
   if (!(ws = resource->data)) return;

   buffer = buffer_resource->data;

   _e_mod_comp_wayland_surface_damage_below(ws);

   if (ws->buffer)
     {
        wl_resource_queue_event(&ws->buffer->resource, WL_BUFFER_RELEASE);
        wl_list_remove(&ws->buffer_destroy_listener.link);
     }

   ws->buffer = buffer;
   wl_list_insert(ws->buffer->resource.destroy_listener_list.prev, 
                  &ws->buffer_destroy_listener.link);

   if (!ws->visual)
     _wl_shell->map(_wl_shell, ws, buffer->width, buffer->height);
   else if ((x != 0) || (y != 0) || (ws->width != buffer->width) || 
            (ws->height != buffer->height))
     {
        _wl_shell->configure(_wl_shell, ws, ws->x + x, ws->y + y, 
                             buffer->width, buffer->height);
     }

   _e_mod_comp_wayland_buffer_attach(buffer, &ws->surface);
}

static void 
_e_mod_comp_wayland_surface_damage(struct wl_client *client __UNUSED__, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
   Wayland_Surface *ws;

   printf("Comp Surface Damage\n");

   ws = resource->data;
   _e_mod_comp_wayland_damage_surface(ws, x, y, width, height);
}

static void 
_e_mod_comp_wayland_surface_frame(struct wl_client *client, struct wl_resource *resource, uint32_t callback)
{
   Wayland_Surface *ws;
   Wayland_Frame_Callback *cb;

   printf("Comp Surface Frame\n");

   ws = resource->data;
   if (!(cb = malloc(sizeof(Wayland_Frame_Callback))))
     {
        wl_resource_post_no_memory(resource);
        return;
     }

   cb->resource.object.interface = &wl_callback_interface;
   cb->resource.object.id = callback;
   cb->resource.destroy = _e_mod_comp_wayland_destroy_frame;
   cb->resource.client = client;
   cb->resource.data = cb;

   wl_client_add_resource(client, &cb->resource);

   /* TODO: Add to frame callback list */
}

static void 
_e_mod_comp_wayland_surface_configure(Wayland_Surface *surface, int32_t x, int32_t y, int32_t width, int32_t height)
{
   printf("Comp Surface Configure\n");

   _e_mod_comp_wayland_surface_damage_below(surface);

   surface->x = x;
   surface->y = y;
   surface->width = width;
   surface->height = height;

   /* TODO: Assign output */

   _e_mod_comp_wayland_damage_surface(surface, 0, 0, surface->width, 
                                      surface->height);

   /* TODO: Pixman opaque stuff (compositor.c: 319) */
}

static Eina_Bool 
_e_mod_comp_wayland_surface_move(Wayland_Surface *surface, Wayland_Input *input, uint32_t timestamp)
{
   /* TODO: Handle mouse grab */

   wl_input_device_set_pointer_focus(&input->input_dev, NULL, timestamp, 
                                     0, 0, 0, 0);

   return EINA_TRUE;
}

static Eina_Bool 
_e_mod_comp_wayland_surface_resize(Wayland_Surface *surface, Wayland_Input *input, uint32_t timestamp, uint32_t edges)
{
   /* TODO: Handle resize grab */

   wl_input_device_set_pointer_focus(&input->input_dev, NULL, timestamp, 
                                     0, 0, 0, 0);
   return EINA_TRUE;
}

static Wayland_Surface *
_e_mod_comp_wayland_create_surface(int32_t x, int32_t y, int32_t width, int32_t height)
{
   Wayland_Surface *ws;

   if (!(ws = calloc(1, sizeof(Wayland_Surface))))
     return NULL;

   wl_list_init(&ws->link);
   wl_list_init(&ws->buffer_link);

   /* Setup gl texture for surface */
   glGenTextures(1, &ws->texture);
   glBindTexture(GL_TEXTURE_2D, ws->texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   ws->x = x;
   ws->y = y;
   ws->width = width;
   ws->height = height;
   ws->saved_texture = 0;
   ws->buffer = NULL;
   ws->buffer_destroy_listener.func = _e_mod_comp_wayland_destroy_buffer;
   ws->visual = 0;

   pixman_region32_init(&ws->damage);

   /* ws->surface.resource.client = NULL; */
   /* ws->surface.resource.destroy = _e_mod_comp_wayland_destroy_surface; */
   /* ws->surface.resource.object.id = id; */
   /* ws->surface.resource.object.interface = &wl_surface_interface; */
   /* ws->surface.resource.object.implementation =  */
   /*   (void (**)(void))&_wl_surface_interface; */
   /* ws->surface.resource.data = ws; */

   /* wl_client_add_resource(client, &ws->surface.resource); */

   return ws;
}

static void 
_e_mod_comp_wayland_surface_damage_below(Wayland_Surface *surface)
{
   Wayland_Surface *below;

   if (surface->link.next == &_wl_comp_surface_list) return;
   below = container_of(surface->link.next, Wayland_Surface, link);
   pixman_region32_union_rect(&below->damage, &below->damage, 
                              surface->x, surface->y, surface->width, 
                              surface->height);
   e_mod_comp_wayland_repaint();
}

static void 
_e_mod_comp_wayland_destroy_shell_surface(struct wl_resource *resource)
{
   Wayland_Shell_Surface *wss;

   wss = resource->data;
   /* TODO: Handle popup grab */
   if (wss->surface)
     wl_list_remove(&wss->surface_destroy_listener.link);
   wl_list_remove(&wss->link);
   free(wss);
}

static void 
_e_mod_comp_wayland_destroy_surface(struct wl_resource *resource)
{
   Wayland_Surface *ws;

   printf("Comp Destroy Surface\n");

   ws = container_of(resource, Wayland_Surface, surface.resource);

   _e_mod_comp_wayland_surface_damage_below(ws);

   /* TODO: flush damage */

   wl_list_remove(&ws->link);

   _e_mod_comp_wayland_compositor_repick();

   if (!ws->saved_texture)
     glDeleteTextures(1, &ws->texture);
   else
     glDeleteTextures(1, &ws->saved_texture);

   if (ws->buffer)
     wl_list_remove(&ws->buffer_destroy_listener.link);

   /* TODO: Destroy Image */

   wl_list_remove(&ws->buffer_link);
   pixman_region32_fini(&ws->damage);

   free(ws);
}

static void 
_e_mod_comp_wayland_destroy_buffer(struct wl_listener *listener, struct wl_resource *resource __UNUSED__, uint32_t timestamp __UNUSED__)
{
   Wayland_Surface *ws;

   ws = container_of(listener, Wayland_Surface, buffer_destroy_listener);
   ws->buffer = NULL;
}

static void 
_e_mod_comp_wayland_destroy_frame(struct wl_resource *resource)
{
   Wayland_Frame_Callback *cb;

   cb = resource->data;
   wl_list_remove(&cb->link);
   free(cb);
}

static void 
_e_mod_comp_wayland_damage_surface(Wayland_Surface *surface, int32_t x, int32_t y, int32_t width, int32_t height)
{
   pixman_region32_union_rect(&surface->damage, &surface->damage, 
                              surface->x + x, surface->y + y, width, height);
   e_mod_comp_wayland_repaint();
}

static void 
_e_mod_comp_wayland_shm_buffer_created(struct wl_buffer *buffer)
{
   struct wl_list *attached;

   printf("Shm Buffer Created\n");

   buffer->user_data = NULL;
   if ((attached = malloc(sizeof(*attached))))
     {
        wl_list_init(attached);
        buffer->user_data = attached;
     }
}

static void 
_e_mod_comp_wayland_shm_buffer_damaged(struct wl_buffer *buffer, int32_t x __UNUSED__, int32_t y __UNUSED__, int32_t width __UNUSED__, int32_t height __UNUSED__)
{
   Wayland_Surface *ws;
   struct wl_list *attached;
   GLsizei tex_width;

   printf("Shm Buffer Damaged\n");
   attached = buffer->user_data;
   tex_width = wl_shm_buffer_get_stride(buffer) / 4;
   wl_list_for_each(ws, attached, buffer_link)
     {
        glBindTexture(GL_TEXTURE_2D, ws->texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, 
                     tex_width, buffer->height, 0, 
                     GL_BGRA_EXT, GL_UNSIGNED_BYTE, 
                     wl_shm_buffer_get_data(buffer));
     }
}

static void 
_e_mod_comp_wayland_shm_buffer_destroyed(struct wl_buffer *buffer)
{
   Wayland_Surface *ws, *next;
   struct wl_list *attached;

   printf("Shm Buffer Destroyed\n");

   attached = buffer->user_data;
   wl_list_for_each_safe(ws, next, attached, buffer_link)
     {
        wl_list_remove(&ws->buffer_link);
        wl_list_init(&ws->buffer_link);
     }

   free(attached);
}

static void 
_e_mod_comp_wayland_buffer_attach(struct wl_buffer *buffer, struct wl_surface *surface)
{
   Wayland_Surface *ws;

   printf("Buffer Attach\n");

   ws = (Wayland_Surface *)surface;
   if (ws->saved_texture != 0) ws->texture = ws->saved_texture;
   glBindTexture(GL_TEXTURE_2D, ws->texture);
   if (wl_buffer_is_shm(buffer))
     {
        struct wl_list *attached;

        glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, 
                     wl_shm_buffer_get_stride(buffer) / 4, buffer->height, 0, 
                     GL_BGRA_EXT, GL_UNSIGNED_BYTE, 
                     wl_shm_buffer_get_data(buffer));
        switch (wl_shm_buffer_get_format(buffer))
          {
           case WL_SHM_FORMAT_ARGB8888:
             ws->visual = 0;
             break;
           case WL_SHM_FORMAT_XRGB8888:
             ws->visual = 1;
             break;
          }
        attached = buffer->user_data;
        wl_list_remove(&ws->buffer_link);
        wl_list_insert(attached, &ws->buffer_link);
     }
   else
     {
        /* TODO: Handle image */
     }
}

static void 
_e_mod_comp_wayland_shell_surface_move(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_resource, uint32_t timestamp)
{
   Wayland_Input *wi;
   Wayland_Shell_Surface *wss;

   wss = resource->data;
   wi = input_resource->data;
   if ((wi->input_dev.button_count == 0) || 
       (wi->input_dev.grab_time != timestamp) || 
       (wi->input_dev.pointer_focus != &wss->surface->surface))
     return;

   if (!_e_mod_comp_wayland_surface_move(wss->surface, wi, timestamp))
     wl_resource_post_no_memory(resource);
}

static void 
_e_mod_comp_wayland_shell_surface_resize(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *input_resource, uint32_t timestamp, uint32_t edges)
{
   Wayland_Input *wi;
   Wayland_Shell_Surface *wss;

   wi = input_resource->data;
   wss = resource->data;

   if ((wi->input_dev.button_count == 0) || 
       (wi->input_dev.grab_time != timestamp) | 
       (wi->input_dev.pointer_focus != &wss->surface->surface))
     return;

   if (!_e_mod_comp_wayland_surface_resize(wss->surface, wi, timestamp, edges))
     wl_resource_post_no_memory(resource);
}

static void 
_e_mod_comp_wayland_shell_surface_set_toplevel(struct wl_client *client __UNUSED__, struct wl_resource *resource)
{
   Wayland_Shell_Surface *wss;

   wss = resource->data;

   if (!_e_mod_comp_wayland_shell_surface_type_reset(wss))
     return;

   _e_mod_comp_wayland_damage_surface(wss->surface, 0, 0, wss->surface->width, 
                                      wss->surface->height);

   wss->type = SHELL_SURFACE_TOPLEVEL;
}

static void 
_e_mod_comp_wayland_shell_surface_set_transient(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *parent_resource, int x, int y, uint32_t flags __UNUSED__)
{
   Wayland_Shell_Surface *wss, *pwss;

   wss = resource->data;
   pwss = parent_resource->data;

   if (!_e_mod_comp_wayland_shell_surface_type_reset(wss))
     return;

   wss->surface->x = pwss->surface->x + x;
   wss->surface->y = pwss->surface->y + y;
   _e_mod_comp_wayland_damage_surface(wss->surface, 0, 0, wss->surface->width, 
                                      wss->surface->height);

   wss->type = SHELL_SURFACE_TRANSIENT;
}

static void 
_e_mod_comp_wayland_shell_surface_set_fullscreen(struct wl_client *client __UNUSED__, struct wl_resource *resource)
{
   Wayland_Shell_Surface *wss;

   wss = resource->data;

   if (!_e_mod_comp_wayland_shell_surface_type_reset(wss))
     return;

   wss->saved_x = wss->surface->x;
   wss->saved_y = wss->surface->y;
   wss->surface->x = (_wl_output->width - wss->surface->width) / 2;
   wss->surface->y = (_wl_output->height - wss->surface->height) / 2;
   _e_mod_comp_wayland_damage_surface(wss->surface, 0, 0, wss->surface->width, 
                                      wss->surface->height);
   wss->type = SHELL_SURFACE_FULLSCREEN;
}

static void 
_e_mod_comp_wayland_shell_surface_set_popup(struct wl_client *client, struct wl_resource *resource, struct wl_resource *input_resource, uint32_t timestamp, struct wl_resource *parent_resource, int32_t x, int32_t y, uint32_t flags)
{

}

static Eina_Bool 
_e_mod_comp_wayland_shell_surface_type_reset(Wayland_Shell_Surface *shell_surface)
{
   switch (shell_surface->type)
     {
      case SHELL_SURFACE_FULLSCREEN:
        shell_surface->surface->x = shell_surface->saved_x;
        shell_surface->surface->y = shell_surface->saved_y;
        break;
      case SHELL_SURFACE_PANEL:
      case SHELL_SURFACE_BACKGROUND:
        wl_list_remove(&shell_surface->link);
        wl_list_init(&shell_surface->link);
        break;
      case SHELL_SURFACE_SCREENSAVER:
      case SHELL_SURFACE_LOCK:
        wl_resource_post_error(&shell_surface->resource, 
                               WL_DISPLAY_ERROR_INVALID_METHOD,
                               "cannot reassign surface type");
        return EINA_FALSE;
        break;
      default:
        break;
     }
   shell_surface->type = SHELL_SURFACE_NONE;
   return EINA_TRUE;
}

static Wayland_Shell_Surface *
_e_mod_comp_wayland_get_shell_surface(Wayland_Surface *surface)
{
   struct wl_list *list;
   struct wl_listener *listener;

   list = &surface->surface.resource.destroy_listener_list;
   wl_list_for_each(listener, list, link)
     {
        if (listener->func == _e_mod_comp_wayland_shell_surface_destroy_handle)
          return container_of(listener, Wayland_Shell_Surface, 
                              surface_destroy_listener);
     }
   return NULL;
}
