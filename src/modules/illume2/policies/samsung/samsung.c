#include "E_Illume.h"
#include "samsung.h"
#include "layout.h"

EAPI E_Illume_Layout_Api e_layapi = 
{
   E_ILLUME_LAYOUT_API_VERSION, "Samsung", "samsung" 
};

EAPI void *
e_layapi_init(E_Illume_Layout_Policy *p) 
{
   p->funcs.border_add = _layout_border_add;
   p->funcs.border_del = _layout_border_del;
   p->funcs.border_focus_in = _layout_border_focus_in;
   p->funcs.border_focus_out = _layout_border_focus_out;
   p->funcs.border_activate = _layout_border_activate;
   p->funcs.zone_layout = _layout_zone_layout;
   p->funcs.zone_move_resize = _layout_zone_move_resize;
   p->funcs.drag_start = _layout_drag_start;
   p->funcs.drag_end = _layout_drag_end;
   return p;
}

EAPI int 
e_layapi_shutdown(E_Illume_Layout_Policy *p) 
{
   p->funcs.border_add = NULL;
   p->funcs.border_del = NULL;
   p->funcs.border_focus_in = NULL;
   p->funcs.border_focus_out = NULL;
   p->funcs.border_activate = NULL;
   p->funcs.zone_layout = NULL;
   p->funcs.zone_move_resize = NULL;
   p->funcs.drag_start = NULL;
   p->funcs.drag_end = NULL;
   return 1;
}

EAPI void 
e_layapi_config(E_Container *con, const char *params) 
{

}
