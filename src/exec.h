#ifndef E_EXEC_H
#define E_EXEC_H

#include "e.h"

void  e_exec_set_args(int argc, char **argv);
void  e_exec_restart(void);
pid_t e_exec_run(char *exe);
pid_t e_exec_run_in_dir(char *exe, char *dir);
pid_t e_exec_in_dir_with_env(char *exe, char *dir, int *launch_id_ret, char **env, char *launch_path);

#endif
