#include "e.h"
#include "e_mod_main.h"
#ifdef HAVE_WAYLAND
# include <xcb/xcb_image.h>
# include "e_mod_comp_wl.h"
# include "e_mod_comp_wl_comp.h"
# include "e_mod_comp_wl_shm.h"
# include "e_mod_comp_wl_output.h"
# include "e_mod_comp_wl_input.h"
# include "e_mod_comp_wl_shell.h"
#endif

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
        EINA_LOG_ERR("Failed to create wayland display\n");
        return EINA_FALSE;
     }

   /* init a wayland compositor ?? */
   if (!e_mod_comp_wl_comp_init())
     {
        wl_display_terminate(_wl_disp);
        EINA_LOG_ERR("Failed to create wayland shm\n");
        return EINA_FALSE;
     }

   /* init shm */
   if (!e_mod_comp_wl_shm_init())
     {
        e_mod_comp_wl_comp_shutdown();
        wl_display_terminate(_wl_disp);
        EINA_LOG_ERR("Failed to create wayland shm\n");
        return EINA_FALSE;
     }

   /* init output */
   if (!e_mod_comp_wl_output_init())
     {
        e_mod_comp_wl_shm_shutdown();
        e_mod_comp_wl_comp_shutdown();
        wl_display_terminate(_wl_disp);
        EINA_LOG_ERR("Failed to create wayland output\n");
        return EINA_FALSE;
     }

   /* init input */
   if (!e_mod_comp_wl_input_init())
     {
        e_mod_comp_wl_output_shutdown();
        e_mod_comp_wl_shm_shutdown();
        e_mod_comp_wl_comp_shutdown();
        wl_display_terminate(_wl_disp);
        EINA_LOG_ERR("Failed to create wayland input\n");
        return EINA_FALSE;
     }

   /* init a wayland shell */
   if (!e_mod_comp_wl_shell_init())
     {
        e_mod_comp_wl_input_shutdown();
        e_mod_comp_wl_output_shutdown();
        e_mod_comp_wl_shm_shutdown();
        e_mod_comp_wl_comp_shutdown();
        wl_display_terminate(_wl_disp);
        EINA_LOG_ERR("Failed to create wayland shell\n");
        return EINA_FALSE;
     }

   if (wl_display_add_socket(_wl_disp, NULL))
     {
        e_mod_comp_wl_shell_shutdown();
        e_mod_comp_wl_input_shutdown();
        e_mod_comp_wl_output_shutdown();
        e_mod_comp_wl_shm_shutdown();
        e_mod_comp_wl_comp_shutdown();
        wl_display_terminate(_wl_disp);
        EINA_LOG_ERR("Failed to add socket to wayland display\n");
        return EINA_FALSE;
     }

   loop = wl_display_get_event_loop(_wl_disp);
   fd = wl_event_loop_get_fd(loop);

   _wl_fd_handler = 
     ecore_main_fd_handler_add(fd, ECORE_FD_READ, 
                               _e_mod_comp_wl_fd_handle, NULL, NULL, NULL);

   wl_event_loop_dispatch(loop, 0);

   return EINA_TRUE;
}

void 
e_mod_comp_wl_shutdown(void)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (_wl_fd_handler)
     ecore_main_fd_handler_del(_wl_fd_handler);
   _wl_fd_handler = NULL;

   e_mod_comp_wl_shell_shutdown();
   e_mod_comp_wl_input_shutdown();
   e_mod_comp_wl_output_shutdown();
   e_mod_comp_wl_shm_shutdown();
   e_mod_comp_wl_comp_shutdown();

   if (_wl_disp) wl_display_terminate(_wl_disp);
   _wl_disp = NULL;
}

uint32_t 
e_mod_comp_wl_time_get(void)
{
   struct timeval tv;

   gettimeofday(&tv, NULL);
   return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

Ecore_X_Pixmap 
e_mod_comp_wl_pixmap_get(Ecore_X_Window win)
{
   Wayland_Compositor *comp;
   Wayland_Surface *ws;
   struct wl_list *list;
   Ecore_X_Pixmap pmap = 0;

   comp = e_mod_comp_wl_comp_get();
   list = &comp->surfaces;
   wl_list_for_each(ws, list, link)
     {
        if ((ws->win) && (ws->win->border->win == win))
          {
             if (ws->buffer)
               {
                  Ecore_X_Connection *conn;
                  Ecore_X_GC gc;
                  uint8_t *pix;
                  int depth;

                  depth = ecore_x_window_depth_get(win);
                  conn = ecore_x_connection_get();

                  pmap = xcb_generate_id(conn);
                  xcb_create_pixmap(conn, depth, pmap, win, ws->w, ws->h);

                  if (wl_buffer_is_shm(ws->buffer))
                    pix = (uint8_t *)wl_shm_buffer_get_data(ws->buffer);
                  else
                    {
                       /* FIXME: egl buffer ?? */
                       printf("Wayland Buffer is NOT SHM !!\n");
                       return 0;
                    }

                  gc = ecore_x_gc_new(pmap, 0, NULL);
                  xcb_put_image(conn, 2, pmap, gc, ws->w, ws->h, 
                                0, 0, 0, depth, 
                                (ws->w * ws->h * sizeof(int)), pix);
                  ecore_x_gc_free(gc);
               }
             else if (ws->image)
               {
                  /* NB: No buffer means it may be an egl surface */
                  printf("Get Pixmap Data from EGL Surface !!!\n");
               }
          }
     }

   return pmap;
}

/* local functions */
static Eina_Bool 
_e_mod_comp_wl_fd_handle(void *data __UNUSED__, Ecore_Fd_Handler *hdl __UNUSED__)
{
   struct wl_event_loop *loop;

   loop = wl_display_get_event_loop(_wl_disp);
   wl_event_loop_dispatch(loop, 0);
   return ECORE_CALLBACK_RENEW;
}
