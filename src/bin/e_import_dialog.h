#ifdef E_TYPEDEFS
typedef struct _E_Import_Dialog E_Import_Dialog;
#else
#ifndef E_IMPORT_DIALOG_H
#define E_IMPORT_DIALOG_H

#define E_IMPORT_DIALOG_TYPE 0xE0b0103f
struct _E_Import_Dialog
{
   E_Object              e_obj_inherit;
   Evas_Object          *fsel_obj;
   Ecore_End_Cb          ok;
   Ecore_Cb              cancel;

   E_Dialog             *dia;
};

EAPI E_Import_Dialog *e_import_dialog_show(E_Container *con, const char *dev, const char *path, Ecore_End_Cb ok, Ecore_Cb cancel);

#endif
#endif
