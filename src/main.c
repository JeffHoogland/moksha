#include "e.h"

#ifdef E_PROF
Evas_List __e_profiles = NULL;
#endif

static void cb_exit(void);
static void cb_exit(void)
{
   E_PROF_DUMP;
}

static void ch_col(int val, void *data);
static void ch_col(int val, void *data)
{
   E_Desktop *desk;
   double v;
   
   v = (double)val / 10;
   desk = e_desktops_get(e_desktops_get_current());
   e_desktops_scroll(desk, (int)(8 * sin(v)), (int)(8 * cos(v)));
   e_add_event_timer("time", 0.02, ch_col, val + 1, NULL);
   return;
   UN(data);
}

void setup(void);
void
setup(void)
{
   e_grab();
   e_window_set_events(0, XEV_CHILD_REDIRECT | XEV_PROPERTY | XEV_COLORMAP);
   e_border_adopt_children(0);
   e_ungrab();
/*   e_add_event_timer("timer", 0.02, ch_col, 0, NULL);*/
}

int
main(int argc, char **argv)
{
   atexit(cb_exit);
   e_exec_set_args(argc, argv);
   
   e_config_init();
   e_display_init(NULL);
   e_ev_signal_init();
   e_event_filter_init();
   e_ev_x_init();

   e_desktops_init();
   e_border_init();
   e_actions_init();
   e_menu_init();
   /* e_fs_init(); */
   e_view_init();
   
   setup();
   
   e_event_loop();

   return 0;
   UN(argc);
   UN(argv);
}
