#include "e.h"

int
e_place_container_region_smart(E_Container *con, Evas_List *skiplist, int x, int y, int w, int h, int *rx, int *ry)
{
   int                 a_w = 0, a_h = 0;
   int                *a_x = NULL, *a_y = NULL;
   Evas_List          *l, *ll;

   a_w = 2;
   a_h = 2;
   a_x = E_NEW(int, 2);
   a_y = E_NEW(int, 2);

   a_x[0] = 0;
   a_x[1] = con->w;
   a_y[0] = 0;
   a_y[1] = con->h;

   for (l = con->clients; l; l = l->next)
     {
	E_Border           *bd;
	int ok;
	
	ok = 1;
	bd = l->data;
	for (ll = skiplist; ll; ll = ll->next)
	  {
	     if (ll->data == bd)
	       {
		  ok = 0;
		  break;
	       }
	  }
	if ((ok) && (bd->visible))
	  {
	     if (E_INTERSECTS(bd->x, bd->y, bd->w, bd->h,
			    0, 0, con->w, con->h))
	       {
		  int i, j;

		  for (i = 0; i < a_w; i++)
		    {
		       int                 ok = 1;

		       if (bd->x > 0)
			 {
			    if (a_x[i] == bd->x)
			       ok = 0;
			    else if (a_x[i] > bd->x)
			      {
				 a_w++;
				 E_REALLOC(a_x, int, a_w);

				 for (j = a_w - 1; j > i; j--)
				    a_x[j] = a_x[j - 1];
				 a_x[i] = bd->x;
				 ok = 0;
			      }
			 }
		       if (!ok)
			  break;
		    }
		  for (i = 0; i < a_w; i++)
		    {
		       int ok = 1;

		       if (bd->x + bd->w < con->w)
			 {
			    if (a_x[i] == bd->x + bd->w)
			       ok = 0;
			    else if (a_x[i] > bd->x + bd->w)
			      {
				 a_w++;
				 E_REALLOC(a_x, int, a_w);

				 for (j = a_w - 1; j > i; j--)
				    a_x[j] = a_x[j - 1];
				 a_x[i] = bd->x + bd->w;
				 ok = 0;
			      }
			 }
		       if (!ok)
			  break;
		    }
		  for (i = 0; i < a_h; i++)
		    {
		       int ok = 1;

		       if (bd->y > 0)
			 {
			    if (a_y[i] == bd->y)
			       ok = 0;
			    else if (a_y[i] > bd->y)
			      {
				 a_h++;
				 E_REALLOC(a_y, int, a_h);

				 for (j = a_h - 1; j > i; j--)
				    a_y[j] = a_y[j - 1];
				 a_y[i] = bd->y;
				 ok = 0;
			      }
			 }
		       if (!ok)
			  break;
		    }
		  for (i = 0; i < a_h; i++)
		    {
		       int ok = 1;

		       if (bd->y + bd->h < con->h)
			 {
			    if (a_y[i] == bd->y + bd->h)
			       ok = 0;
			    else if (a_y[i] > bd->y + bd->h)
			      {
				 a_h++;
				 E_REALLOC(a_y, int, a_h);

				 for (j = a_h - 1; j > i; j--)
				    a_y[j] = a_y[j - 1];
				 a_y[i] = bd->y + bd->h;
				 ok = 0;
			      }
			 }
		       if (!ok)
			  break;
		    }
	       }
	  }
     }
   {
      int                 i, j;
      int                 area = 0x7fffffff;

      for (j = 0; j < a_h - 1; j++)
	{
	   for (i = 0; i < a_w - 1; i++)
	     {
		if ((a_x[i] < (con->w - w)) &&
		    (a_y[j] < (con->h - h)))
		  {
		     int ar = 0;

		     for (l = con->clients; l; l = l->next)
		       {
			  E_Border *bd;
			  int x1, y1, w1, h1, x2, y2, w2, h2;
			  int ok;
			  
			  ok = 1;
			  bd = l->data;
			  x1 = a_x[i];
			  y1 = a_y[j];
			  w1 = w;
			  h1 = h;
			  x2 = bd->x;
			  y2 = bd->y;
			  w2 = bd->w;
			  h2 = bd->h;
			  for (ll = skiplist; ll; ll = ll->next)
			    {
			       if (ll->data == bd)
				 {
				    ok = 0;
				    break;
				 }
			    }
			  if ((ok) && (bd->visible) &&
			      E_INTERSECTS(x1, y1, w1, h1, x2, y2, w2, h2))
			    {
			       int                 iw, ih;
			       int                 x0, x00, y0, y00;

			       x0 = x1;
			       if (x1 < x2)
				  x0 = x2;
			       x00 = (x1 + w1);
			       if ((x2 + w2) < (x1 + w1))
				  x00 = (x2 + w2);

			       y0 = y1;
			       if (y1 < y2)
				  y0 = y2;
			       y00 = (y1 + h1);
			       if ((y2 + h2) < (y1 + h1))
				  y00 = (y2 + h2);

			       iw = x00 - x0;
			       ih = y00 - y0;
			       ar += (iw * ih);
			    }
		       }
		     if (ar < area)
		       {
			  area = ar;
			  *rx = a_x[i];
			  *ry = a_y[j];
			  if (ar == 0)
			     goto done;
		       }
		  }
		if ((a_x[i + 1] - w > 0) && (a_y[j] < (con->h - h)))
		  {
		     int ar = 0;

		     for (l = con->clients; l; l = l->next)
		       {
			  E_Border *bd;
			  int x1, y1, w1, h1, x2, y2, w2, h2;
			  int ok;
			  
			  ok = 1;
			  bd = l->data;
			  x1 = a_x[i + 1] - w;
			  y1 = a_y[j];
			  w1 = w;
			  h1 = h;
			  x2 = bd->x;
			  y2 = bd->y;
			  w2 = bd->w;
			  h2 = bd->h;
			  for (ll = skiplist; ll; ll = ll->next)
			    {
			       if (ll->data == bd)
				 {
				    ok = 0;
				    break;
				 }
			    }
			  if ((ok) && (bd->visible) &&
			      E_INTERSECTS(x1, y1, w1, h1, x2, y2, w2, h2))
			    {
			       int                 iw, ih;
			       int                 x0, x00, y0, y00;

			       x0 = x1;
			       if (x1 < x2)
				  x0 = x2;
			       x00 = (x1 + w1);
			       if ((x2 + w2) < (x1 + w1))
				  x00 = (x2 + w2);

			       y0 = y1;
			       if (y1 < y2)
				  y0 = y2;
			       y00 = (y1 + h1);
			       if ((y2 + h2) < (y1 + h1))
				  y00 = (y2 + h2);

			       iw = x00 - x0;
			       ih = y00 - y0;
			       ar += (iw * ih);
			    }
		       }
		     if (ar < area)
		       {
			  area = ar;
			  *rx = a_x[i + 1] - w;
			  *ry = a_y[j];
			  if (ar == 0)
			     goto done;
		       }
		  }
		if ((a_x[i + 1] - w > 0) && (a_y[j + 1] - h > 0))
		  {
		     int ar = 0;

		     for (l = con->clients; l; l = l->next)
		       {
			  E_Border *bd;
			  int x1, y1, w1, h1, x2, y2, w2, h2;
			  int ok;
			  
			  ok = 1;
			  bd = l->data;
			  x1 = a_x[i + 1] - w;
			  y1 = a_y[j + 1] - h;
			  w1 = w;
			  h1 = h;
			  x2 = bd->x;
			  y2 = bd->y;
			  w2 = bd->w;
			  h2 = bd->h;
			  for (ll = skiplist; ll; ll = ll->next)
			    {
			       if (ll->data == bd)
				 {
				    ok = 0;
				    break;
				 }
			    }
			  if ((ok) && (bd->visible) &&
			      E_INTERSECTS(x1, y1, w1, h1, x2, y2, w2, h2))
			    {
			       int                 iw, ih;
			       int                 x0, x00, y0, y00;

			       x0 = x1;
			       if (x1 < x2)
				  x0 = x2;
			       x00 = (x1 + w1);
			       if ((x2 + w2) < (x1 + w1))
				  x00 = (x2 + w2);

			       y0 = y1;
			       if (y1 < y2)
				  y0 = y2;
			       y00 = (y1 + h1);
			       if ((y2 + h2) < (y1 + h1))
				  y00 = (y2 + h2);

			       iw = x00 - x0;
			       ih = y00 - y0;
			       ar += (iw * ih);
			    }
		       }
		     if (ar < area)
		       {
			  area = ar;
			  *rx = a_x[i + 1] - w;
			  *ry = a_y[j + 1] - h;
			  if (ar == 0)
			     goto done;
		       }
		  }
		if ((a_x[i] < (con->w - w)) && (a_y[j + 1] - h > 0))
		  {
		     int ar = 0;

		     for (l = con->clients; l; l = l->next)
		       {
			  E_Border *bd;
			  int x1, y1, w1, h1, x2, y2, w2, h2;
			  int ok;
			  
			  ok = 1;
			  bd = l->data;
			  x1 = a_x[i];
			  y1 = a_y[j + 1] - h;
			  w1 = w;
			  h1 = h;
			  x2 = bd->x;
			  y2 = bd->y;
			  w2 = bd->w;
			  h2 = bd->h;
			  for (ll = skiplist; ll; ll = ll->next)
			    {
			       if (ll->data == bd)
				 {
				    ok = 0;
				    break;
				 }
			    }
			  if ((ok) && (bd->visible) &&
			      E_INTERSECTS(x1, y1, w1, h1, x2, y2, w2, h2))
			    {
			       int                 iw, ih;
			       int                 x0, x00, y0, y00;

			       x0 = x1;
			       if (x1 < x2)
				  x0 = x2;
			       x00 = (x1 + w1);
			       if ((x2 + w2) < (x1 + w1))
				  x00 = (x2 + w2);

			       y0 = y1;
			       if (y1 < y2)
				  y0 = y2;
			       y00 = (y1 + h1);
			       if ((y2 + h2) < (y1 + h1))
				  y00 = (y2 + h2);

			       iw = x00 - x0;
			       ih = y00 - y0;
			       ar += (iw * ih);
			    }
		       }
		     if (ar < area)
		       {
			  area = ar;
			  *rx = a_x[i];
			  *ry = a_y[j + 1] - h;
			  if (ar == 0)
			     goto done;
		       }
		  }
	     }
	}
   }
 done:
   E_FREE(a_x);
   E_FREE(a_y);
   return 1;
}
