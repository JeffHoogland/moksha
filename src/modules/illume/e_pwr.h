#ifndef E_PWR_H
#define E_PWR_H

EAPI int e_pwr_init(void);
EAPI int e_pwr_shutdown(void);

EAPI void e_pwr_cfg_update(void);
EAPI void e_pwr_init_done(void);
    
#endif
