/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#ifdef E_TYPEDEFS

typedef struct _E_Fm_Mime_Entry E_Fm_Mime_Entry;
typedef struct _E_Fm_Mime_Action E_Fm_Mime_Action;
typedef int    (*E_Fm_Mime_Preview_Function) (E_Fm_File*);

#if 0
file->preview_funcs = E_NEW(E_Fm_File_Preview_Function, 5);
   file->preview_funcs[0] = e_fm_file_is_image;
   file->preview_funcs[1] = e_fm_file_is_etheme;
   file->preview_funcs[2] = e_fm_file_is_ebg;
   file->preview_funcs[3] = e_fm_file_is_eap;
   file->preview_funcs[4] = NULL;
#endif

#else
#ifndef E_FILEMAN_MIME_H
#define E_FILEMAN_MIME_H


struct _E_Fm_Mime_Entry
{
   char             *name;
   char             *label;
   int              level; /* the level on the three for easy search/comparsion */
   E_Fm_Mime_Entry  *parent;
   E_Fm_Mime_Action *action_default; /* the default action also exists on the actions list */
   E_Fm_Mime_Action *action_default_relative;
   Evas_List        *actions;
   /* the autodetect features */
   char             *suffix;
   int              type;
   /* to thumbnail this file type */
   Evas_Object * (*thumbnail) (char *path, Evas_Coord w, Evas_Coord h, Evas *evas, Evas_Object **tmp, void (*cb)(Evas_Object *obj, void *data), void *data);
   /* to preview this file type */
   E_Fm_Mime_Preview_Function preview; 

};

struct _E_Fm_Mime_Action
{
   char *name;
   char *label;
   char *cmd;
   unsigned char  multiple : 1;    /* support for multiple files at once */
   unsigned char  is_internal : 1; /* if its internal cant be modified */
   unsigned char  relative : 1;    /* if the action MUST be realitve to a dir */
   struct
     {
	void (*function)(E_Fm_Smart_Data *sd);
     } internal;
};

EAPI int               e_fm_mime_init(void);
EAPI void              e_fm_mime_shutdwon(void);
EAPI E_Fm_Mime_Entry  *e_fm_mime_get_from_list(Evas_List *files);
EAPI void              e_fm_mime_set(E_Fm_File *file);
EAPI int               e_fm_mime_action_call(E_Fm_Smart_Data *sd, E_Fm_Mime_Action *action);
EAPI void              e_fm_mime_action_default_call(E_Fm_Smart_Data *sd);
EAPI char             *e_fm_mime_translate(E_Fm_Smart_Data *sd, char *istr);


#endif
#endif
