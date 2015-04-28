#ifndef E_MOD_MAIN_H
# define E_MOD_MAIN_H

# define DLOGFNS 1

# ifdef DLOGFNS
#  include <stdio.h>
#  define DLOGFN(fl, ln, fn) printf("-E-DRM: %25s: %5i - %s\n", fl, ln, fn);
# else
#  define DLOGFN(fl, ln, fn)
# endif

# include <libudev.h>
# include <gbm.h>
# include <xf86drm.h>
# include <xf86drmMode.h>
# include <drm_fourcc.h>
# include <termios.h>
# include <linux/kd.h>
# include <linux/vt.h>
# include <linux/major.h>
# include <sys/ioctl.h>
# include <wayland-server.h>
# include <mtdev.h>

typedef void (*tty_vt_func_t)(E_Compositor *comp, int event);
typedef struct _E_Tty E_Tty;
typedef struct _E_Drm_Compositor E_Drm_Compositor;
typedef struct _E_Sprite E_Sprite;
typedef struct _E_Drm_Output_Mode E_Drm_Output_Mode;
typedef struct _E_Drm_Output E_Drm_Output;
typedef struct _E_Evdev_Input E_Evdev_Input;
typedef struct _E_Evdev_Input_Device E_Evdev_Input_Device;

struct _E_Drm_Compositor
{
   E_Compositor base;

   struct udev *udev;
   struct wl_event_source *drm_source;
   struct udev_monitor *udev_monitor;
   struct wl_event_source *udev_drm_source;
   struct 
     {
        int id, fd;
     } drm;
   struct gbm_device *gbm;
   unsigned int *crtcs;
   int num_crtcs;
   unsigned int crtc_alloc;
   unsigned int conn_alloc;

   E_Tty *tty;

   struct wl_list sprites;
   Eina_Bool sprites_broken;
   E_Compositor_State prev_state;
};

struct _E_Tty
{
   E_Compositor *comp;

   int fd;
   struct termios term_attribs;
   struct wl_event_source *input_source;
   struct wl_event_source *vt_source;
   tty_vt_func_t vt_func;
   int vt, start_vt;
   Eina_Bool has_vt : 1;
};

struct _E_Sprite
{
   struct wl_list link;

   unsigned int fb_id;
   unsigned int pending_fb_id;

   E_Surface *surface, *pending_surface;
   E_Drm_Compositor *compositor;

   struct wl_listener destroy_listener;
   struct wl_listener pending_destroy_listener;

   unsigned int possible_crtcs;
   unsigned int plane_id;

   int sx, sy;
   unsigned int sw, sh;
   unsigned int dx, dy, dw, dh;

   unsigned int format_count;
   unsigned int formats[];
};

struct _E_Drm_Output_Mode
{
   E_Output_Mode base;
   drmModeModeInfo info;
};

struct _E_Drm_Output
{
   E_Output base;

   unsigned int crtc_id;
   unsigned int conn_id;
   drmModeCrtcPtr orig_crtc;
   GLuint rbo[2];
   unsigned int fb_id[2];

   EGLImageKHR image[2];
   struct gbm_bo *bo[2];

   unsigned int current;
   unsigned int fs_surf_fb_id;
   unsigned int pending_fs_surf_fb_id;

   struct wl_buffer *scanout_buffer, *pending_scanout_buffer;
   struct wl_listener scanout_buffer_destroy_listener;
   struct wl_listener pending_scanout_buffer_destroy_listener;

   /* TODO: backlight */
};

struct _E_Evdev_Input
{
   E_Input_Device base;
   struct wl_list devices;
   struct udev_monitor *monitor;
   char *seat;
};

struct _E_Evdev_Input_Device
{
   E_Evdev_Input *master;
   struct wl_list link;
   struct wl_event_source *source;
   E_Output *output;
   char *devnode;
   int fd;
   struct 
     {
        int min_x, max_x, min_y, max_y;
        int ox, oy, rx, ry;
        int x, y;
     } absolute;
   struct 
     {
        int slot;
        int x[16], y[16];
     } mt;
   struct mtdev *mtdev;
   struct 
     {
        int dx, dy;
     } rel;

   int type;

   Eina_Bool is_pad : 1;
   Eina_Bool is_mt : 1;
};

extern E_Drm_Compositor *_comp;

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

EINTERN E_Sprite *e_sprite_create(E_Drm_Compositor *dcomp, drmModePlane *plane);
EINTERN Eina_Bool e_sprite_crtc_supported(E_Output *output, unsigned int supported);

EINTERN E_Tty *e_tty_create(E_Compositor *comp, tty_vt_func_t vt_func, int tty);
EINTERN void e_tty_destroy(E_Tty *et);

EINTERN int e_drm_output_subpixel_convert(int value);
EINTERN Eina_Bool e_drm_output_add_mode(E_Drm_Output *output, drmModeModeInfo *info);
EINTERN void e_drm_output_set_modes(E_Drm_Compositor *dcomp);
EINTERN void e_drm_output_scanout_buffer_destroy(struct wl_listener *listener, void *data);
EINTERN void e_drm_output_pending_scanout_buffer_destroy(struct wl_listener *listener, void *data);
EINTERN void e_drm_output_repaint(E_Output *base, pixman_region32_t *damage);
EINTERN void e_drm_output_destroy(E_Output *base);
EINTERN void e_drm_output_assign_planes(E_Output *base);
EINTERN void e_drm_output_set_dpms(E_Output *base, E_Dpms_Level level);
EINTERN Eina_Bool e_drm_output_prepare_scanout_surface(E_Drm_Output *output);
EINTERN void e_drm_output_disable_sprites(E_Output *base);
EINTERN drmModePropertyPtr e_drm_output_get_property(int fd, drmModeConnectorPtr conn, const char *name);
EINTERN int e_drm_output_prepare_overlay_surface(E_Output *base, E_Surface *es, pixman_region32_t *overlap);
EINTERN Eina_Bool e_drm_output_surface_transform_supported(E_Surface *es);
EINTERN Eina_Bool e_drm_output_surface_overlap_supported(E_Output *base __UNUSED__, pixman_region32_t *overlap);
EINTERN Eina_Bool e_drm_output_surface_format_supported(E_Sprite *s, unsigned int format);
EINTERN void e_drm_output_set_cursor_region(E_Output *output, E_Input_Device *device, pixman_region32_t *overlap);
EINTERN Eina_Bool e_drm_output_set_cursor(E_Output *output, E_Input_Device *device);

EINTERN void e_evdev_add_devices(struct udev *udev, E_Input_Device *base);
EINTERN void e_evdev_remove_devices(E_Input_Device *base);
EINTERN void e_evdev_input_create(E_Compositor *comp, struct udev *udev, const char *seat);
EINTERN void e_evdev_input_destroy(E_Input_Device *base);

#endif
