#include "e.h"

void         e_ipc_init(void);
static char *e_ipc_get_version(char *argv);

void
e_ipc_init(void)
{
  char buf[4096];

  /* unix domain socket file path */
  sprintf(buf, "%secom", e_config_user_dir());

  /* init ecore ipc */
  e_ev_ipc_init(buf);

  /* add ipc services or functions clients can use */
  e_add_ipc_service(0, e_ipc_get_version);
}

static char 
*e_ipc_get_version(char *argv)
{
  printf("e_ipc_get_version service called\n"); fflush(stdout);
  return "0.17.0";
}
