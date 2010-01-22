/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Wizard_Page E_Wizard_Page;

#else
#ifndef E_WIZARD_H
#define E_WIZARD_H

struct _E_Wizard_Page
{
   void *handle;
   Evas *evas;
   int (*init)     (E_Wizard_Page *pg);
   int (*shutdown) (E_Wizard_Page *pg);
   int (*show)     (E_Wizard_Page *pg);
   int (*hide)     (E_Wizard_Page *pg);
   int (*apply)    (E_Wizard_Page *pg);
   void *data;
};

int e_wizard_init(void);
int e_wizard_shutdown(void);
void e_wizard_go(void);
void e_wizard_apply(void);
void e_wizard_next(void);
void e_wizard_page_show(Evas_Object *obj);
E_Wizard_Page *
  e_wizard_page_add(void *handle,
		    int (*init)     (E_Wizard_Page *pg),
		    int (*shutdown) (E_Wizard_Page *pg),
		    int (*show)     (E_Wizard_Page *pg),
		    int (*hide)     (E_Wizard_Page *pg),
		    int (*apply)    (E_Wizard_Page *pg)
		    );
void e_wizard_page_del(E_Wizard_Page *pg);
void e_wizard_button_next_enable_set(int enable);
void e_wizard_title_set(const char *title);
void e_wizard_labels_update(void);
    
#endif
#endif
