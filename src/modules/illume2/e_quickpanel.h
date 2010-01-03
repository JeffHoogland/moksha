#ifndef E_QUICKPANEL_H
#define E_QUICKPANEL_H

#define E_QUICKPANEL_TYPE 0xE1b0990

typedef struct _E_Quickpanel E_Quickpanel;
struct _E_Quickpanel 
{
   E_Object e_obj_inherit;

   E_Zone *zone;
   Eina_List *borders;
   Ecore_Timer *timer;
   Ecore_Animator *animator;
   double start, len;
   int h, adjust, adjust_start, adjust_end;
   unsigned char visible : 1;
};

int e_quickpanel_init(void);
int e_quickpanel_shutdown(void);

E_Quickpanel *e_quickpanel_new(E_Zone *zone);
void e_quickpanel_show(E_Quickpanel *qp);
void e_quickpanel_hide(E_Quickpanel *qp);
E_Quickpanel *e_quickpanel_by_zone_get(E_Zone *zone);
void e_quickpanel_position_update(E_Quickpanel *qp);

#endif
