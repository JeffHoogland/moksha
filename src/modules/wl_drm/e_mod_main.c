#include "e.h"
#include "e_mod_main.h"

/* local function prototypes */
static void _cb_tty(E_Compositor *comp, int event);
static int _cb_drm_input(int fd, unsigned int mask __UNUSED__, void *data __UNUSED__);
static int _cb_drm_udev_event(int fd __UNUSED__, unsigned int mask __UNUSED__, void *data);
static void _cb_drm_page_flip(int fd __UNUSED__, unsigned int frame __UNUSED__, unsigned int sec, unsigned int usec, void *data);
static void _cb_drm_vblank(int fd __UNUSED__, unsigned int frame __UNUSED__, unsigned int sec __UNUSED__, unsigned int usec __UNUSED__, void *data);
static Eina_Bool _egl_init(E_Drm_Compositor *dcomp, struct udev_device *dev);
static void _sprites_init(E_Drm_Compositor *dcomp);
static void _sprites_shutdown(E_Drm_Compositor *dcomp);
static Eina_Bool _outputs_init(E_Drm_Compositor *dcomp, unsigned int conn, struct udev_device *drm_device);
static Eina_Bool _output_create(E_Drm_Compositor *dcomp, drmModeRes *res, drmModeConnector *conn, int x, int y, struct udev_device *drm_device);
static void _outputs_update(E_Drm_Compositor *dcomp, struct udev_device *drm_device);
static Eina_Bool _udev_event_is_hotplug(E_Drm_Compositor *dcomp, struct udev_device *dev);

/* local variables */
static drmModeModeInfo builtin_mode = 
{
   63500,			/* clock */
   1024, 1072, 1176, 1328, 0,
   768, 771, 775, 798, 0,
   59920,
   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_PVSYNC,
   0,
   "1024x768"
};

/* external variables */
E_Drm_Compositor *_drm_comp = NULL;

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Drm" };

EAPI void *
e_modapi_init(E_Module *m)
{
   struct wl_display *disp;
   struct udev_enumerate *ue;
   struct udev_list_entry *entry;
   struct udev_device *drm_dev = NULL;
   struct wl_event_loop *loop;
   const char *seat = NULL;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the wayland display */
   if (!(disp = (struct wl_display *)m->data)) return NULL;

   /* allocate space for the drm compositor */
   if (!(_drm_comp = malloc(sizeof(E_Drm_Compositor)))) return NULL;

   memset(_drm_comp, 0, sizeof(E_Drm_Compositor));

   if (!(_drm_comp->udev = udev_new())) 
     {
        free(_drm_comp);
        return NULL;
     }

   _drm_comp->base.display = disp;
   if (!(_drm_comp->tty = e_tty_create(&_drm_comp->base, _cb_tty, 0)))
     {
        free(_drm_comp);
        return NULL;
     }

   ue = udev_enumerate_new(_drm_comp->udev);
   udev_enumerate_add_match_subsystem(ue, "drm");
   udev_enumerate_add_match_sysname(ue, "card[0-9]*");

   udev_enumerate_scan_devices(ue);
   udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(ue))
     {
        struct udev_device *dev;
        const char *path = NULL;

        path = udev_list_entry_get_name(entry);
        dev = udev_device_new_from_syspath(_drm_comp->udev, path);
        if (!(seat = udev_device_get_property_value(dev, "ID_SEAT")))
          seat = "seat0";
        if (!strcmp(seat, "seat0")) 
          {
             drm_dev = dev;
             break;
          }
        udev_device_unref(dev);
     }

   if (!drm_dev)
     {
        free(_drm_comp);
        return NULL;
     }

   /* init egl */
   if (!_egl_init(_drm_comp, drm_dev))
     {
        free(_drm_comp);
        return NULL;
     }

   /* _drm_comp->base.destroy = _cb_destroy; */
   _drm_comp->base.focus = EINA_TRUE;

   _drm_comp->prev_state = E_COMPOSITOR_STATE_ACTIVE;

   glGenFramebuffers(1, &_drm_comp->base.fbo);
   glBindFramebuffer(GL_FRAMEBUFFER, _drm_comp->base.fbo);

   if (!e_compositor_init(&_drm_comp->base, disp))
     {
        free(_drm_comp);
        return NULL;
     }

   wl_list_init(&_drm_comp->sprites);
   _sprites_init(_drm_comp);

   if (!_outputs_init(_drm_comp, 0, drm_dev))
     {
        free(_drm_comp);
        return NULL;
     }

   udev_device_unref(drm_dev);
   udev_enumerate_unref(ue);

   e_evdev_input_create(&_drm_comp->base, _drm_comp->udev, seat);

   loop = wl_display_get_event_loop(_drm_comp->base.display);
   _drm_comp->drm_source = 
     wl_event_loop_add_fd(loop, _drm_comp->drm.fd, WL_EVENT_READABLE, 
                          _cb_drm_input, _drm_comp);

   _drm_comp->udev_monitor = 
     udev_monitor_new_from_netlink(_drm_comp->udev, "udev");
   if (!_drm_comp->udev_monitor)
     {
        free(_drm_comp);
        return NULL;
     }
   udev_monitor_filter_add_match_subsystem_devtype(_drm_comp->udev_monitor, 
                                                   "drm", NULL);

   _drm_comp->udev_drm_source = 
     wl_event_loop_add_fd(loop, udev_monitor_get_fd(_drm_comp->udev_monitor), 
                          WL_EVENT_READABLE, _cb_drm_udev_event, _drm_comp);
   if (udev_monitor_enable_receiving(_drm_comp->udev_monitor) < 0)
     {
        free(_drm_comp);
        return NULL;
     }

   return &_drm_comp->base;
}

EAPI int 
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   E_Input_Device *input, *next;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   e_compositor_shutdown(&_drm_comp->base);
   gbm_device_destroy(_drm_comp->gbm);
   _sprites_shutdown(_drm_comp);
   drmDropMaster(_drm_comp->drm.fd);
   e_tty_destroy(_drm_comp->tty);

   wl_list_for_each_safe(input, next, &_drm_comp->base.inputs, link)
     e_evdev_input_destroy(input);

   free(_drm_comp);
   _drm_comp = NULL;

   return 1;
}

EAPI int 
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}

/* local function prototypes */
static void 
_cb_tty(E_Compositor *comp, int event)
{
   E_Output *output;
   E_Drm_Output *doutput;
   E_Sprite *sprite;
   E_Input_Device *input;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   switch (event)
     {
      case 0: // TTY_ENTER_VT
        comp->focus = EINA_TRUE;
        if (drmSetMaster(_drm_comp->drm.fd))
          {
             printf("Failed to set master: %m\n");
             wl_display_terminate(comp->display);
          }
        comp->state = _drm_comp->prev_state;
        e_drm_output_set_modes(_drm_comp);
        e_compositor_damage_all(comp);
        wl_list_for_each(input, &comp->inputs, link)
          e_evdev_add_devices(_drm_comp->udev, input);
        break;
      case 1: // TTY_LEAVE_VT
        comp->focus = EINA_FALSE;
        _drm_comp->prev_state = comp->state;
        comp->state = E_COMPOSITOR_STATE_SLEEPING;
        wl_list_for_each(output, &comp->outputs, link)
          {
             output->repaint_needed = EINA_FALSE;
             e_drm_output_set_cursor(output, NULL);
          }
        doutput = 
          container_of(comp->outputs.next, E_Drm_Output, base.link);

        wl_list_for_each(sprite, &_drm_comp->sprites, link)
          drmModeSetPlane(_drm_comp->drm.fd, sprite->plane_id, 
                          doutput->crtc_id, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

        wl_list_for_each(input, &comp->inputs, link)
          e_evdev_remove_devices(input);

        if (drmDropMaster(_drm_comp->drm.fd < 0))
          printf("Failed to drop master: %m\n");

        break;
     }
}

static int 
_cb_drm_input(int fd, unsigned int mask __UNUSED__, void *data __UNUSED__)
{
   drmEventContext ectxt;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   memset(&ectxt, 0, sizeof(ectxt));

   ectxt.version = DRM_EVENT_CONTEXT_VERSION;
   ectxt.page_flip_handler = _cb_drm_page_flip;
   ectxt.vblank_handler = _cb_drm_vblank;
   drmHandleEvent(fd, &ectxt);

   return 1;
}

static int 
_cb_drm_udev_event(int fd __UNUSED__, unsigned int mask __UNUSED__, void *data)
{
   E_Drm_Compositor *dcomp;
   struct udev_device *event;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(dcomp = data)) return 1;
   event = udev_monitor_receive_device(dcomp->udev_monitor);
   if (_udev_event_is_hotplug(dcomp, event))
     _outputs_update(dcomp, event);
   udev_device_unref(event);
   return 1;
}

static void 
_cb_drm_page_flip(int fd __UNUSED__, unsigned int frame __UNUSED__, unsigned int sec, unsigned int usec, void *data)
{
   E_Drm_Output *doutput;
   E_Drm_Compositor *dcomp;
   unsigned int msecs;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(doutput = data)) return;
   dcomp = (E_Drm_Compositor *)doutput->base.compositor;

   if (doutput->scanout_buffer)
     {
        e_buffer_post_release(doutput->scanout_buffer);
        wl_list_remove(&doutput->scanout_buffer_destroy_listener.link);
        doutput->scanout_buffer = NULL;
        drmModeRmFB(dcomp->drm.fd, doutput->fs_surf_fb_id);
        doutput->fs_surf_fb_id = 0;
     }

   if (doutput->pending_scanout_buffer)
     {
        doutput->scanout_buffer = doutput->pending_scanout_buffer;
        wl_list_remove(&doutput->pending_scanout_buffer_destroy_listener.link);
        wl_signal_add(&doutput->scanout_buffer->resource.destroy_signal,
                       &doutput->scanout_buffer_destroy_listener);
        doutput->pending_scanout_buffer = NULL;
        doutput->fs_surf_fb_id = doutput->pending_fs_surf_fb_id;
        doutput->pending_fs_surf_fb_id = 0;
     }

   msecs = sec * 1000 + usec / 1000;
   e_output_finish_frame(&doutput->base, msecs);
}

static void 
_cb_drm_vblank(int fd __UNUSED__, unsigned int frame __UNUSED__, unsigned int sec __UNUSED__, unsigned int usec __UNUSED__, void *data)
{
   E_Sprite *es;
   E_Drm_Compositor *dcomp;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(es = data)) return;
   dcomp = es->compositor;

   if (es->surface)
     {
        e_buffer_post_release(es->surface->buffer);
        wl_list_remove(&es->destroy_listener.link);
        es->surface = NULL;
        drmModeRmFB(dcomp->drm.fd, es->fb_id);
        es->fb_id = 0;
     }

   if (es->pending_surface)
     {
        wl_list_remove(&es->pending_destroy_listener.link);
        wl_signal_add(&es->pending_surface->buffer->resource.destroy_signal,
                       &es->destroy_listener);
        es->surface = es->pending_surface;
        es->pending_surface = NULL;
        es->fb_id = es->pending_fb_id;
        es->pending_fb_id = 0;
     }
}

static Eina_Bool 
_egl_init(E_Drm_Compositor *dcomp, struct udev_device *dev)
{
   EGLint maj, min;
   const char *ext, *fname, *snum;
   int fd = 0;
   static const EGLint ctxt_att[] = 
     {
        EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
     };

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if ((snum = udev_device_get_sysnum(dev)))
     dcomp->drm.id = atoi(snum);
   if ((!snum) || (dcomp->drm.id < 0)) 
     return EINA_FALSE;

   fname = udev_device_get_devnode(dev);
   if (!(fd = open(fname, (O_RDWR | O_CLOEXEC))))
     return EINA_FALSE;

   dcomp->drm.fd = fd;
   dcomp->gbm = gbm_create_device(dcomp->drm.fd);

//   dcomp->base.egl_display = eglGetDisplay(dcomp->base.display);
   dcomp->base.egl_display = eglGetDisplay(dcomp->gbm);
   if (!dcomp->base.egl_display)
     return EINA_FALSE;

   if (!eglInitialize(dcomp->base.egl_display, &maj, &min))
     return EINA_FALSE;

   ext = eglQueryString(dcomp->base.egl_display, EGL_EXTENSIONS);
   if (!strstr(ext, "EGL_KHR_surfaceless_gles2"))
     return EINA_FALSE;

   if (!eglBindAPI(EGL_OPENGL_ES_API)) return EINA_FALSE;

   dcomp->base.egl_context = 
     eglCreateContext(dcomp->base.egl_display, NULL, EGL_NO_CONTEXT, ctxt_att);
   if (!dcomp->base.egl_context) return EINA_FALSE;

   if (!eglMakeCurrent(dcomp->base.egl_display, EGL_NO_SURFACE, 
                       EGL_NO_SURFACE, dcomp->base.egl_context))
     return EINA_FALSE;

   return EINA_TRUE;
}

static void 
_sprites_init(E_Drm_Compositor *dcomp)
{
   E_Sprite *es;
   drmModePlaneRes *res;
   drmModePlane *plane;
   unsigned int i = 0;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(res = drmModeGetPlaneResources(dcomp->drm.fd)))
     return;

   for (i = 0; i < res->count_planes; i++)
     {
        if (!(plane = drmModeGetPlane(dcomp->drm.fd, res->planes[i])))
          continue;
        if (!(es = e_sprite_create(dcomp, plane))) 
          {
             free(plane);
             continue;
          }
        drmModeFreePlane(plane);
        wl_list_insert(&dcomp->sprites, &es->link);
     }
   free(res->planes);
   free(res);
}

static void 
_sprites_shutdown(E_Drm_Compositor *dcomp)
{
   E_Drm_Output *doutput;
   E_Sprite *es, *next;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   doutput = container_of(dcomp->base.outputs.next, E_Drm_Output, base.link);

   wl_list_for_each_safe(es, next, &dcomp->sprites, link)
     {
        drmModeSetPlane(dcomp->drm.fd, es->plane_id, doutput->crtc_id, 
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        drmModeRmFB(dcomp->drm.fd, es->fb_id);
        free(es);
     }
}

static Eina_Bool 
_outputs_init(E_Drm_Compositor *dcomp, unsigned int conn, struct udev_device *drm_device)
{
   drmModeConnector *connector;
   drmModeRes *res;
   int i = 0, x = 0, y = 0;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(res = drmModeGetResources(dcomp->drm.fd)))
     return EINA_FALSE;

   if (!(dcomp->crtcs = calloc(res->count_crtcs, sizeof(unsigned int))))
     {
        drmModeFreeResources(res);
        return EINA_FALSE;
     }

   dcomp->num_crtcs = res->count_crtcs;
   memcpy(dcomp->crtcs, res->crtcs, sizeof(unsigned int) * dcomp->num_crtcs);

   for (i = 0; i < res->count_connectors; i++)
     {
        if (!(connector = 
              drmModeGetConnector(dcomp->drm.fd, res->connectors[i])))
          continue;
        if ((connector->connection == DRM_MODE_CONNECTED) && 
            ((conn == 0) || (connector->connector_id == conn)))
          {
             if (!_output_create(dcomp, res, connector, x, y, drm_device))
               {
                  drmModeFreeConnector(connector);
                  continue;
               }
             x += container_of(dcomp->base.outputs.prev, E_Output, 
                               link)->current->w;
          }
        drmModeFreeConnector(connector);
     }

   if (wl_list_empty(&dcomp->base.outputs))
     {
        drmModeFreeResources(res);
        return EINA_FALSE;
     }

   drmModeFreeResources(res);

   return EINA_TRUE;
}

static Eina_Bool 
_output_create(E_Drm_Compositor *dcomp, drmModeRes *res, drmModeConnector *conn, int x, int y, struct udev_device *drm_device)
{
   E_Drm_Output *output;
   E_Drm_Output_Mode *mode, *next;
   drmModeEncoder *encoder;
   int i = 0, ret = 0;
   unsigned int hdl, stride;

   if (!(encoder = drmModeGetEncoder(dcomp->drm.fd, conn->encoders[0])))
     return EINA_FALSE;

   for (i = 0; i < res->count_crtcs; i++)
     {
        if ((encoder->possible_crtcs & (1 << i)) && 
            !(dcomp->crtc_alloc & (1 << res->crtcs[i])))
          break;
     }
   if (i == res->count_crtcs)
     {
        drmModeFreeEncoder(encoder);
        return EINA_FALSE;
     }

   if (!(output = malloc(sizeof(E_Drm_Output))))
     {
        drmModeFreeEncoder(encoder);
        return EINA_FALSE;
     }

   memset(output, 0, sizeof(E_Drm_Output));

   output->fb_id[0] = -1;
   output->fb_id[1] = -1;
   output->base.subpixel = e_drm_output_subpixel_convert(conn->subpixel);
   output->base.make = "unknown";
   output->base.model = "unknown";
   wl_list_init(&output->base.modes);

   output->crtc_id = res->crtcs[i];
   dcomp->crtc_alloc |= (1 << output->crtc_id);
   output->conn_id = conn->connector_id;
   dcomp->conn_alloc |= (1 << output->conn_id);

   output->orig_crtc = drmModeGetCrtc(dcomp->drm.fd, output->crtc_id);
   drmModeFreeEncoder(encoder);

   for (i = 0; i < conn->count_modes; i++)
     {
        if (!e_drm_output_add_mode(output, &conn->modes[i]))
          goto efree;
     }

   if (conn->count_modes == 0)
     {
        if (!e_drm_output_add_mode(output, &builtin_mode))
          goto efree;
     }

   mode = container_of(output->base.modes.next, E_Drm_Output_Mode, base.link);
   output->base.current = &mode->base;
   mode->base.flags = (WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED);

   glGenRenderbuffers(2, output->rbo);

   for (i = 0; i < 2; i++)
     {
        glBindRenderbuffer(GL_RENDERBUFFER, output->rbo[i]);
        output->bo[i] = 
          gbm_bo_create(dcomp->gbm, output->base.current->w, 
                        output->base.current->h, GBM_BO_FORMAT_XRGB8888, 
                        (GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING));
        if (!output->bo[i]) goto ebuffs;
        output->image[i] = 
          dcomp->base.create_image(dcomp->base.egl_display, NULL, 
                                   EGL_NATIVE_PIXMAP_KHR, output->bo[i], NULL);
        if (!output->image[i]) goto ebuffs;
        dcomp->base.image_target_renderbuffer_storage(GL_RENDERBUFFER, 
                                                      output->image[i]);
        stride = gbm_bo_get_pitch(output->bo[i]);
        hdl = gbm_bo_get_handle(output->bo[i]).u32;
        ret = drmModeAddFB(dcomp->drm.fd, output->base.current->w, 
                           output->base.current->h, 24, 32, stride, hdl, 
                           &output->fb_id[i]);
        if (ret) goto ebuffs;
     }

   output->current = 0;
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                             GL_RENDERBUFFER, output->rbo[output->current]);
   ret = drmModeSetCrtc(dcomp->drm.fd, output->crtc_id, 
                        output->fb_id[output->current ^ 1], 0, 0, 
                        &output->conn_id, 1, &mode->info);
   if (ret) goto efb;

   /* TODO: backlight init */

   e_output_init(&output->base, &dcomp->base, x, y, 
                 conn->mmWidth, conn->mmHeight, 0);

   wl_list_insert(dcomp->base.outputs.prev, &output->base.link);

   output->scanout_buffer_destroy_listener.notify = 
     e_drm_output_scanout_buffer_destroy;
   output->pending_scanout_buffer_destroy_listener.notify = 
     e_drm_output_pending_scanout_buffer_destroy;

   output->pending_fs_surf_fb_id = 0;
   output->base.repaint = e_drm_output_repaint;
   output->base.destroy = e_drm_output_destroy;
   output->base.assign_planes = e_drm_output_assign_planes;
   output->base.set_dpms = e_drm_output_set_dpms;

   return EINA_TRUE;

efb:
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                             GL_RENDERBUFFER, 0);

ebuffs:
   for (i = 0; i < 2; i++)
     {
        if ((int)output->fb_id[i] != -1)
          drmModeRmFB(dcomp->drm.fd, output->fb_id[i]);
        if (output->image[i])
          dcomp->base.destroy_image(dcomp->base.egl_display, output->image[i]);
        if (output->bo[i]) gbm_bo_destroy(output->bo[i]);
     }
   glBindRenderbuffer(GL_RENDERBUFFER, 0);
   glDeleteRenderbuffers(2, output->rbo);

efree:
   wl_list_for_each_safe(mode, next, &output->base.modes, base.link)
     {
        wl_list_remove(&mode->base.link);
        free(mode);
     }
   drmModeFreeCrtc(output->orig_crtc);
   dcomp->crtc_alloc &= ~(1 << output->crtc_id);
   dcomp->conn_alloc &= ~(1 << output->conn_id);
   free(output);

   return EINA_TRUE;
}

static void 
_outputs_update(E_Drm_Compositor *dcomp, struct udev_device *drm_device)
{
   drmModeConnector *conn;
   drmModeRes *res;
   E_Drm_Output *doutput, *next;
   int x = 0, y = 0;
   int xo = 0, yo = 0;
   unsigned int connected = 0, disconnects = 0;
   int i = 0;

   if (!(res = drmModeGetResources(dcomp->drm.fd)))
     return;

   for (i = 0; i < res->count_connectors; i++)
     {
        int conn_id;

        conn_id = res->connectors[i];
        if (!(conn = drmModeGetConnector(dcomp->drm.fd, conn_id)))
          continue;
        if (conn->connection != DRM_MODE_CONNECTED)
          {
             drmModeFreeConnector(conn);
             continue;
          }
        connected |= (1 << conn_id);
        if (!(dcomp->conn_alloc & (1 << conn_id)))
          {
             E_Output *last;

             last = container_of(dcomp->base.outputs.prev, E_Output, link);
             if (!wl_list_empty(&dcomp->base.outputs))
               x = last->x + last->current->w;
             else
               x = 0;
             y = 0;
             _output_create(dcomp, res, conn, x, y, drm_device);
          }
        drmModeFreeConnector(conn);
     }
   drmModeFreeResources(res);

   if ((disconnects = dcomp->conn_alloc & ~connected))
     {
        wl_list_for_each_safe(doutput, next, &dcomp->base.outputs, base.link)
          {
             if ((xo != 0) || (yo != 0))
               e_output_move(&doutput->base, doutput->base.x - xo, 
                             doutput->base.y - yo);
             if (disconnects & (1 << doutput->conn_id))
               {
                  disconnects &= ~(1 << doutput->conn_id);
                  xo += doutput->base.current->w;
                  e_drm_output_destroy(&doutput->base);
               }
          }
     }

   if (dcomp->conn_alloc == 0)
     wl_display_terminate(dcomp->base.display);
}

static Eina_Bool 
_udev_event_is_hotplug(E_Drm_Compositor *dcomp, struct udev_device *dev)
{
   const char *snum, *val;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   snum = udev_device_get_sysnum(dev);
   if ((!snum) || (atoi(snum) != dcomp->drm.id)) 
     return EINA_FALSE;

   if (!(val = udev_device_get_property_value(dev, "HOTPLUG")))
     return EINA_FALSE;

   if (!strcmp(val, "1")) return EINA_TRUE;

   return EINA_FALSE;
}
