#include "e.h"
#include "e_mod_main.h"
#ifdef HAVE_WAYLAND
# include "e_mod_comp_wl.h"
# include "e_mod_comp_wl_comp.h"
# include "e_mod_comp_wl_output.h"
# include "e_mod_comp_wl_surface.h"
#endif

/* local function prototypes */
static void _e_mod_comp_wl_output_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id);
static void _e_mod_comp_wl_output_scanout_buffer_destroy(struct wl_listener *listener, struct wl_resource *resource __UNUSED__, uint32_t timestamp __UNUSED__);
static void _e_mod_comp_wl_output_pending_scanout_buffer_destroy(struct wl_listener *listener, struct wl_resource *resource __UNUSED__, uint32_t timestamp __UNUSED__);
static int _e_mod_comp_wl_output_finish_frame_handler(void *data);
static void _e_mod_comp_wl_output_finish_frame(Wayland_Output *output, int secs);
static void _e_mod_comp_wl_output_repaint(Wayland_Output *output, int secs);
static void _e_mod_comp_wl_output_schedule_repaint(void);
static Eina_Bool _e_mod_comp_wl_output_setup_scanout_surface(Wayland_Output *output, Wayland_Surface *ws);
static int _e_mod_comp_wl_output_prepare_render(Wayland_Output *output);
static int _e_mod_comp_wl_output_present(Wayland_Output *output);
static int _e_mod_comp_wl_output_prepare_scanout_surface(Wayland_Output *output __UNUSED__, Wayland_Surface *surface __UNUSED__);
static void _e_mod_comp_wl_output_move(Wayland_Output *output, int32_t x, int32_t y);

/* private variables */
static Wayland_Output *_wl_output;

Eina_Bool 
e_mod_comp_wl_output_init(void)
{
   Wayland_Compositor *comp;
   struct wl_event_loop *loop;
   Ecore_X_Window *roots;
   int num = 0, rw, rh;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   roots = ecore_x_window_root_list(&num);
   if ((!roots) || (num <= 0)) 
     {
        EINA_LOG_ERR("Could not get root window list\n");
        return EINA_FALSE;
     }
   ecore_x_window_size_get(roots[0], &rw, &rh);

   if (!(_wl_output = malloc(sizeof(Wayland_Output))))
     {
        EINA_LOG_ERR("Could not allocate space for output\n");
        return EINA_FALSE;
     }

   memset(_wl_output, 0, sizeof(*_wl_output));

   _wl_output->mode.flags = 
     (WL_OUTPUT_MODE_CURRENT | WL_OUTPUT_MODE_PREFERRED);

   _wl_output->x = 0;
   _wl_output->y = 0;
   _wl_output->width = rw;
   _wl_output->height = rh;

   _wl_output->mode.width = rw;
   _wl_output->mode.height = rh;
   _wl_output->mode.refresh = 60;

   _e_mod_comp_wl_output_move(_wl_output, 0, 0);

   _wl_output->scanout_buffer_destroy_listener.func = 
     _e_mod_comp_wl_output_scanout_buffer_destroy;
   _wl_output->pending_scanout_buffer_destroy_listener.func = 
     _e_mod_comp_wl_output_pending_scanout_buffer_destroy;

   wl_list_init(&_wl_output->link);
   wl_list_init(&_wl_output->frame_callbacks);

   if (!wl_display_add_global(_wl_disp, &wl_output_interface, _wl_output, 
                              _e_mod_comp_wl_output_bind))
     {
        EINA_LOG_ERR("Failed to add wayland output\n");
        free(_wl_output);
        return EINA_FALSE;
     }

   comp = e_mod_comp_wl_comp_get();

   _wl_output->egl_surface = 
     eglCreateWindowSurface(comp->egl.display, comp->egl.config, 
                            roots[0], NULL);
   free(roots);

   if (!_wl_output->egl_surface)
     {
        EINA_LOG_ERR("Failed to create EGL Surface\n");
        free(_wl_output);
        return EINA_FALSE;
     }
   if (!eglMakeCurrent(comp->egl.display, _wl_output->egl_surface, 
                       _wl_output->egl_surface, comp->egl.context))
     {
        EINA_LOG_ERR("Failed to make current\n");
        free(_wl_output);
        return EINA_FALSE;
     }

   loop = wl_display_get_event_loop(_wl_disp);
   _wl_output->finish_frame_timer = 
     wl_event_loop_add_timer(loop, _e_mod_comp_wl_output_finish_frame_handler, 
                             _wl_output);

   _wl_output->prepare_render = _e_mod_comp_wl_output_prepare_render;
   _wl_output->present = _e_mod_comp_wl_output_present;
   _wl_output->prepare_scanout_surface = 
     _e_mod_comp_wl_output_prepare_scanout_surface;

   return EINA_TRUE;
}

Eina_Bool 
e_mod_comp_wl_output_shutdown(void)
{
   Wayland_Compositor *comp;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!_wl_output) return EINA_TRUE;

   comp = e_mod_comp_wl_comp_get();

   wl_list_remove(&_wl_output->frame_callbacks);
   wl_list_remove(&_wl_output->link);

   if (_wl_output->finish_frame_timer) 
     wl_event_source_remove(_wl_output->finish_frame_timer);

   eglDestroySurface(comp->egl.display, _wl_output->egl_surface);

   pixman_region32_fini(&_wl_output->region);
   pixman_region32_fini(&_wl_output->prev_damage);

   free(_wl_output);

   return EINA_TRUE;
}

void 
e_mod_comp_wl_output_damage(Wayland_Output *output)
{
   Wayland_Compositor *comp;
   Wayland_Surface *ws;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   comp = e_mod_comp_wl_comp_get();
   if (wl_list_empty(&comp->surfaces)) return;
   ws = container_of(comp->surfaces.next, Wayland_Surface, link);
   pixman_region32_union(&ws->damage, &ws->damage, &output->region);
   e_mod_comp_wl_comp_schedule_repaint();
}

Wayland_Output *
e_mod_comp_wl_output_get(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   return _wl_output;
}

void 
e_mod_comp_wl_output_idle_repaint(void *data)
{
   Wayland_Output *output;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(output = data)) return;
   if (output->repaint_needed) 
     {
        struct timeval tv;
        uint32_t timestamp;

        gettimeofday(&tv, NULL);
        timestamp = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        _e_mod_comp_wl_output_repaint(output, timestamp);
     }
}

/* local functions */
static void 
_e_mod_comp_wl_output_bind(struct wl_client *client, void *data, uint32_t version __UNUSED__, uint32_t id)
{
   Wayland_Output *output;
   struct wl_resource *resource;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(output = data)) return;

   resource = 
     wl_client_add_object(client, &wl_output_interface, NULL, id, output);

   wl_resource_post_event(resource, WL_OUTPUT_GEOMETRY, output->x, output->y, 
                          output->width, output->height, output->subpixel, 
                          output->make, output->model);

   wl_resource_post_event(resource, WL_OUTPUT_MODE, output->mode.flags, 
                          output->mode.width, output->mode.height, 
                          output->mode.refresh);
}

static void 
_e_mod_comp_wl_output_scanout_buffer_destroy(struct wl_listener *listener, struct wl_resource *resource __UNUSED__, uint32_t timestamp __UNUSED__)
{
   Wayland_Output *output;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   output = 
     container_of(listener, Wayland_Output, scanout_buffer_destroy_listener);

   output->scanout_buffer = NULL;

   if (!output->pending_scanout_buffer)
     _e_mod_comp_wl_output_schedule_repaint();
}

static void 
_e_mod_comp_wl_output_pending_scanout_buffer_destroy(struct wl_listener *listener, struct wl_resource *resource __UNUSED__, uint32_t timestamp __UNUSED__)
{
   Wayland_Output *output;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   output = 
     container_of(listener, Wayland_Output, 
                  pending_scanout_buffer_destroy_listener);

   output->pending_scanout_buffer = NULL;

   _e_mod_comp_wl_output_schedule_repaint();
}

static int 
_e_mod_comp_wl_output_finish_frame_handler(void *data)
{
   Wayland_Output *output;
   uint32_t msec;
   struct timeval tv;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(output = data)) return 1;

   gettimeofday(&tv, NULL);
   msec = tv.tv_sec * 1000 + tv.tv_usec / 1000;
   _e_mod_comp_wl_output_finish_frame(output, msec);

   return 1;
}

static void 
_e_mod_comp_wl_output_finish_frame(Wayland_Output *output, int secs)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!output) return;
   if (output->scanout_buffer)
     {
        e_mod_comp_wl_buffer_post_release(output->scanout_buffer);
        wl_list_remove(&output->scanout_buffer_destroy_listener.link);
        output->scanout_buffer = NULL;
     }
   if (output->pending_scanout_buffer)
     {
        output->scanout_buffer = output->pending_scanout_buffer;
        wl_list_remove(&output->pending_scanout_buffer_destroy_listener.link);
        wl_list_insert(output->scanout_buffer->resource.destroy_listener_list.prev, 
                       &output->scanout_buffer_destroy_listener.link);
        output->pending_scanout_buffer = NULL;
     }
   if (output->repaint_needed)
     _e_mod_comp_wl_output_repaint(output, secs);
   else
     output->repaint_scheduled = EINA_FALSE;
}

static void 
_e_mod_comp_wl_output_repaint(Wayland_Output *output, int secs)
{
   Wayland_Frame_Callback *cb, *cnext;
   Wayland_Compositor *comp;
   Wayland_Surface *ws;
   pixman_region32_t opaque, new_damage, total_damage, repaint;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!output) return;

   comp = e_mod_comp_wl_comp_get();

   /* start weston output repaint */
   output->prepare_render(output);
   glViewport(0, 0, output->width, output->height);
   glUseProgram(comp->texture_shader.program);
   glUniformMatrix4fv(comp->texture_shader.proj_uniform, 1, GL_FALSE, 
                      output->matrix.d);
   glUniform1i(comp->texture_shader.tex_uniform, 0);

   /* TODO: Set cursor */

   pixman_region32_init(&new_damage);
   pixman_region32_init(&opaque);

   wl_list_for_each(ws, &comp->surfaces, link)
     {
        pixman_region32_subtract(&ws->damage, &ws->damage, &opaque);
        pixman_region32_union(&new_damage, &new_damage, &ws->damage);
        pixman_region32_union(&opaque, &opaque, &ws->opaque);
     }

   pixman_region32_init(&total_damage);
   pixman_region32_union(&total_damage, &new_damage, &output->prev_damage);
   pixman_region32_intersect(&output->prev_damage, &new_damage, &output->region);

   pixman_region32_fini(&opaque);
   pixman_region32_fini(&new_damage);

   ws = container_of(comp->surfaces.next, Wayland_Surface, link);

   if (_e_mod_comp_wl_output_setup_scanout_surface(output, ws))
     return;

   /* TODO: Handle Fullscreen */
     {
        wl_list_for_each(ws, &comp->surfaces, link)
          {
             pixman_region32_copy(&ws->damage, &total_damage);
             pixman_region32_subtract(&total_damage, &total_damage, &ws->opaque);
          }

        wl_list_for_each_reverse(ws, &comp->surfaces, link)
          {
             pixman_region32_init(&repaint);
             pixman_region32_intersect(&repaint, &output->region, &ws->damage);
             e_mod_comp_wl_surface_draw(ws, output, &repaint);
             pixman_region32_subtract(&ws->damage, &ws->damage, &output->region);
             pixman_region32_fini(&repaint);
          }
     }

   /* TODO: Fade out */

   pixman_region32_fini(&total_damage);

   /* end weston output repaint */

   output->repaint_needed = EINA_FALSE;
   output->repaint_scheduled = EINA_TRUE;
   output->present(output);

   wl_list_for_each_safe(cb, cnext, &output->frame_callbacks, link)
     {
        wl_resource_post_event(&cb->resource, WL_CALLBACK_DONE, secs);
        wl_resource_destroy(&cb->resource, 0);
     }

   /* TODO: Handle animations */
}

static void 
_e_mod_comp_wl_output_schedule_repaint(void)
{
   struct wl_event_loop *loop;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   loop = wl_display_get_event_loop(_wl_disp);
   _wl_output->repaint_needed = EINA_TRUE;
   if (_wl_output->repaint_scheduled) return;
   wl_event_loop_add_idle(loop, e_mod_comp_wl_output_idle_repaint, _wl_output);
   _wl_output->repaint_scheduled = EINA_TRUE;
}

static Eina_Bool 
_e_mod_comp_wl_output_setup_scanout_surface(Wayland_Output *output, Wayland_Surface *ws)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if ((!output) || (!ws)) return EINA_FALSE;

   if ((ws->visual != WAYLAND_RGB_VISUAL) || 
       (!output->prepare_scanout_surface(output, ws)))
     return EINA_FALSE;

   output->pending_scanout_buffer = ws->buffer;
   output->pending_scanout_buffer->busy_count++;

   wl_list_insert(output->pending_scanout_buffer->resource.destroy_listener_list.prev, 
                  &output->pending_scanout_buffer_destroy_listener.link);

   return EINA_TRUE;
}

static int 
_e_mod_comp_wl_output_prepare_render(Wayland_Output *output)
{
   Wayland_Compositor *comp;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!output) return -1;
   comp = e_mod_comp_wl_comp_get();

   if (!eglMakeCurrent(comp->egl.display, output->egl_surface, 
                       output->egl_surface, comp->egl.context))
     {
        EINA_LOG_ERR("Failed to make current\n");
        return -1;
     }

   return 0;
}

static int 
_e_mod_comp_wl_output_present(Wayland_Output *output)
{
   Wayland_Compositor *comp;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!output) return -1;

   comp = e_mod_comp_wl_comp_get();

   if (_e_mod_comp_wl_output_prepare_render(output)) return -1;

   eglSwapBuffers(comp->egl.display, output->egl_surface);

   wl_event_source_timer_update(output->finish_frame_timer, 10);

   return 0;
}

static int 
_e_mod_comp_wl_output_prepare_scanout_surface(Wayland_Output *output __UNUSED__, Wayland_Surface *surface __UNUSED__)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   return -1;
}

static void 
_e_mod_comp_wl_output_move(Wayland_Output *output, int32_t x, int32_t y)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!output) return;

   pixman_region32_init(&output->prev_damage);
   pixman_region32_init_rect(&_wl_output->region, x, y, 
                             output->width, output->height);

   e_mod_comp_wl_matrix_init(&output->matrix);
   e_mod_comp_wl_matrix_translate(&output->matrix, 
                                          -output->x - output->width / 2.0, 
                                          -output->y - output->height / 2.0, 0);
   e_mod_comp_wl_matrix_scale(&output->matrix, 2.0 / output->width, 
                                      -1 * 2.0 / output->height, 1);
   e_mod_comp_wl_output_damage(output);
}
