/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_File_Dialog E_File_Dialog;

#else
#ifndef E_FILE_DIALOG_H
#define E_FILE_DIALOG_H

#define E_FILE_DIALOG_TYPE 0xE0b01020

struct _E_File_Dialog
{
   E_Object             e_obj_inherit;
   
   E_Container         *con;
   E_Dialog            *dia;
   
   char                *file;
   
   void (*select_func)(E_File_Dialog *dia, char *file, void *data);
   void  *select_data;
};

EAPI E_File_Dialog  *e_file_dialog_new               (E_Container *con);
EAPI void            e_file_dialog_show              (E_File_Dialog *dia);
EAPI void            e_file_dialog_title_set         (E_File_Dialog *dia, const char *title);
EAPI void            e_file_dialog_select_callback_add(E_File_Dialog *dia, void (*func)(E_File_Dialog *dia, char *file, void *data), void *data);

#endif
#endif
