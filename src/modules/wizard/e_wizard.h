#ifndef E_WIZARD_H

#include "e.h"

extern E_Module *wiz_module;

typedef struct _E_Wizard_Page E_Wizard_Page;

typedef enum
{
   E_WIZARD_PAGE_STATE_INIT,
   E_WIZARD_PAGE_STATE_SHOW,
   E_WIZARD_PAGE_STATE_HIDE,
   E_WIZARD_PAGE_STATE_SHUTDOWN
} E_Wizard_Page_State;

struct _E_Wizard_Page
{
   EINA_INLIST;
   void *handle;
   Evas *evas;
   int (*init)     (E_Wizard_Page *pg, Eina_Bool *need_xdg_desktops, Eina_Bool *need_xdg_icons);
   int (*shutdown) (E_Wizard_Page *pg);
   int (*show)     (E_Wizard_Page *pg);
   int (*hide)     (E_Wizard_Page *pg);
   int (*apply)    (E_Wizard_Page *pg);
   E_Wizard_Page_State state;
};

EAPI int e_wizard_init(void);
EAPI int e_wizard_shutdown(void);
EAPI void e_wizard_go(void);
EAPI void e_wizard_apply(void);
EAPI void e_wizard_next(void);
EAPI void e_wizard_page_show(Evas_Object *obj);
EAPI E_Wizard_Page *e_wizard_page_add(void *handle,
                                      int (*init)     (E_Wizard_Page *pg, Eina_Bool *need_xdg_desktops, Eina_Bool *need_xdg_icons),
                                      int (*shutdown) (E_Wizard_Page *pg),
                                      int (*show)     (E_Wizard_Page *pg),
                                      int (*hide)     (E_Wizard_Page *pg),
                                      int (*apply)    (E_Wizard_Page *pg)
                                     );
EAPI void e_wizard_page_del(E_Wizard_Page *pg);
EAPI void e_wizard_button_next_enable_set(int enable);
EAPI void e_wizard_title_set(const char *title);
EAPI void e_wizard_labels_update(void);
EAPI const char *e_wizard_dir_get(void);
EAPI void e_wizard_xdg_desktops_reset(void);

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Wizard Wizard
 *
 * Configures the whole Enlightenment in a nice walk-through wizard.
 *
 * Usually is presented on the first run of Enlightenment. The wizard
 * pages are configurable and should be extended by distributions that
 * want to ship Enlightenment as the default window manager.
 *
 * @}
 */
#endif
