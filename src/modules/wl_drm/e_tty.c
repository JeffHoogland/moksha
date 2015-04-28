#include "e.h"
#include "e_mod_main.h"

/* local function prototypes */
static int _e_tty_open_vt(E_Tty *et);
static int _e_tty_cb_input(int fd __UNUSED__, uint32_t mask __UNUSED__, void *data);
static int _e_tty_cb_handle(int sig __UNUSED__, void *data);

EINTERN E_Tty *
e_tty_create(E_Compositor *comp, tty_vt_func_t vt_func, int tty)
{
   E_Tty *et = NULL;
   struct stat buff;
   struct termios raw_attribs;
   struct wl_event_loop *loop;
   struct vt_mode mode;
   int ret = 0;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(et = malloc(sizeof(E_Tty))))
     {
        e_error_message_show(_("Could not allocate space for E_Tty\n"));
        return NULL;
     }

   memset(et, 0, sizeof(E_Tty));

   et->comp = comp;
   et->vt_func = vt_func;
   if (tty > 0)
     {
        char fname[16];

        snprintf(fname, sizeof(fname), "/dev/tty%d", tty);
        printf("Using %s for tty\n", fname);
        et->fd = open(fname, O_RDWR | O_NOCTTY | O_CLOEXEC);
     }
   else if ((fstat(et->fd, &buff) == 0) && 
            (major(buff.st_rdev) == TTY_MAJOR) && (minor(buff.st_rdev) > 0))
     et->fd = fcntl(0, F_DUPFD_CLOEXEC, 0);
   else 
     et->fd = _e_tty_open_vt(et);

   if (et->fd <= 0)
     {
        e_error_message_show(_("Could not open a tty.\n"));
        free(et);
        return NULL;
     }

   if (tcgetattr(et->fd, &et->term_attribs) < 0)
     {
        e_error_message_show(_("Could not get terminal attributes: %m\n"));
        free(et);
        return NULL;
     }

   raw_attribs = et->term_attribs;
   cfmakeraw(&raw_attribs);

   raw_attribs.c_oflag |= (OPOST | OCRNL);
   if (tcsetattr(et->fd, TCSANOW, &raw_attribs) < 0)
     {
        e_error_message_show(_("Could not put terminal in raw mode: %m\n"));
        free(et);
        return NULL;
     }

   loop = wl_display_get_event_loop(comp->display);
   et->input_source = 
     wl_event_loop_add_fd(loop, et->fd, WL_EVENT_READABLE, _e_tty_cb_input, et);

   if ((ret = ioctl(et->fd, KDSETMODE, KD_GRAPHICS)))
     {
        e_error_message_show(_("Failed to set graphics mode on tty: %m\n"));
        free(et);
        return NULL;
     }

   et->has_vt = EINA_TRUE;
   mode.mode = VT_PROCESS;
   mode.relsig = SIGUSR1;
   mode.acqsig = SIGUSR1;
   if (ioctl(et->fd, VT_SETMODE, &mode) < 0)
     {
        e_error_message_show(_("Failed to take control of vt handling: %m\n"));
        free(et);
        return NULL;
     }

   et->vt_source = 
     wl_event_loop_add_signal(loop, SIGUSR1, _e_tty_cb_handle, et);

   return et;
}

EINTERN void 
e_tty_destroy(E_Tty *et)
{
   struct vt_mode mode;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!et) return;
   if (ioctl(et->fd, KDSETMODE, KD_TEXT))
     e_error_message_show(_("Failed to set KD_TEXT mode on tty: %m\n"));
   if (tcsetattr(et->fd, TCSANOW, &et->term_attribs) < 0)
     e_error_message_show(_("Could not restore terminal to canonical mode: %m\n"));
   mode.mode = VT_AUTO;
   if (ioctl(et->fd, VT_SETMODE, &mode) < 0)
     e_error_message_show(_("Could not reset vt handling: %m\n"));
   if ((et->has_vt) && (et->vt != et->start_vt))
     {
        ioctl(et->fd, VT_ACTIVATE, et->start_vt);
        ioctl(et->fd, VT_WAITACTIVE, et->start_vt);
     }
   close(et->fd);
   free(et);
}

/* local functions */
static int 
_e_tty_open_vt(E_Tty *et)
{
   int tty0, fd;
   char fname[16];
   struct vt_stat vts;

   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   if ((tty0 = open("/dev/tty0", O_WRONLY | O_CLOEXEC)) < 0)
     {
        e_error_message_show(_("Failed to open tty0: %m\n"));
        return -1;
     }

   if ((ioctl(tty0, VT_OPENQRY, &et->vt) < 0) || (et->vt == -1))
     {
        e_error_message_show(_("Failed to open tty0: %m\n"));
        close(tty0);
        return -1;
     }
   close(tty0);

   snprintf(fname, sizeof(fname), "/dev/tty%d", et->vt);
   printf("Compositor: Using new vt %s\n", fname);
   fd = open(fname, O_RDWR | O_NOCTTY | O_CLOEXEC);
   if (fd < 0) return fd;

   if (ioctl(fd, VT_GETSTATE, &vts) == 0)
     et->start_vt = vts.v_active;
   else
     et->start_vt = et->vt;

   if ((ioctl(fd, VT_ACTIVATE, et->vt) < 0) || 
       (ioctl(fd, VT_WAITACTIVE, et->vt) < 0))
     {
        e_error_message_show(_("Failed to switch to new vt: %m\n"));
        close(fd);
        return -1;
     }

   return fd;
}

static int 
_e_tty_cb_input(int fd __UNUSED__, uint32_t mask __UNUSED__, void *data)
{
   E_Tty *et;

//   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   et = data;
   tcflush(et->fd, TCIFLUSH);
   return 1;
}

static int 
_e_tty_cb_handle(int sig __UNUSED__, void *data)
{
   E_Tty *et;

//   DLOGFN(__FILE__, __LINE__, __FUNCTION__);

   et = data;
   if (et->has_vt)
     {
        et->vt_func(et->comp, 1);
        et->has_vt = EINA_FALSE;
        ioctl(et->fd, VT_RELDISP, 1);
     }
   else
     {
        ioctl(et->fd, VT_RELDISP, VT_ACKACQ);
        et->vt_func(et->comp, 0);
        et->has_vt = EINA_TRUE;
     }

   return 1;
}
