#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "e.h"

typedef struct _Instance Instance;
typedef struct _Icon     Icon;

struct _Icon
{
   Ecore_X_Window win;
   Evas_Object   *o;
   Instance      *inst;
};

struct _Instance
{
   E_Gadcon_Client *gcc;
   E_Container     *con;
   Evas            *evas;
   struct
   {
      Ecore_X_Window parent;
      Ecore_X_Window base;
      Ecore_X_Window selection;
   } win;
   struct
   {
      Evas_Object *gadget;
   } ui;
   Eina_List *handlers;
   struct
   {
      Ecore_Timer *retry;
   } timer;
   struct
   {
      Ecore_Job *size_apply;
   } job;
   Eina_List *icons;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int   e_modapi_shutdown(E_Module *m);
EAPI int   e_modapi_save(E_Module *m);

extern Instance *instance;

/**
 * @addtogroup Optional_Gadgets
 * @{
 *
 * @defgroup Module_Systray Systray (System Icons Tray)
 *
 * Shows system icons in a box.
 *
 * The icons come from the FreeDesktop.Org systray specification.
 *
 * @see http://standards.freedesktop.org/systemtray-spec/systemtray-spec-latest.html
 * @}
 */
/**
 * systray implementation following freedesktop.org specification.
 *
 * @see: http://standards.freedesktop.org/systemtray-spec/latest/
 *
 * @todo: implement xembed, mostly done, at least relevant parts are done.
 *        http://standards.freedesktop.org/xembed-spec/latest/
 *
 * @todo: implement messages/popup part of the spec (anyone using this at all?)
 */

#endif
