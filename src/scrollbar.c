#include "scrollbar.h"

static void e_scrollbar_recalc(E_Scrollbar *sb);
static void e_scrollbar_setup_bits(E_Scrollbar *sb);

static void
e_scrollbar_recalc(E_Scrollbar *sb)
{
   sb = NULL;
}

static void
e_scrollbar_setup_bits(E_Scrollbar *sb)
{
/*	sb->base = ebits_load("");*/
/*	sb->bar = ebits_load("");*/
   sb = NULL;
}

E_Scrollbar *
e_scrollbar_new(void)
{
   E_Scrollbar *sb;
   
   sb = NEW(E_Scrollbar, 1);
   ZERO(sb, E_Scrollbar, 1);
   sb->range = 1.0;
   sb->max = 1.0;
   sb->w = 12;
   sb->h = 64;
   return sb;
}

void
e_scrollbar_free(E_Scrollbar *sb)
{
   if (sb->evas)
     {
	if (sb->base) ebits_free(sb->base);
	if (sb->bar) ebits_free(sb->bar);
     }
   FREE(sb);
}

void
e_scrollbar_add_to_evas(E_Scrollbar *sb, Evas evas)
{
   if (sb->evas)
     {
	if (sb->base) ebits_free(sb->base);
	if (sb->bar) ebits_free(sb->bar);
     }
   sb->evas = evas;
   if (sb->evas)
     {
	e_scrollbar_setup_bits(sb);
	if (sb->base) ebits_set_layer(sb->base, sb->layer);
	if (sb->bar) ebits_set_layer(sb->bar, sb->layer);
	if (sb->base) ebits_move(sb->base, sb->x, sb->y);
	if (sb->base) ebits_resize(sb->base, sb->w, sb->h);
	e_scrollbar_recalc(sb);
	if (sb->bar) ebits_move(sb->bar, sb->bar_pos.x, sb->bar_pos.y);
	if (sb->bar) ebits_resize(sb->bar, sb->bar_pos.w, sb->bar_pos.h);
	if (sb->visible)
	  {
	     if (sb->base) ebits_show(sb->base);
	     if (sb->bar) ebits_show(sb->bar);
	  }
     }
}

void
e_scrollbar_show(E_Scrollbar *sb)
{
   if (sb->visible) return;
   sb->visible = 1;
   if (sb->base) ebits_show(sb->base);
   if (sb->bar) ebits_show(sb->bar);
}

void
e_scrollbar_hide(E_Scrollbar *sb)
{
   if (!sb->visible) return;
   sb->visible = 0;
   if (sb->base) ebits_hide(sb->base);
   if (sb->bar) ebits_hide(sb->bar);
}

void
e_scrollbar_raise(E_Scrollbar *sb)
{
   if (sb->base) ebits_raise(sb->base);
   if (sb->bar) ebits_raise(sb->bar);
}

void
e_scrollbar_lower(E_Scrollbar *sb)
{
   if (sb->bar) ebits_lower(sb->bar);
   if (sb->base) ebits_lower(sb->base);
}

void
e_scrollbar_set_layer(E_Scrollbar *sb, int l)
{
   if (l == sb->layer) return;
   sb->layer = l;
   if (sb->base) ebits_set_layer(sb->base, sb->layer);
   if (sb->bar) ebits_set_layer(sb->bar, sb->layer);
}

void
e_scrollbar_set_direction(E_Scrollbar *sb, int d)
{
   if (d == sb->direction) return;
   if (sb->evas)
     {
	Evas evas;
	
	if (sb->base) ebits_free(sb->base);
	if (sb->bar) ebits_free(sb->bar);
	evas = sb->evas;
	sb->evas = NULL;
	e_scrollbar_add_to_evas(sb, evas);
     }
 }

void
e_scrollbar_move(E_Scrollbar *sb, double x, double y)
{
   if ((x == sb->x) && (y == sb->y)) return;
   sb->x = x;
   sb->y = y;
   if (sb->base) ebits_move(sb->base, sb->x, sb->y);
   e_scrollbar_recalc(sb);
   if (sb->bar) ebits_move(sb->bar, sb->bar_pos.x, sb->bar_pos.y);
}

void
e_scrollbar_resize(E_Scrollbar *sb, double w, double h)
{
   if ((w == sb->w) && (h == sb->h)) return;
   sb->w = w;
   sb->h = h;
   if (sb->base) ebits_resize(sb->base, sb->w, sb->h);
   e_scrollbar_recalc(sb);
   if (sb->bar) ebits_move(sb->bar, sb->bar_pos.x, sb->bar_pos.y);
   if (sb->bar) ebits_resize(sb->bar, sb->bar_pos.w, sb->bar_pos.h);
}

void
e_scrollbar_set_change_func(E_Scrollbar *sb,
			    void (*func_change) (void *_data, E_Scrollbar *sb, double val),
			    void *data)
{
   sb->func_change = func_change;
   sb->func_data = data;
}

void
e_scrollbar_set_value(E_Scrollbar *sb, double val)
{
   if (sb->val == val) return;
   sb->val = val;
   e_scrollbar_recalc(sb);
   if (sb->bar) ebits_move(sb->bar, sb->bar_pos.x, sb->bar_pos.y);
   if (sb->bar) ebits_resize(sb->bar, sb->bar_pos.w, sb->bar_pos.h);
   if (sb->func_change) sb->func_change(sb->func_data, sb, sb->val);
}

void
e_scrollbar_set_range(E_Scrollbar *sb, double range)
{
   if (sb->range == range) return;
   sb->range = range;
   e_scrollbar_recalc(sb);
   if (sb->bar) ebits_move(sb->bar, sb->bar_pos.x, sb->bar_pos.y);
   if (sb->bar) ebits_resize(sb->bar, sb->bar_pos.w, sb->bar_pos.h);
}

void
e_scrollbar_set_max(E_Scrollbar *sb, double max)
{
   if (sb->max == max) return;
   sb->max = max;
   e_scrollbar_recalc(sb);
   if (sb->bar) ebits_move(sb->bar, sb->bar_pos.x, sb->bar_pos.y);
   if (sb->bar) ebits_resize(sb->bar, sb->bar_pos.w, sb->bar_pos.h);
}

double
e_scrollbar_get_value(E_Scrollbar *sb)
{
   return sb->val;
}

double
e_scrollbar_get_range(E_Scrollbar *sb)
{
   return sb->range;
}

double
e_scrollbar_get_max(E_Scrollbar *sb)
{
   return sb->max;
}

void
e_scrollbar_get_geometry(E_Scrollbar *sb, double *x, double *y, double *w, double *h)
{
   if (x) *x = sb->x;
   if (y) *y = sb->y;
   if (w) *w = sb->w;
   if (h) *h = sb->h;
}
