#include "e.h"

struct _coords {
  int xid;
  int x;
  int y;
};

typedef struct _coords coords;

/* void         e_ipc_init(void); */
static void  e_ipc_get_version(int fd);
static void  e_ipc_move_window(int fd);

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
  e_add_ipc_service(1, e_ipc_move_window);
}

static void
e_ipc_get_version(int fd)
{
  e_ipc_send_data(fd, &VERSION, strlen(VERSION));
}

static void
e_ipc_move_window(int fd)
{
  coords test;
  int retval = 0;

  /* get window id and coords to move to */
  e_ipc_get_data(fd, &test);

  /* move window here */

  /* return failure or success */
  e_ipc_send_data(fd, &retval, sizeof(retval));
}
