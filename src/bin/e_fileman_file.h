/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Fm_File            E_Fm_File;
typedef struct _E_Fm_File_Attributes E_Fm_File_Attributes;

#define E_FM_FILE_TYPE_FILE      (1 << 8)
#define E_FM_FILE_TYPE_DIRECTORY (1 << 9)
#define E_FM_FILE_TYPE_SYMLINK   (1 << 10)
#define E_FM_FILE_TYPE_UNKNOWN   (1 << 11)
#define E_FM_FILE_TYPE_HIDDEN    (1 << 12)

#define E_FM_FILE_TYPE_NORMAL    E_FM_FILE_TYPE_FILE|E_FM_FILE_TYPE_DIRECTORY|E_FM_FILE_TYPE_SYMLINK
#define E_FM_FILE_TYPE_ALL       E_FM_FILE_TYPE_NORMAL|E_FM_FILE_TYPE_HIDDEN

#else
#ifndef E_FILEMAN_FILE_H
#define E_FILEMAN_FILE_H

#define E_FM_FILE_TYPE 0xE0b01018

struct _E_Fm_File
{
   E_Object e_obj_inherit;

   int          type;

   Evas        *evas; /* what is it used for? the icon theme? use the mime instead */
   Evas_Object *icon_object;
   /* Do we need those?
    * Evas_Object *image_object;
    * Evas_Object *event_object;
    */

   char     *path;            /* full name with path */
   char     *name;            /* file name without parent directories */
   E_Fm_Mime_Entry *mime;

   dev_t     device;          /* ID of device containing file */
   ino_t     inode;           /* inode number */
   mode_t    mode;            /* protection */
   nlink_t   nlink;           /* number of hard links */
   uid_t     owner;           /* user ID of owner */
   gid_t     group;           /* group ID of owner */
   dev_t     rdev;            /* device ID (if special file) */
   off_t     size;            /* total size, in bytes */   
   time_t    atime;           /* time of last access */
   time_t    mtime;           /* time of last modification */
   time_t    ctime;           /* time of last status change */
   
};

EAPI E_Fm_File *e_fm_file_new         (const char *filename);
EAPI int        e_fm_file_rename      (E_Fm_File *file, const char *name);
EAPI int        e_fm_file_delete      (E_Fm_File *file);
EAPI int        e_fm_file_copy        (E_Fm_File *file, const char *name);
EAPI int        e_fm_file_can_preview (E_Fm_File *file);
EAPI int        e_fm_file_is_image    (E_Fm_File *file);
EAPI int        e_fm_file_is_etheme   (E_Fm_File *file);
EAPI int        e_fm_file_is_ebg      (E_Fm_File *file);
EAPI int        e_fm_file_is_eap      (E_Fm_File *file);
EAPI int        e_fm_file_can_exec    (E_Fm_File *file);
EAPI int        e_fm_file_exec        (E_Fm_File *file);
EAPI int        e_fm_file_assoc_set   (E_Fm_File *file, const char *assoc);
EAPI int        e_fm_file_assoc_exec  (E_Fm_File *file);
EAPI int				e_fm_file_has_mime    (E_Fm_File *file, char* mime);
EAPI int        e_fm_file_is_regular  (E_Fm_File *file);
#endif
#endif

