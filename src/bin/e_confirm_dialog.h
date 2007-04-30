/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Confirm_Dialog E_Confirm_Dialog;

#else
#ifndef E_CONFIRM_DIALOG_H
#define E_CONFIRM_DIALOG_H

#define E_CONFIRM_DIALOG_TYPE 0x16f5904e

struct _E_Confirm_Dialog
{
   E_Object  e_obj_inherit;
   
   E_Dialog *dia;

   struct 
     {
	void *data;
	void (*func)(void *data);
     } yes;
   struct
     {
	void *data;
	void (*func)(void *data);
     } no;
   struct
     {
	void *data;
	void (*func)(void *data);
     } del;
};

/*
 * @title - dialog title
 * @icon  - dialog icon
 * @text - the text show in the dialog
 * @button_text - "yes" button text
 * @button2_text - "no" button text
 * func - the function to call if yes is pressed
 * func2 - the function to call if no is pressed
 * data - the pointer passed to func
 * data2 - the pointer passed to func2
 * del_func - the function to call before dialog is deleted
 * del_data - the pointer passer to del_func
 */
EAPI E_Confirm_Dialog *e_confirm_dialog_show(const char *title, const char *icon, const char *text, const char *button_text, const char *button2_text, void (*func)(void *data), void (*func2)(void *data), void *data, void *data2, void (*del_func)(void *data), void * del_data);

#endif
#endif
