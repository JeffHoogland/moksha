#include <X11/Xlib.h>
#include <Imlib2.h>

int main(int argc, char **argv)
{
   Imlib_Image im;
   
   if (argc == 1)
     {
	printf("usage:\n\t%s source_image.png dest.db:/key/in/db\n", argv[0]);
	exit(-1);
     }
   im = imlib_load_image(argv[1]);
   if (im)
     {
	imlib_context_set_image(im);
	imlib_image_attach_data_value("compression", NULL, 9, NULL);
	imlib_image_set_format("db");
	imlib_save_image(argv[2]);
     }
   return 0;
}
