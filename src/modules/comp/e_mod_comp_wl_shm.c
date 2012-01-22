#include "e.h"
#include "e_mod_main.h"
#ifdef HAVE_WAYLAND
# include "e_mod_comp_wl.h"
# include "e_mod_comp_wl_shm.h"
#endif

void 
e_mod_comp_wl_shm_buffer_created(struct wl_buffer *buffer)
{
   struct wl_list *attached;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(attached = malloc(sizeof(*attached))))
     {
        EINA_LOG_ERR("Failed to allocate attached list\n");
        buffer->user_data = NULL;
        return;
     }
   wl_list_init(attached);
   buffer->user_data = attached;
}

void 
e_mod_comp_wl_shm_buffer_damaged(struct wl_buffer *buffer, int32_t x __UNUSED__, int32_t y __UNUSED__, int32_t width __UNUSED__, int32_t height __UNUSED__)
{
   struct wl_list *attached;
   GLsizei tex_width;
   Wayland_Surface *ws;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   attached = buffer->user_data;
   tex_width = wl_shm_buffer_get_stride(buffer) / 4;

   wl_list_for_each(ws, attached, buffer_link)
     {
        glBindTexture(GL_TEXTURE_2D, ws->texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_BGRA_EXT, 
                     tex_width, buffer->height, 0, GL_BGRA_EXT, 
                     GL_UNSIGNED_BYTE, wl_shm_buffer_get_data(buffer));
     }
}

void 
e_mod_comp_wl_shm_buffer_destroyed(struct wl_buffer *buffer)
{
   struct wl_list *attached;
   Wayland_Surface *ws, *next;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   attached = buffer->user_data;
   wl_list_for_each_safe(ws, next, attached, buffer_link)
     {
        wl_list_remove(&ws->buffer_link);
        wl_list_init(&ws->buffer_link);
     }
   free(attached);
}

