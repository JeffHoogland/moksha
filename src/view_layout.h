#ifndef E_VIEW_LAYOUT_H
#define E_VIEW_LAYOUT_H

#include "e.h"
#include "e_view_look.h"
#include "object.h"

#ifndef E_VIEW_LAYOUT_TYPEDEF
#define E_VIEW_LAYOUT_TYPEDEF
typedef struct _E_View_Layout E_View_Layout;
typedef struct _E_View_Layout_Element E_View_Layout_Element;
#endif

#ifndef E_VIEW_TYPEDEF
#define E_VIEW_TYPEDEF
typedef struct _E_View    E_View;
#endif

struct _E_View_Layout
{
   E_Object     o;

   E_Desktop   *desktop;

   Ebits_Object bits;

   Evas_List *    elements;

   time_t       mod_time;
};

struct _E_View_Layout_Element
{
   char        *name;
   double       x, y, w, h;
};

E_View_Layout *e_view_layout_new(E_Desktop *d);
void e_view_layout_realize(E_View_Layout *layout);
void e_view_layout_update(E_View_Layout *layout);

void e_view_layout_add_new_element(E_View_Layout *layout, char *name);

/**
 * e_view_layout_get_element_geometry - Get element geometry
 * 
 * This function returns 1 if the element exists, and 0 if it doesn't.
 * It also sets the passed pointers (x, y, w, h) to the values of an
 * elements geometry.
 */
int e_view_layout_get_element_geometry(E_View_Layout *layout, char *element,
                                        double *x, double *y, double *w,
					double *h);

#endif
