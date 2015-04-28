#include "e.h"
#include "e_mod_main.h"
#include "e_mod_comp.h"
#ifdef HAVE_WAYLAND_CLIENTS
# include "e_mod_comp_wl.h"
# include "e_mod_comp_wl_buffer.h"
# include "e_mod_comp_wl_comp.h"
#endif

void
e_mod_comp_wl_buffer_post_release(struct wl_buffer *buffer)
{
   if (--buffer->busy_count > 0) return;
   if (buffer->resource.client)
     wl_resource_queue_event(&buffer->resource, WL_BUFFER_RELEASE);
}

void
e_mod_comp_wl_buffer_attach(struct wl_buffer *buffer, struct wl_surface *surface)
{
   Wayland_Surface *ws;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ws = (Wayland_Surface *)surface;

   if (!ws->texture)
     {
        glGenTextures(1, &ws->texture);
        glBindTexture(GL_TEXTURE_2D, ws->texture);
        glTexParameteri(GL_TEXTURE_2D,
                        GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D,
                        GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//        ws->shader = &ws->texture_shader;
     }
   else
     glBindTexture(GL_TEXTURE_2D, ws->texture);

   /* if (ws->saved_texture != 0) */
   /*   ws->texture = ws->saved_texture; */

   /* glBindTexture(GL_TEXTURE_2D, ws->texture); */

   if (wl_buffer_is_shm(buffer))
     ws->pitch = wl_shm_buffer_get_stride(buffer) / 4;
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
        ws->pitch = ws->w;
     }
}

