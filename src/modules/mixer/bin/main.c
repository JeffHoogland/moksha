#include "../lib/common.h"
#include "../lib/epulse.h"
#include "main_window.h"

#define DEFAULT_HEIGHT 600
#define DEFAULT_WIDTH 800

EAPI int
elm_main(int argc EINA_UNUSED, char *argv[]  EINA_UNUSED)
{
   Evas_Object *win;

   EINA_SAFETY_ON_FALSE_RETURN_VAL(epulse_common_init("epulse"), EXIT_FAILURE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(epulse_init() > 0, EXIT_FAILURE);

   win = main_window_add();
   evas_object_resize(win, DEFAULT_WIDTH, DEFAULT_HEIGHT);
   evas_object_show(win);

   elm_run();

   epulse_common_shutdown();
   epulse_shutdown();
   return 0;
}

/*
 * Create the default main() that will work with both quicklaunch or
 * regular applications.
 */
ELM_MAIN()
