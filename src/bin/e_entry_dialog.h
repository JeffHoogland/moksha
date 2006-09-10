/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Entry_Dialog E_Entry_Dialog;

#else
#ifndef E_ENTRY_DIALOG_H
#define E_ENTRY_DIALOG_H

#define E_ENTRY_DIALOG_TYPE 0xE0b0101d

struct _E_Entry_Dialog
{
   E_Object e_obj_inherit;
   
   E_Dialog *dia;
   Evas_Object *entry;
   char *text;
   struct {
      void (*func) (char *text, void *data);
      void *data;
   } ok;
   struct {
      void (*func) (void *data);
      void *data;
   } cancel;
};

EAPI E_Entry_Dialog *e_entry_dialog_show(const char *title,
					 const char *icon,
					 const char *text,
					 const char *initial_text,
					 const char *button_text,
					 const char *button2_text, 
					 void (*ok_func) (char *text, void *data), 
					 void (*cancel_func) (void *data),
					 void *data);

#endif
#endif
