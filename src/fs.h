#ifndef E_FS_H
#define E_FS_H

#include <libefsd.h>

typedef struct _E_FS_Restarter E_FS_Restarter;

struct _E_FS_Restarter
{
   void                (*func) (void *data);
   void               *data;
};

/**
 * e_fs_init - Filesystem code initialization.
 *
 * This function makes sure that efsd is running,
 * starts it when necessary, and makes sure that
 * it can be restarted in case efsd dies.
 */
void                e_fs_init(void);

/**
 * e_fs_cleanup - Filesystem code cleanup routine.
 *
 * This function takes care of properly saying
 * good bye to Efsd.
 */
void                e_fs_cleanup(void);

E_FS_Restarter     *e_fs_add_restart_handler(void (*func) (void *data),
					     void *data);
void                e_fs_del_restart_handler(E_FS_Restarter * rs);
void                e_fs_add_event_handler(void (*func) (EfsdEvent * ev));
EfsdConnection     *e_fs_get_connection(void);

#endif
