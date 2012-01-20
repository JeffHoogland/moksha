#ifdef E_TYPEDEFS
#else
# ifndef E_MOD_COMP_WAYLAND_H
#  define E_MOD_COMP_WAYLAND_H

#  include <wayland-server.h>
#  include <GLES2/gl2.h>
#  include <GLES2/gl2ext.h>
#  include <EGL/egl.h>
#  include <EGL/eglext.h>
#  include <pixman.h>

/* variables & functions here */
Eina_Bool e_mod_comp_wayland_init(void);
void e_mod_comp_wayland_shutdown(void);
void e_mod_comp_wayland_repaint(void);

# endif
#endif
