#ifndef E_FILE_H
#define E_FILE_H

#include <sys/time.h>
#include <sys/stat.h>
#include <Evas.h>

time_t     e_file_mod_time(char *file);
int        e_file_exists(char *file);
int        e_file_is_dir(char *file);
int        e_file_mkdir(char *dir);
int        e_file_mkpath(char *path);
int        e_file_cp(char *src, char *dst);
char      *e_file_realpath(char *file);
char      *e_file_get_file(char *path);
char      *e_file_get_dir(char *path);

int        e_file_can_exec(struct stat *st);
char      *e_file_readlink(char *link);
Evas_List *e_file_ls(char *dir);

#endif
