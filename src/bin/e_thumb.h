/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_THUMB_H
#define E_THUMB_H


EAPI int                   e_thumb_init(void);
EAPI int                   e_thumb_shutdown(void);
EAPI const char           *e_thumb_dir_get(void);
EAPI char                 *e_thumb_file_get(char *file);
EAPI int                   e_thumb_exists(char *file);
EAPI int                   e_thumb_create(char *file, Evas_Coord w, Evas_Coord h);
EAPI Evas_Object          *e_thumb_evas_object_get(char *file, Evas *evas, Evas_Coord width, Evas_Coord height);

#endif
#endif
