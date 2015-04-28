#include "e_illume.h"
#include "tablet.h"
#include "policy.h"

EAPI E_Illume_Policy_Api e_illume_policy_api = 
{
   /* version, name, label */
   E_ILLUME_POLICY_API_VERSION, "tablet", "Tablet"
};

EAPI int 
e_illume_policy_init(E_Illume_Policy *p) 
{
   /* tell the policy what functions we support */
   p->funcs.border_add = _policy_border_add;
   p->funcs.border_del = _policy_border_del;
   p->funcs.border_focus_in = _policy_border_focus_in;
   p->funcs.border_focus_out = _policy_border_focus_out;
   p->funcs.border_activate = _policy_border_activate;
   p->funcs.border_post_fetch = _policy_border_post_fetch;
   p->funcs.border_post_assign = _policy_border_post_assign;
   p->funcs.border_show = _policy_border_show;
   p->funcs.zone_layout = _policy_zone_layout;
   p->funcs.zone_move_resize = _policy_zone_move_resize;
   p->funcs.zone_mode_change = _policy_zone_mode_change;
   p->funcs.zone_close = _policy_zone_close;
   p->funcs.drag_start = _policy_drag_start;
   p->funcs.drag_end = _policy_drag_end;
   p->funcs.focus_back = _policy_focus_back;
   p->funcs.focus_forward = _policy_focus_forward;
   p->funcs.focus_home = _policy_focus_home;
   p->funcs.property_change = _policy_property_change;

   return 1;
}

EAPI int 
e_illume_policy_shutdown(E_Illume_Policy *p) 
{
   p->funcs.border_add = NULL;
   p->funcs.border_del = NULL;
   p->funcs.border_focus_in = NULL;
   p->funcs.border_focus_out = NULL;
   p->funcs.border_activate = NULL;
   p->funcs.border_post_fetch = NULL;
   p->funcs.border_post_assign = NULL;
   p->funcs.border_show = NULL;
   p->funcs.zone_layout = NULL;
   p->funcs.zone_move_resize = NULL;
   p->funcs.zone_mode_change = NULL;
   p->funcs.zone_close = NULL;
   p->funcs.drag_start = NULL;
   p->funcs.drag_end = NULL;
   p->funcs.focus_back = NULL;
   p->funcs.focus_forward = NULL;
   p->funcs.focus_home = NULL;
   p->funcs.property_change = NULL;

   return 1;
}
