/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

EAPI void e_resize_begin(E_Zone *zone, int w, int h);
EAPI void e_resize_end(void);
EAPI void e_resize_update(int w, int h);

EAPI void e_move_begin(E_Zone *zone, int x, int y);
EAPI void e_move_end(void);
EAPI void e_move_update(int x, int y);
