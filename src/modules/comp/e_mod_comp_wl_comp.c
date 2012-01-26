#include "e.h"
#include "e_mod_main.h"
#ifdef HAVE_WAYLAND
# include "e_mod_comp_wl.h"
# include "e_mod_comp_wl_comp.h"
# include "e_mod_comp_wl_shm.h"
# include "e_mod_comp_wl_surface.h"
#endif

/* local function prototypes */
static Eina_Bool _e_mod_comp_wl_comp_egl_init(void);
static void _e_mod_comp_wl_comp_egl_shutdown(void);
static void _e_mod_comp_wl_comp_destroy(void);
static void _e_mod_comp_wl_comp_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id);
static void _e_mod_comp_wl_comp_surface_create(struct wl_client *client, struct wl_resource *resource, uint32_t id);

/* wayland interfaces */
static const struct wl_compositor_interface _wl_comp_interface = 
{
   _e_mod_comp_wl_comp_surface_create
};
static const struct wl_surface_interface _wl_surface_interface = 
{
   e_mod_comp_wl_surface_destroy,
   e_mod_comp_wl_surface_attach,
   e_mod_comp_wl_surface_damage,
   e_mod_comp_wl_surface_frame
};

/* private variables */
static Wayland_Compositor *_wl_comp;

Eina_Bool 
e_mod_comp_wl_comp_init(void)
{
   const char *extensions;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(_wl_comp = malloc(sizeof(Wayland_Compositor))))
     {
        EINA_LOG_ERR("Could not allocate space for compositor\n");
        return EINA_FALSE;
     }

   memset(_wl_comp, 0, sizeof(*_wl_comp));

   if (!_e_mod_comp_wl_comp_egl_init())
     {
        EINA_LOG_ERR("Could not initialize egl\n");
        free(_wl_comp);
        return EINA_FALSE;
     }

   _wl_comp->destroy = _e_mod_comp_wl_comp_destroy;

   if (!wl_display_add_global(_wl_disp, &wl_compositor_interface, _wl_comp, 
                              _e_mod_comp_wl_comp_bind))
     {
        EINA_LOG_ERR("Failed to add compositor to wayland\n");
        free(_wl_comp);
        return EINA_FALSE;
     }

   _wl_comp->image_target_texture_2d =
     (void *)eglGetProcAddress("glEGLImageTargetTexture2DOES");
   _wl_comp->image_target_renderbuffer_storage = (void *)
     eglGetProcAddress("glEGLImageTargetRenderbufferStorageOES");
   _wl_comp->create_image = (void *)eglGetProcAddress("eglCreateImageKHR");
   _wl_comp->destroy_image = (void *)eglGetProcAddress("eglDestroyImageKHR");

   _wl_comp->bind_display = 
     (void *)eglGetProcAddress("eglBindWaylandDisplayWL");
   _wl_comp->unbind_display = 
     (void *)eglGetProcAddress("eglUnbindWaylandDisplayWL");

   extensions = (const char *)glGetString(GL_EXTENSIONS);
   if (!strstr(extensions, "GL_EXT_texture_format_BGRA8888"))
     {
        EINA_LOG_ERR("GL_EXT_texture_format_BGRA8888 not available\n");
        free(_wl_comp);
        return EINA_FALSE;
     }

   extensions = 
     (const char *)eglQueryString(_wl_comp->egl.display, EGL_EXTENSIONS);
   if (strstr(extensions, "EGL_WL_bind_wayland_display"))
     _wl_comp->has_bind = EINA_TRUE;
   if (_wl_comp->has_bind) 
     _wl_comp->bind_display(_wl_comp->egl.display, _wl_disp);

   wl_data_device_manager_init(_wl_disp);

   wl_list_init(&_wl_comp->surfaces);

   return EINA_TRUE;
}

void 
e_mod_comp_wl_comp_shutdown(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (_wl_comp) _wl_comp->destroy();
}

Wayland_Compositor *
e_mod_comp_wl_comp_get(void)
{
   return _wl_comp;
}

/* local functions */
static Eina_Bool 
_e_mod_comp_wl_comp_egl_init(void)
{
   EGLint major, minor, n;
   const char *extensions;
   EGLint config_attribs[] = 
     {
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, 
        EGL_ALPHA_SIZE, 1, EGL_DEPTH_SIZE, 0, EGL_STENCIL_SIZE, 0, 
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_SURFACE_TYPE, 
        EGL_WINDOW_BIT, EGL_NONE
     };
   EGLint context_attribs[] = 
     {
        EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
     };

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(_wl_comp->egl.display = eglGetDisplay(NULL)))
     {
        EINA_LOG_ERR("Failed to create EGL display\n");
        return EINA_FALSE;
     }

   if (!eglInitialize(_wl_comp->egl.display, &major, &minor))
     {
        EINA_LOG_ERR("Failed to initialize EGL\n");
        eglTerminate(_wl_comp->egl.display);
        return EINA_FALSE;
     }

   extensions = eglQueryString(_wl_comp->egl.display, EGL_EXTENSIONS);
   if (!strstr(extensions, "EGL_KHR_surfaceless_gles2"))
     {
        EINA_LOG_ERR("EGL_KHR_surfaceless_gles2 not supported\n");
        eglTerminate(_wl_comp->egl.display);
        return EINA_FALSE;
     }

   if (!eglBindAPI(EGL_OPENGL_ES_API))
     {
        EINA_LOG_ERR("Failed to bind EGL API\n");
        eglTerminate(_wl_comp->egl.display);
        return EINA_FALSE;
     }

   if ((!eglChooseConfig(_wl_comp->egl.display, config_attribs, 
                        &_wl_comp->egl.config, 1, &n) || (n == 0)))
     {
        EINA_LOG_ERR("Failed to choose EGL config\n");
        eglTerminate(_wl_comp->egl.display);
        return EINA_FALSE;
     }

   if (!(_wl_comp->egl.context = 
         eglCreateContext(_wl_comp->egl.display, _wl_comp->egl.config, 
                          EGL_NO_CONTEXT, context_attribs)))
     {
        EINA_LOG_ERR("Failed to create EGL context\n");
        eglTerminate(_wl_comp->egl.display);
        return EINA_FALSE;
     }

   if (!eglMakeCurrent(_wl_comp->egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, 
                       _wl_comp->egl.context))
     {
        EINA_LOG_ERR("Failed to make EGL context current\n");
        eglTerminate(_wl_comp->egl.display);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

static void 
_e_mod_comp_wl_comp_egl_shutdown(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   eglMakeCurrent(_wl_comp->egl.display, EGL_NO_SURFACE, EGL_NO_SURFACE, 
                  EGL_NO_CONTEXT);
   eglTerminate(_wl_comp->egl.display);
   eglReleaseThread();
}

static void 
_e_mod_comp_wl_comp_destroy(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (_wl_comp->has_bind)
     _wl_comp->unbind_display(_wl_comp->egl.display, _wl_disp);

   _e_mod_comp_wl_comp_egl_shutdown();

   if (&_wl_comp->surfaces) wl_list_remove(&_wl_comp->surfaces);
   free(_wl_comp);
}

static void 
_e_mod_comp_wl_comp_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wl_client_add_object(client, &wl_compositor_interface, 
                        &_wl_comp_interface, id, data);
}

static void 
_e_mod_comp_wl_comp_surface_create(struct wl_client *client, struct wl_resource *resource, uint32_t id)
{
   Wayland_Surface *ws;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(ws = e_mod_comp_wl_surface_create(0, 0, 0, 0)))
     {
        wl_resource_post_no_memory(resource);
        return;
     }

   ws->surface.resource.destroy = e_mod_comp_wl_surface_destroy_surface;
   ws->surface.resource.object.id = id;
   ws->surface.resource.object.interface = &wl_surface_interface;
   ws->surface.resource.object.implementation = 
     (void (**)(void))&_wl_surface_interface;
   ws->surface.resource.data = ws;

   wl_client_add_resource(client, &ws->surface.resource);
}
