#include <X11/Xlib.h>
#include <Imlib2.h>
#include <Edb.h>
#include <stdio.h>
#include <stdlib.h>

static int sort_compare(const void *v1, const void *v2);

static int
sort_compare(const void *v1, const void *v2)
{
   return strcmp(*(char **)v1, *(char **)v2);
}

int main(int argc, char **argv)
{
   Imlib_Image im;
   
   if (argc == 1)
     {
	printf("usage:\n\t%s src.db:/key/in/db dest_image\n", argv[0]);
	printf("usage:\n\t%s src.db\n", argv[0]);
	exit(-1);
     }
   if (argc == 2)
     {
	E_DB_File *db;
	char **keys;
	int keys_num;
	int i;
	
	db = e_db_open_read(argv[1]);
	if (!db)
	  {
	     printf("Cannot load db:\n");
	     printf("  %s\n", argv[1]);	     
	     exit(0);
	  }
	keys_num = 0;
	keys = e_db_dump_key_list(argv[1], &keys_num);
	qsort(keys, keys_num, sizeof(char *), sort_compare);
	printf("Possible images in db file:\n");
	printf("  %s\n", argv[1]);
	printf("Possible entries: %i\n", keys_num);
	printf("  filtering out known non-image entries...\n");
	printf("---\n");
	for (i = 0; i < keys_num; i++)
	  {
	     char *t;
	     char *type;
	     
	     type = e_db_type_get(db, keys[i]);
	     if (
		 (!type) ||
		 (
		  (!(!strcmp(type, "int"))) &&
		  (!(!strcmp(type, "float"))) &&
		  (!(!strcmp(type, "str")))
		  )
		 )
	       printf("%s:%s\n", argv[1], keys[i]);
	     if (type) free(type);
	  }
	e_db_close(db);
	e_db_flush();
	exit(0);
     }
   im = imlib_load_image(argv[1]);
   if (im)
     {
	imlib_context_set_image(im);
	imlib_image_attach_data_value("compression", NULL, 9, NULL);
	imlib_image_set_format("png");
	imlib_save_image(argv[2]);
     }
   return 0;
}
