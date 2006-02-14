/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#ifdef E_TYPEDEFS

typedef struct _E_Fm_Mime_Entry E_Fm_Mime_Entry;
typedef struct _E_Fm_Mime_Action E_Fm_Mime_Action;

#else
#ifndef E_FILEMAN_MIME_H
#define E_FILEMAN_MIME_H


struct _E_Fm_Mime_Entry
{
   char            *name;
   char            *label;
   int             level; /* the level on the three for easy search/comparsion */
   E_Fm_Mime_Entry *parent;
   Evas_List       *actions;
   /* the autodetect features */
   char            *suffix;
   int             type;
};

struct _E_Fm_Mime_Action
{
   char *name;
   char *label;
   char *cmd;
   int  multiple; /* support for multiple files at once */
};

EAPI int              e_fm_mime_init(void);
EAPI void             e_fm_mime_shutdwon(void);
EAPI void             e_fm_mime_set(E_Fm_File *file);
EAPI void             e_fm_mime_action_call(Evas_List *files, E_Fm_Mime_Action *action);
EAPI E_Fm_Mime_Entry *e_fm_mime_common(E_Fm_Mime_Entry *e1, E_Fm_Mime_Entry *e2);
EAPI E_Fm_Mime_Entry *e_fm_mime_list(Evas_List *mis);


#endif
#endif
