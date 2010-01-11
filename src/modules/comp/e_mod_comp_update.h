#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_UPDATE_H
#define E_MOD_COMP_UPDATE_H

typedef struct _Update      Update;
typedef struct _Update_Rect Update_Rect;
typedef enum _Update_Policy
{
   UPDATE_POLICY_RAW,
   UPDATE_POLICY_HALF_WIDTH_OR_MORE_ROUND_UP_TO_FULL_WIDTH,
} Update_Policy;

struct _Update_Rect
{
   int x, y, w, h;
};

Update      *e_mod_comp_update_new           (void);
void         e_mod_comp_update_free          (Update *up);
void         e_mod_comp_update_policy_set    (Update *up, Update_Policy pol);
void         e_mod_comp_update_tile_size_set (Update *up, int tsw, int tsh);
void         e_mod_comp_update_resize        (Update *up, int w, int h);
void         e_mod_comp_update_add           (Update *up, int x, int y, int w, int h);
Update_Rect *e_mod_comp_update_rects_get     (Update *up);
void         e_mod_comp_update_clear         (Update *up);

#endif
#endif
