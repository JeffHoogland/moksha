#ifdef E_TYPEDEFS
#else
#ifndef E_PLACE_H
#define E_PLACE_H

EAPI void e_place_zone_region_smart_cleanup(E_Zone *zone);
EAPI int e_place_zone_region_smart(E_Zone *zone, Eina_List *skiplist, int x, int y, int w, int h, int *rx, int *ry);
EAPI int e_place_desk_region_smart(E_Desk *desk, Eina_List *skiplist, int x, int y, int w, int h, int *rx, int *ry);
EAPI int e_place_zone_cursor(E_Zone *zone, int x, int y, int w, int h, int it, int *rx, int *ry);
EAPI int e_place_zone_manual(E_Zone *zone, int w, int h, int *rx, int *ry);

#endif
#endif
