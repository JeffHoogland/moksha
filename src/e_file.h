#ifndef E_EFILE_H
#define E_EFILE_H
#include "object.h"
#include <sys/stat.h>

typedef struct _E_File E_File;
struct _E_File
{
   E_Object            o;

   char               *file;
   struct stat         stat;

   struct
   {
      char               *icon;
      char               *custom_icon;
      char               *link;
      struct
      {
	 char               *base;
	 char               *type;
      }
      mime;
   }
   info;
};

E_File             *e_file_new(char *file);
E_File             *e_file_get_by_name(Evas_List * l, char *file);
void                e_file_set_mime(E_File * f, char *base, char *mime);
void                e_file_set_link(E_File * f, char *link);

#endif
