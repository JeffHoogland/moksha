#ifndef E_VIEW_LOOK_H
#define E_VIEW_LOOK_H
#include "observer.h"
#include "view_layout.h"
#include "e_dir.h"
#include <Evas.h>

#ifndef E_VIEW_LOOK_TYPEDEF
#define E_VIEW_LOOK_TYPEDEF
typedef struct _E_View_Look E_View_Look;
typedef struct _E_View_Look_Objects E_View_Look_Objects;
#endif


#ifndef E_DIR_TYPEDEF
#define E_DIR_TYPEDEF
typedef struct _E_Dir E_Dir;
#endif
#ifndef E_VIEW_LAYOUT_TYPEDEF
#define E_VIEW_LAYOUT_TYPEDEF
typedef struct _E_View_Layout E_View_Layout;
typedef struct _E_View_Layout_Element E_View_Layout_Element;
#endif

struct _E_View_Look 
{
   E_Observer                 o;

   E_View_Look_Objects      *obj;

   /* The .e_layout dir to monitor */
   E_Dir                    *dir;
};

struct _E_View_Look_Objects
{
   E_Observee          o;

   /* FIXME keep the bits/objects themselves here */
   
   char               *bg;
   char               *icb;
   char               *icb_bits;
   char               *layout;
};

E_View_Look *e_view_look_new(void);
void e_view_look_set_dir(E_View_Look *l, char *dir);

#endif
