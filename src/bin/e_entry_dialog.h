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
      void (*func) (void *data, char *text);
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
					 void (*ok_func) (void *data, char *text),
					 void (*cancel_func) (void *data),
					 void *data);

#endif
#endif
