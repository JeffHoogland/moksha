#ifndef E_ICONIFY_H
#define E_ICONIFY_H

EAPI int        e_iconify_init(void);
EAPI int        e_iconify_shutdown(void);

EAPI Evas_List *e_iconify_clients_list_get(void);
EAPI int        e_iconify_border_iconfied(E_Border *bd);
EAPI void       e_iconify_border_add(E_Border *bd);
EAPI void       e_iconify_border_remove(E_Border *bd);
    
#endif
