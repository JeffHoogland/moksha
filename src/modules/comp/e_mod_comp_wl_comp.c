#include "e.h"
#include "e_mod_main.h"
#ifdef HAVE_WAYLAND
# include "e_mod_comp_wl.h"
# include "e_mod_comp_wl_comp.h"
# include "e_mod_comp_wl_shm.h"
# include "e_mod_comp_wl_input.h"
# include "e_mod_comp_wl_output.h"
# include "e_mod_comp_wl_surface.h"
#endif

/* local function prototypes */
static Eina_Bool _e_mod_comp_wl_comp_egl_init(void);
static void _e_mod_comp_wl_comp_egl_shutdown(void);
static Eina_Bool _e_mod_comp_wl_comp_shader_init(Wayland_Shader *shader, const char *vertex_source, const char *fragment_source);
/* static void _e_mod_comp_wl_comp_shader_shutdown(void); */
static Eina_Bool _e_mod_comp_wl_comp_solid_shader_init(Wayland_Shader *shader, GLuint vertex_shader, const char *fragment_source);
/* static void _e_mod_comp_wl_comp_solid_shader_shutdown(); */
static int _e_mod_comp_wl_comp_compile_shader(GLenum type, const char *source);
static void _e_mod_comp_wl_comp_destroy(void);
static void _e_mod_comp_wl_comp_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id);
static void _e_mod_comp_wl_comp_surface_create(struct wl_client *client, struct wl_resource *resource, uint32_t id);
static int _e_mod_comp_wl_comp_idle_handler(void *data);

/* wayland interfaces */
static const struct wl_compositor_interface _wl_comp_interface = 
{
   _e_mod_comp_wl_comp_surface_create
};

static const struct wl_shm_callbacks _wl_shm_callbacks = 
{
   e_mod_comp_wl_shm_buffer_created,
   e_mod_comp_wl_shm_buffer_damaged,
   e_mod_comp_wl_shm_buffer_destroyed
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
static const char vertex_shader[] = 
{
   "uniform mat4 proj;\n"
   "attribute vec2 position;\n"
   "attribute vec2 texcoord;\n"
   "varying vec2 v_texcoord;\n"
   "void main()\n"
   "{\n"
   "   gl_Position = proj * vec4(position, 0.0, 1.0);\n"
   "   v_texcoord = texcoord;\n"
   "}\n"
};

static const char texture_fragment_shader[] = 
{
   "precision mediump float;\n"
   "varying vec2 v_texcoord;\n"
   "uniform sampler2D tex;\n"
   "uniform float alpha;\n"
   "void main()\n"
   "{\n"
   "   gl_FragColor = texture2D(tex, v_texcoord)\n;"
   "   gl_FragColor = alpha * gl_FragColor;\n"
   "}\n"
};

static const char solid_fragment_shader[] = 
{
   "precision mediump float;\n"
   "uniform vec4 color;\n"
   "void main()\n"
   "{\n"
   "   gl_FragColor = color\n;"
   "}\n"
};

Eina_Bool 
e_mod_comp_wl_comp_init(void)
{
   const char *extensions;
   struct wl_event_loop *loop;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(_wl_comp = malloc(sizeof(Wayland_Compositor))))
     {
        EINA_LOG_ERR("Could not allocate space for compositor\n");
        return EINA_FALSE;
     }

   memset(_wl_comp, 0, sizeof(*_wl_comp));

// open display, get X connection, create cursor

   if (!_e_mod_comp_wl_comp_egl_init())
     {
        EINA_LOG_ERR("Failed to init EGL\n");
        free(_wl_comp);
        return EINA_FALSE;
     }

   _wl_comp->destroy = _e_mod_comp_wl_comp_destroy;

// weston_compositor_init

   if (!wl_display_add_global(_wl_disp, &wl_compositor_interface, _wl_comp, 
                              _e_mod_comp_wl_comp_bind))
     {
        EINA_LOG_ERR("Failed to add compositor to wayland\n");
        free(_wl_comp);
        return EINA_FALSE;
     }

   _wl_comp->shm = wl_shm_init(_wl_disp, &_wl_shm_callbacks);

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

   wl_list_init(&_wl_comp->surfaces);

// spring init
// screenshooter init

   wl_data_device_manager_init(_wl_disp);

   glActiveTexture(GL_TEXTURE0);

   /* init shader */
   if (!_e_mod_comp_wl_comp_shader_init(&_wl_comp->texture_shader, 
                                       vertex_shader, texture_fragment_shader))
     {
        EINA_LOG_ERR("Failed to initialize texture shader\n");
        free(_wl_comp);
        return EINA_FALSE;
     }

   /* init solid shader */
   if (!_e_mod_comp_wl_comp_solid_shader_init(&_wl_comp->solid_shader, 
                                              _wl_comp->texture_shader.vertex_shader, 
                                              solid_fragment_shader))
     {
        EINA_LOG_ERR("Failed to initialize solid shader\n");
        free(_wl_comp);
        return EINA_FALSE;
     }

   loop = wl_display_get_event_loop(_wl_disp);
   _wl_comp->idle_source = 
     wl_event_loop_add_timer(loop, _e_mod_comp_wl_comp_idle_handler, _wl_comp);
   wl_event_source_timer_update(_wl_comp->idle_source, 300 * 1000);

   e_mod_comp_wl_comp_schedule_repaint();

   return EINA_TRUE;
}

Eina_Bool 
e_mod_comp_wl_comp_shutdown(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!_wl_comp) return EINA_TRUE;
   if (_wl_comp->destroy) _wl_comp->destroy();
   return EINA_TRUE;
}

Wayland_Compositor *
e_mod_comp_wl_comp_get(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   return _wl_comp;
}

void 
e_mod_comp_wl_comp_repick(void)
{
   Wayland_Input *device;
   uint32_t timestamp;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   device = e_mod_comp_wl_input_get();
   timestamp = e_mod_comp_wl_time_get();
   e_mod_comp_wl_input_repick(&device->input_device, timestamp);
}

Wayland_Surface *
e_mod_comp_wl_comp_surface_pick(int32_t x, int32_t y, int32_t *sx, int32_t *sy)
{
   Wayland_Surface *surface;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wl_list_for_each(surface, &_wl_comp->surfaces, link)
     {
        if (!surface->surface.resource.client) continue;
        e_mod_comp_wl_surface_transform(surface, x, y, sx, sy);
        if ((0 <= *sx) && (*sx < surface->width) && 
            (0 <= *sy) && (*sy < surface->height))
          return surface;
     }

   return NULL;
}

void 
e_mod_comp_wl_comp_schedule_repaint(void)
{
   Wayland_Output *output;
   struct wl_event_loop *loop;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(output = e_mod_comp_wl_output_get())) return;
   loop = wl_display_get_event_loop(_wl_disp);
   output->repaint_needed = EINA_TRUE;
   if (output->repaint_scheduled) return;
   wl_event_loop_add_idle(loop, e_mod_comp_wl_output_idle_repaint, output);
   output->repaint_scheduled = EINA_TRUE;
}

void 
e_mod_comp_wl_comp_wake(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wl_event_source_timer_update(_wl_comp->idle_source, 
                                300 * 1000);
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

static Eina_Bool 
_e_mod_comp_wl_comp_shader_init(Wayland_Shader *shader, const char *vertex_source, const char *fragment_source)
{
   GLint status;
   char msg[512];

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   shader->vertex_shader = 
     _e_mod_comp_wl_comp_compile_shader(GL_VERTEX_SHADER, vertex_source);
   shader->fragment_shader = 
     _e_mod_comp_wl_comp_compile_shader(GL_FRAGMENT_SHADER, fragment_source);

   shader->program = glCreateProgram();
   glAttachShader(shader->program, shader->vertex_shader);
   glAttachShader(shader->program, shader->fragment_shader);
   glBindAttribLocation(shader->program, 0, "position");
   glBindAttribLocation(shader->program, 1, "texcoord");

   glLinkProgram(shader->program);
   glGetProgramiv(shader->program, GL_LINK_STATUS, &status);
   if (!status) 
     {
        glGetProgramInfoLog(shader->program, sizeof(msg), NULL, msg);
        EINA_LOG_ERR("Link info: %s\n", msg);
        return EINA_FALSE;
     }

   shader->proj_uniform = glGetUniformLocation(shader->program, "proj");
   shader->tex_uniform = glGetUniformLocation(shader->program, "tex");
   shader->alpha_uniform = glGetUniformLocation(shader->program, "alpha");

   return EINA_TRUE;
}

/* static void  */
/* _e_mod_comp_wl_comp_shader_shutdown(void) */
/* { */

/* } */

static Eina_Bool 
_e_mod_comp_wl_comp_solid_shader_init(Wayland_Shader *shader, GLuint vertex_shader, const char *fragment_source)
{
   GLint status;
   char msg[512];

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   shader->vertex_shader = vertex_shader;
   shader->fragment_shader =
     _e_mod_comp_wl_comp_compile_shader(GL_FRAGMENT_SHADER, fragment_source);

   shader->program = glCreateProgram();
   glAttachShader(shader->program, shader->vertex_shader);
   glAttachShader(shader->program, shader->fragment_shader);
   glBindAttribLocation(shader->program, 0, "position");
   glBindAttribLocation(shader->program, 1, "texcoord");

   glLinkProgram(shader->program);
   glGetProgramiv(shader->program, GL_LINK_STATUS, &status);
   if (!status) 
     {
        glGetProgramInfoLog(shader->program, sizeof(msg), NULL, msg);
        EINA_LOG_ERR("Link info: %s\n", msg);
        return EINA_FALSE;
     }
 
   shader->proj_uniform = glGetUniformLocation(shader->program, "proj");
   shader->color_uniform = glGetUniformLocation(shader->program, "color");

   return EINA_TRUE;
}

/* static void  */
/* _e_mod_comp_wl_comp_solid_shader_shutdown() */
/* { */

/* } */

static int 
_e_mod_comp_wl_comp_compile_shader(GLenum type, const char *source)
{
   GLuint s;
   GLint status;
   char msg[512];

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   s = glCreateShader(type);
   glShaderSource(s, 1, &source, NULL);
   glCompileShader(s);
   glGetShaderiv(s, GL_COMPILE_STATUS, &status);
   if (!status) 
     {
        glGetShaderInfoLog(s, sizeof(msg), NULL, msg);
        EINA_LOG_ERR("shader info: %s\n", msg);
        return GL_NONE;
     }

   return s;
}

static void 
_e_mod_comp_wl_comp_destroy(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (_wl_comp->has_bind)
     _wl_comp->unbind_display(_wl_comp->egl.display, _wl_disp);

   if (_wl_comp->idle_source) wl_event_source_remove(_wl_comp->idle_source);

   wl_shm_finish(_wl_comp->shm);

   wl_array_release(&_wl_comp->vertices);
   wl_array_release(&_wl_comp->indices);

   _e_mod_comp_wl_comp_egl_shutdown();

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
   Wayland_Surface *surface;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(surface = e_mod_comp_wl_surface_create(0, 0, 0, 0))) 
     {
        wl_resource_post_no_memory(resource);
        return;
     }

   surface->surface.resource.destroy = e_mod_comp_wl_surface_destroy_surface;
   surface->surface.resource.object.id = id;
   surface->surface.resource.object.interface = &wl_surface_interface;
   surface->surface.resource.object.implementation = 
     (void (**)(void))&_wl_surface_interface;
   surface->surface.resource.data = surface;

   wl_client_add_resource(client, &surface->surface.resource);
}

static int 
_e_mod_comp_wl_comp_idle_handler(void *data)
{
   Wayland_Compositor *wc;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(wc = data)) return 1;

   /* TODO: Check idle inhibit */

   /* TODO: fade */

   return 1;
}
