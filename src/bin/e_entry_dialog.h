#ifdef E_TYPEDEF

#else
#ifndef E_ENTRY_DIALOG_H
#define E_ENTRY_DIALOG_H

/*
 * @title - dialog title
 * @icon - dialog icon
 * @text - the text shown in the dialog
 * @button_text - "Ok" button text
 * @button2_text - "Cancel" button text
 * @func - the function to call if ok is pressed
 * @func2 - the function to call if cancel is pressed
 * @data - the pointer passed to func
*/

EAPI void e_entry_dialog_show(const char *title, const char *icon, const char *text, 
			      const char *button_text, const char *button2_text, 
			      void (*func)(char *text, void *data), 
			      void (*func2)(void *data), void *data);

#endif
#endif
