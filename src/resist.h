#ifndef E_RESIST_H
#define E_RESIST_H

#include "e.h"
#include "border.h"

typedef struct _E_Rect                E_Rect;

struct _E_Rect
{
   int x, y, w, h;
   int v1, v2, v3, v4;
};

void e_resist_border(E_Border *b);

#endif
