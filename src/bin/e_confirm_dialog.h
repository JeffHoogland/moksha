/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEF

#else
#ifndef E_CONFIRM_DIALOG_H
#define E_CONFIRM_DIALOG_H

/*
 * @title - dialog title
 * @icon  - dialog icon
 * @text - the text show in the dialog
 * @button_text - "yes" button text
 * @button2_text - "no" button text
 * func - the function is called if yes is pressed
 * func2 - the function is called if no is pressed
 * data - the pointer passed to func
 * data2 - the pointer passed to func2
 */
EAPI void e_confirm_dialog_show(const char *title, const char *icon, const char *text, const char *button_text, const char *button2_text, void (*func)(void *data), void (*func2)(void *data), void *data, void *data2);

#endif
#endif
