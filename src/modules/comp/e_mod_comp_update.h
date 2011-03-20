#ifdef E_TYPEDEFS
#else
#ifndef E_MOD_COMP_UPDATE_H
#define E_MOD_COMP_UPDATE_H

typedef struct _E_Update      E_Update;
typedef struct _E_Update_Rect E_Update_Rect;
typedef enum _E_Update_Policy
{
   E_UPDATE_POLICY_RAW,
   E_UPDATE_POLICY_HALF_WIDTH_OR_MORE_ROUND_UP_TO_FULL_WIDTH,
} E_Update_Policy;

struct _E_Update_Rect
{
   int x, y, w, h;
};

struct _E_Update
{
   int             w, h;
   int             tw, th;
   int             tsw, tsh;
   unsigned char  *tiles;
   E_Update_Policy pol;
};

E_Update *e_mod_comp_update_new(void);
void      e_mod_comp_update_free(E_Update *up);
void      e_mod_comp_update_policy_set(E_Update       *up,
                                       E_Update_Policy pol);
void      e_mod_comp_update_tile_size_set(E_Update *up,
                                          int       tsw,
                                          int       tsh);
void e_mod_comp_update_resize(E_Update *up,
                              int       w,
                              int       h);
void e_mod_comp_update_add(E_Update *up,
                           int       x,
                           int       y,
                           int       w,
                           int       h);
E_Update_Rect *e_mod_comp_update_rects_get(E_Update *up);
void           e_mod_comp_update_clear(E_Update *up);

#endif
#endif
