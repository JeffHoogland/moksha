#include "e.h"
#include "e_mod_main.h"
#include "e_mod_comp.h"
#ifdef HAVE_WAYLAND
# include "e_mod_comp_wl.h"
# include "e_mod_comp_wl_comp.h"
# include "e_mod_comp_wl_output.h"
# include "e_mod_comp_wl_input.h"
# include "e_mod_comp_wl_shell.h"
#endif
#include <assert.h>
#include <wayland-client.h>

/* local function prototypes */
static Eina_Bool _e_mod_comp_wl_fd_handle(void *data, Ecore_Fd_Handler *hdl __UNUSED__);

/* private variables */
static Ecore_Fd_Handler *_wl_fd_handler = NULL;

/* extern variables */
struct wl_display *_wl_disp;

Eina_Bool 
e_mod_comp_wl_init(void)
{
   struct wl_event_loop *loop;
   int fd = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* init wayland display */
   if (!(_wl_disp = wl_display_create()))
     {
        printf("Failed to create wayland display\n");
        EINA_LOG_ERR("Failed to create wayland display\n");
        return EINA_FALSE;
     }

   /* TODO: Add signal handlers ? */

   /* init wayland compositor */
   if (!e_mod_comp_wl_comp_init())
     {
        wl_display_terminate(_wl_disp);
        printf("Failed to initialize compositor\n");
        EINA_LOG_ERR("Failed to initialize compositor\n");
        return EINA_FALSE;
     }

   /* init output */
   if (!e_mod_comp_wl_output_init()) 
     {
        e_mod_comp_wl_comp_shutdown();
        wl_display_terminate(_wl_disp);
        printf("Failed to initialize output\n");
        EINA_LOG_ERR("Failed to initialize output\n");
        return EINA_FALSE;
     }

   /* init input */
   if (!e_mod_comp_wl_input_init())
     {
        e_mod_comp_wl_output_shutdown();
        e_mod_comp_wl_comp_shutdown();
        wl_display_terminate(_wl_disp);
        printf("Failed to initialize input\n");
        EINA_LOG_ERR("Failed to initialize input\n");
        return EINA_FALSE;
     }

// NB: x11 compositor adds X event source to wayland event loop

   /* init shell */
   if (!e_mod_comp_wl_shell_init())
     {
        e_mod_comp_wl_input_shutdown();
        e_mod_comp_wl_output_shutdown();
        e_mod_comp_wl_comp_shutdown();
        wl_display_terminate(_wl_disp);
        printf("Failed to initialize shell\n");
        EINA_LOG_ERR("Failed to initialize shell\n");
        return EINA_FALSE;
     }

   if (wl_display_add_socket(_wl_disp, NULL))
     {
        e_mod_comp_wl_shell_shutdown();
        e_mod_comp_wl_input_shutdown();
        e_mod_comp_wl_output_shutdown();
        e_mod_comp_wl_comp_shutdown();
        wl_display_terminate(_wl_disp);
        printf("Failed to add display socket\n");
        EINA_LOG_ERR("Failed to add display socket\n");
        return EINA_FALSE;
     }

   loop = wl_display_get_event_loop(_wl_disp);
   fd = wl_event_loop_get_fd(loop);

   _wl_fd_handler = 
     ecore_main_fd_handler_add(fd, ECORE_FD_READ, 
                               _e_mod_comp_wl_fd_handle, _wl_disp, NULL, NULL);

   e_mod_comp_wl_comp_wake();

   wl_event_loop_dispatch(loop, 0);

   return EINA_TRUE;
}

void 
e_mod_comp_wl_shutdown(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   e_mod_comp_wl_shell_shutdown();
   e_mod_comp_wl_input_shutdown();
   e_mod_comp_wl_output_shutdown();
   e_mod_comp_wl_comp_shutdown();
   wl_display_terminate(_wl_disp);
}

uint32_t 
e_mod_comp_wl_time_get(void)
{
   struct timeval tv;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   gettimeofday(&tv, NULL);
   return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

void 
e_mod_comp_wl_matrix_init(Wayland_Matrix *matrix)
{
   static const Wayland_Matrix identity = 
     {
        { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  0, 0, 0, 1 }
     };

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   memcpy(matrix, &identity, sizeof(identity));
}

void 
e_mod_comp_wl_matrix_translate(Wayland_Matrix *matrix, GLfloat x, GLfloat y, GLfloat z)
{
   Wayland_Matrix translate = 
     {
        { 1, 0, 0, 0,  0, 1, 0, 0,  0, 0, 1, 0,  x, y, z, 1 }
     };

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   e_mod_comp_wl_matrix_multiply(matrix, &translate);
}

void 
e_mod_comp_wl_matrix_scale(Wayland_Matrix *matrix, GLfloat x, GLfloat y, GLfloat z)
{
   Wayland_Matrix scale = 
     {
        { x, 0, 0, 0,  0, y, 0, 0,  0, 0, z, 0,  0, 0, 0, 1 }
     };

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   e_mod_comp_wl_matrix_multiply(matrix, &scale);
}

void 
e_mod_comp_wl_matrix_multiply(Wayland_Matrix *m, const Wayland_Matrix *n)
{
   Wayland_Matrix tmp;
   const GLfloat *row, *column;
   div_t d;
   int i, j;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   for (i = 0; i < 16; i++) {
      tmp.d[i] = 0;
      d = div(i, 4);
      row = m->d + d.quot * 4;
      column = n->d + d.rem;
      for (j = 0; j < 4; j++)
        tmp.d[i] += row[j] * column[j * 4];
   }
   memcpy(m, &tmp, sizeof tmp);
}

void 
e_mod_comp_wl_matrix_transform(Wayland_Matrix *matrix, Wayland_Vector *v)
{
   int i, j;
   Wayland_Vector t;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   for (i = 0; i < 4; i++) 
     {
        t.f[i] = 0;
        for (j = 0; j < 4; j++)
          t.f[i] += v->f[j] * matrix->d[i + j * 4];
     }

   *v = t;
}

void 
e_mod_comp_wl_buffer_attach(struct wl_buffer *buffer, struct wl_surface *surface)
{
   Wayland_Surface *ws;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ws = (Wayland_Surface *)surface;

   if (ws->saved_texture != 0) 
     ws->texture = ws->saved_texture;

   glBindTexture(GL_TEXTURE_2D, ws->texture);

   if (wl_buffer_is_shm(buffer))
     {
        struct wl_list *attached;

        ws->pitch = wl_shm_buffer_get_stride(buffer) / 4;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, ws->pitch, buffer->height, 
                     0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, 
                     wl_shm_buffer_get_data(buffer));

        switch (wl_shm_buffer_get_format(buffer))
          {
           case WL_SHM_FORMAT_ARGB8888:
             ws->visual = WAYLAND_ARGB_VISUAL;
             break;
           case WL_SHM_FORMAT_XRGB8888:
             ws->visual = WAYLAND_RGB_VISUAL;
             break;
          }

        attached = buffer->user_data;
        wl_list_remove(&ws->buffer_link);
        wl_list_insert(attached, &ws->buffer_link);
     }
   else
     {
        Wayland_Compositor *comp;

        comp = e_mod_comp_wl_comp_get();
        if (ws->image != EGL_NO_IMAGE_KHR)
          comp->destroy_image(comp->egl.display, ws->image);
        ws->image = comp->create_image(comp->egl.display, NULL, 
                                       EGL_WAYLAND_BUFFER_WL, buffer, NULL);
        comp->image_target_texture_2d(GL_TEXTURE_2D, ws->image);
        ws->visual = WAYLAND_ARGB_VISUAL;
        ws->pitch = ws->width;
     }
}

void 
e_mod_comp_wl_buffer_post_release(struct wl_buffer *buffer)
{
   if (--buffer->busy_count > 0) return;
   assert(buffer->resource.client != NULL);
   wl_resource_queue_event(&buffer->resource, WL_BUFFER_RELEASE);
}

/* local functions */
static Eina_Bool 
_e_mod_comp_wl_fd_handle(void *data, Ecore_Fd_Handler *hdl __UNUSED__)
{
   struct wl_display *disp;
   struct wl_event_loop *loop;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(disp = data)) return ECORE_CALLBACK_RENEW;
   if (disp != _wl_disp) return ECORE_CALLBACK_RENEW;

   loop = wl_display_get_event_loop(disp);
   wl_event_loop_dispatch(loop, 0);

   return ECORE_CALLBACK_RENEW;
}
