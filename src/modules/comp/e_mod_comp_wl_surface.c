#include "e.h"
#include "e_mod_main.h"
#ifdef HAVE_WAYLAND
# include "e_mod_comp_wl.h"
# include "e_mod_comp_wl_comp.h"
# include "e_mod_comp_wl_output.h"
# include "e_mod_comp_wl_surface.h"
# include "e_mod_comp_wl_shell.h"
#endif

/* local function prototypes */
static void _e_mod_comp_wl_surface_buffer_destroy_handle(struct wl_listener *listener, struct wl_resource *resource __UNUSED__, uint32_t timestamp __UNUSED__);
static int _e_mod_comp_wl_surface_texture_region(Wayland_Surface *ws, pixman_region32_t *region);
static int _e_mod_comp_wl_surface_texture_transformed_surface(Wayland_Surface *ws);
static void _e_mod_comp_wl_surface_transform_vertex(Wayland_Surface *ws, GLfloat x, GLfloat y, GLfloat u, GLfloat v, GLfloat *r);
static void _e_mod_comp_wl_surface_frame_destroy_callback(struct wl_resource *resource);
static void _e_mod_comp_wl_surface_damage_rectangle(Wayland_Surface *ws, int32_t x, int32_t y, int32_t width, int32_t height);
static void _e_mod_comp_wl_surface_flush_damage(Wayland_Surface *ws);
static void _e_mod_comp_wl_surface_raise(Wayland_Surface *ws);

Wayland_Surface *
e_mod_comp_wl_surface_create(int32_t x, int32_t y, int32_t width, int32_t height)
{
   Wayland_Surface *surface;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(surface = calloc(1, sizeof(Wayland_Surface)))) return NULL;

   wl_list_init(&surface->link);
   wl_list_init(&surface->buffer_link);

   glGenTextures(1, &surface->texture);
   glBindTexture(GL_TEXTURE_2D, surface->texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   surface->surface.resource.client = NULL;

   surface->visual = WAYLAND_NONE_VISUAL;
   surface->saved_texture = 0;
   surface->x = x;
   surface->y = y;
   surface->width = width;
   surface->height = height;
   surface->buffer = NULL;

   pixman_region32_init(&surface->damage);
   pixman_region32_init(&surface->opaque);

   wl_list_init(&surface->frame_callbacks);

   surface->buffer_destroy_listener.func = 
     _e_mod_comp_wl_surface_buffer_destroy_handle;

   return surface;
}

void 
e_mod_comp_wl_surface_destroy(struct wl_client *client __UNUSED__, struct wl_resource *resource)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wl_resource_destroy(resource, e_mod_comp_wl_time_get());
}

void 
e_mod_comp_wl_surface_attach(struct wl_client *client __UNUSED__, struct wl_resource *resource, struct wl_resource *buffer_resource, int32_t x, int32_t y)
{
   Wayland_Surface *ws;
   Wayland_Shell *shell;
   struct wl_buffer *buffer;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ws = resource->data;
   buffer = buffer_resource->data;
   shell = e_mod_comp_wl_shell_get();

   e_mod_comp_wl_surface_damage_below(ws);

   if (ws->buffer)
     {
        e_mod_comp_wl_buffer_post_release(ws->buffer);
        wl_list_remove(&ws->buffer_destroy_listener.link);
     }

   buffer->busy_count++;
   ws->buffer = buffer;
   wl_list_insert(ws->buffer->resource.destroy_listener_list.prev, 
                  &ws->buffer_destroy_listener.link);

   if (!ws->visual) 
     shell->map(shell, ws, buffer->width, buffer->height);
   else if ((x != 0) || (y != 0) || (ws->width != buffer->width) || 
            (ws->height != buffer->height))
     shell->configure(shell, ws, ws->x + x, ws->y + y, 
                      buffer->width, buffer->height);

   e_mod_comp_wl_buffer_attach(buffer, &ws->surface);
}

void 
e_mod_comp_wl_surface_damage(struct wl_client *client __UNUSED__, struct wl_resource *resource, int32_t x, int32_t y, int32_t width, int32_t height)
{
   Wayland_Surface *ws;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ws = resource->data;
   _e_mod_comp_wl_surface_damage_rectangle(ws, x, y, width, height);
}

void 
e_mod_comp_wl_surface_frame(struct wl_client *client, struct wl_resource *resource, uint32_t callback)
{
   Wayland_Surface *ws;
   Wayland_Frame_Callback *cb;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ws = resource->data;

   if (!(cb = malloc(sizeof(*cb))))
     {
        wl_resource_post_no_memory(resource);
        return;
     }

   cb->resource.object.interface = &wl_callback_interface;
   cb->resource.object.id = callback;
   cb->resource.destroy = _e_mod_comp_wl_surface_frame_destroy_callback;
   cb->resource.client = client;
   cb->resource.data = cb;

   wl_client_add_resource(client, &cb->resource);

   wl_list_insert(ws->frame_callbacks.prev, &cb->link);
}

void 
e_mod_comp_wl_surface_destroy_surface(struct wl_resource *resource)
{
   Wayland_Surface *ws;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ws = container_of(resource, Wayland_Surface, surface.resource);

   e_mod_comp_wl_surface_damage_below(ws);

   _e_mod_comp_wl_surface_flush_damage(ws);

   wl_list_remove(&ws->link);

   e_mod_comp_wl_comp_repick();

   if (!ws->saved_texture)
     glDeleteTextures(1, &ws->texture);
   else
     glDeleteTextures(1, &ws->saved_texture);

   if (ws->buffer)
     wl_list_remove(&ws->buffer_destroy_listener.link);

   wl_list_remove(&ws->buffer_link);

   pixman_region32_fini(&ws->damage);
   pixman_region32_fini(&ws->opaque);

   free(ws);
}

void 
e_mod_comp_wl_surface_draw(Wayland_Surface *ws, Wayland_Output *output __UNUSED__, pixman_region32_t *clip)
{
   Wayland_Compositor *comp;
   GLfloat *v;
   pixman_region32_t repaint;
   GLint filter;
   int n = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   comp = e_mod_comp_wl_comp_get();
   pixman_region32_init_rect(&repaint, ws->x, ws->y, ws->width, ws->height);
   pixman_region32_intersect(&repaint, &repaint, clip);
   if (!pixman_region32_not_empty(&repaint)) return;
   switch (ws->visual)
     {
      case WAYLAND_ARGB_VISUAL:
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        break;
      case WAYLAND_RGB_VISUAL:
        glDisable(GL_BLEND);
        break;
      default:
        break;
     }

   if (ws->alpha != comp->current_alpha)
     {
        glUniform1f(comp->texture_shader.alpha_uniform, ws->alpha / 255.0);
        comp->current_alpha = ws->alpha;
     }

   if (ws->transform == NULL)
     {
        filter = GL_NEAREST;
        n = _e_mod_comp_wl_surface_texture_region(ws, &repaint);
     }
   else
     {
        filter = GL_LINEAR;
        n = _e_mod_comp_wl_surface_texture_transformed_surface(ws);
     }

   glBindTexture(GL_TEXTURE_2D, ws->texture);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

   v = comp->vertices.data;
   glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(*v), &v[0]);
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(*v), &v[2]);
   glEnableVertexAttribArray(0);
   glEnableVertexAttribArray(1);
   glDrawElements(GL_TRIANGLES, n * 6, GL_UNSIGNED_INT, comp->indices.data);

   comp->vertices.size = 0;
   comp->indices.size = 0;
   pixman_region32_fini(&repaint);
}

void 
e_mod_comp_wl_surface_damage_surface(Wayland_Surface *ws)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   _e_mod_comp_wl_surface_damage_rectangle(ws, 0, 0, ws->width, ws->height);
}

void 
e_mod_comp_wl_surface_transform(Wayland_Surface *ws, int32_t x, int32_t y, int32_t *sx, int32_t *sy)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   *sx = x - ws->x;
   *sy = y - ws->y;
}

void 
e_mod_comp_wl_surface_configure(Wayland_Surface *ws, int32_t x, int32_t y, int32_t width, int32_t height)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   e_mod_comp_wl_surface_damage_below(ws);

   ws->x = x;
   ws->y = y;
   ws->width = width;
   ws->height = height;

   e_mod_comp_wl_surface_assign_output(ws);
   e_mod_comp_wl_surface_damage_surface(ws);

   pixman_region32_fini(&ws->opaque);
   if (ws->visual == WAYLAND_RGB_VISUAL) 
     pixman_region32_init_rect(&ws->opaque, ws->x, ws->y, ws->width, ws->height);
   else
     pixman_region32_init(&ws->opaque);
}

void 
e_mod_comp_wl_surface_assign_output(Wayland_Surface *ws)
{
   Wayland_Output *output;
   pixman_region32_t region;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   output = e_mod_comp_wl_output_get();
   pixman_region32_init_rect(&region, ws->x, ws->y, ws->width, ws->height);
   pixman_region32_intersect(&region, &region, &output->region);

   /* NB: maybe region32_extents if we handle more than one output */

   if (!wl_list_empty(&ws->frame_callbacks))
     {
        wl_list_insert_list(output->frame_callbacks.prev, 
                            &ws->frame_callbacks);
        wl_list_init(&ws->frame_callbacks);
     }
}

void 
e_mod_comp_wl_surface_damage_below(Wayland_Surface *ws)
{
   Wayland_Compositor *comp;
   Wayland_Surface *below;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   comp = e_mod_comp_wl_comp_get();
   if (ws->link.next == &comp->surfaces) return;
   below = container_of(ws->link.next, Wayland_Surface, link);
   pixman_region32_union_rect(&below->damage, &below->damage, 
                              ws->x, ws->y, ws->width, ws->height);
   e_mod_comp_wl_comp_schedule_repaint();
}

void 
e_mod_comp_wl_surface_activate(Wayland_Surface *ws, Wayland_Input *device, uint32_t timestamp)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   _e_mod_comp_wl_surface_raise(ws);
   wl_input_device_set_keyboard_focus(&device->input_device, &ws->surface, timestamp);
   wl_data_device_set_keyboard_focus(&device->input_device);
}

Eina_Bool 
e_mod_comp_wl_surface_move(Wayland_Surface *ws, Wayland_Input *wi, uint32_t timestamp)
{
   /* TODO: Code me */

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   wl_input_device_set_pointer_focus(&wi->input_device, NULL, timestamp, 
                                     0, 0, 0, 0);
   return EINA_TRUE;
}

Eina_Bool 
e_mod_comp_wl_surface_resize(Wayland_Shell_Surface *wss, Wayland_Input *wi, uint32_t timestamp, uint32_t edges)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* TODO: Code me */

   wl_input_device_set_pointer_focus(&wi->input_device, NULL, timestamp, 
                                     0, 0, 0, 0);
   return EINA_TRUE;
}

/* local functions */
static void 
_e_mod_comp_wl_surface_buffer_destroy_handle(struct wl_listener *listener, struct wl_resource *resource __UNUSED__, uint32_t timestamp __UNUSED__)
{
   Wayland_Surface *ws;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ws = container_of(listener, Wayland_Surface, buffer_destroy_listener);
   ws->buffer = NULL;
}

static int 
_e_mod_comp_wl_surface_texture_region(Wayland_Surface *ws, pixman_region32_t *region)
{
   Wayland_Compositor *comp;
   GLfloat *v, inv_width, inv_height;
   pixman_box32_t *rects;
   unsigned int *p;
   int i, n;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   comp = e_mod_comp_wl_comp_get();
   rects = pixman_region32_rectangles(region, &n);
   v = wl_array_add(&comp->vertices, n * 16 * sizeof *v);
   p = wl_array_add(&comp->indices, n * 6 * sizeof *p);
   inv_width = 1.0 / ws->pitch;
   inv_height = 1.0 / ws->height;

   for (i = 0; i < n; i++, v += 16, p += 6) {
      v[ 0] = rects[i].x1;
      v[ 1] = rects[i].y1;
      v[ 2] = (GLfloat) (rects[i].x1 - ws->x) * inv_width;
      v[ 3] = (GLfloat) (rects[i].y1 - ws->y) * inv_height;

      v[ 4] = rects[i].x1;
      v[ 5] = rects[i].y2;
      v[ 6] = v[ 2];
      v[ 7] = (GLfloat) (rects[i].y2 - ws->y) * inv_height;

      v[ 8] = rects[i].x2;
      v[ 9] = rects[i].y1;
      v[10] = (GLfloat) (rects[i].x2 - ws->x) * inv_width;
      v[11] = v[ 3];

      v[12] = rects[i].x2;
      v[13] = rects[i].y2;
      v[14] = v[10];
      v[15] = v[ 7];

      p[0] = i * 4 + 0;
      p[1] = i * 4 + 1;
      p[2] = i * 4 + 2;
      p[3] = i * 4 + 2;
      p[4] = i * 4 + 1;
      p[5] = i * 4 + 3;
   }

   return n;
}

static int 
_e_mod_comp_wl_surface_texture_transformed_surface(Wayland_Surface *ws)
{
   Wayland_Compositor *comp;
   GLfloat *v;
   unsigned int *p;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   comp = e_mod_comp_wl_comp_get();
   v = wl_array_add(&comp->vertices, 16 * sizeof *v);
   p = wl_array_add(&comp->indices, 6 * sizeof *p);

   _e_mod_comp_wl_surface_transform_vertex(ws, ws->x, ws->y, 0.0, 0.0, &v[0]);
   _e_mod_comp_wl_surface_transform_vertex(ws, ws->x, ws->y + ws->height, 
                                           0.0, 1.0, &v[4]);
   _e_mod_comp_wl_surface_transform_vertex(ws, ws->x + ws->width, ws->y, 
                                           1.0, 0.0, &v[8]);
   _e_mod_comp_wl_surface_transform_vertex(ws, ws->x + ws->width, 
                                           ws->y + ws->height, 1.0, 1.0, 
                                           &v[12]);

   p[0] = 0;
   p[1] = 1;
   p[2] = 2;
   p[3] = 2;
   p[4] = 1;
   p[5] = 3;

   return 1;
}

static void 
_e_mod_comp_wl_surface_transform_vertex(Wayland_Surface *ws, GLfloat x, GLfloat y, GLfloat u, GLfloat v, GLfloat *r)
{
   Wayland_Vector t;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   t.f[0] = x;
   t.f[1] = y;
   t.f[2] = 0.0;
   t.f[3] = 1.0;

   e_mod_comp_wl_matrix_transform(&ws->transform->matrix, &t);

   r[ 0] = t.f[0];
   r[ 1] = t.f[1];
   r[ 2] = u;
   r[ 3] = v;
}

static void 
_e_mod_comp_wl_surface_frame_destroy_callback(struct wl_resource *resource)
{
   Wayland_Frame_Callback *cb;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   cb = resource->data;
   wl_list_remove(&cb->link);
   free(cb);
}

static void 
_e_mod_comp_wl_surface_damage_rectangle(Wayland_Surface *ws, int32_t x, int32_t y, int32_t width, int32_t height)
{
   Wayland_Compositor *comp;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   comp = e_mod_comp_wl_comp_get();
   pixman_region32_union_rect(&ws->damage, &ws->damage, 
                              ws->x + x, ws->y + y, width, height);
   e_mod_comp_wl_comp_schedule_repaint();
}

static void 
_e_mod_comp_wl_surface_flush_damage(Wayland_Surface *ws)
{
   Wayland_Compositor *comp;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   comp = e_mod_comp_wl_comp_get();
   if (ws->link.next != &comp->surfaces)
     {
        Wayland_Surface *below;

        below = container_of(ws->link.next, Wayland_Surface, link);
        pixman_region32_union(&below->damage, &below->damage, &ws->damage);
     }
}

static void 
_e_mod_comp_wl_surface_raise(Wayland_Surface *ws)
{
   Wayland_Compositor *comp;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   comp = e_mod_comp_wl_comp_get();
   wl_list_remove(&ws->link);
   wl_list_insert(&comp->surfaces, &ws->link);
   e_mod_comp_wl_comp_repick();
   e_mod_comp_wl_surface_damage_surface(ws);
}
