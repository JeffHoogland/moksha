#ifndef E_FILE_H
#define E_FILE_H
#include <sys/time.h>
#include <sys/stat.h>
#include <Evas.h>

time_t              e_file_mod_time(char *file);
int                 e_file_exists(char *file);
int                 e_file_is_dir(char *file);
int                 e_file_mkdir(char *dir);
int                 e_file_cp(char *src, char *dst);
char               *e_file_realpath(char *file);

/**
 * e_file_get_file - returns file in a path
 * @path:  The chanonical path to a file
 *
 * This functions returns the file name (everything
 * after the last "/") of a full path. It returns
 * a pointer into the original string, so you don't
 * need to free the result.
 */
char               *e_file_get_file(char *path);

/**
 * e_file_get_dir - returns directory in a path
 * @path:  The chanonical path to a file
 *
 * This functions returns the directory name (everything
 * before the last "/") of a full path. It returns
 * a freshly allocated string, so you need to free
 *the result.
 */
char               *e_file_get_dir(char *path);

int                 e_file_can_exec(struct stat *st);
char               *e_file_readlink(char *link);
Evas_List *           e_file_ls(char *dir);

#endif
