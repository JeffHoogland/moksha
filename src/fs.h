#ifndef E_FS_H
#define E_FS_H

#include <libefsd.h>
#include "e.h"

typedef struct _E_FS_Restarter        E_FS_Restarter;

struct _E_FS_Restarter
{
   void (*func) (void *data);
   void *data;
};

void            e_fs_init(void);
E_FS_Restarter *e_fs_add_restart_handler(void (*func) (void *data), void *data);
void            e_fs_del_restart_handler(E_FS_Restarter *rs);
void            e_fs_add_event_handler(void (*func) (EfsdEvent *ev));
EfsdConnection *e_fs_get_connection(void);

#endif
