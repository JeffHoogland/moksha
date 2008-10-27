#ifndef E_CFG_H
#define E_CFG_H

#define ILLUME_CONFIG_MIN 3
#define ILLUME_CONFIG_MAJ 0

typedef struct _Illume_Cfg Illume_Cfg;

struct _Illume_Cfg
{
   int config_version;
   
   struct {
      int mode;
      int icon_size;
      int single_click;
   } launcher;
   
   struct {
      int auto_suspend;
      int auto_suspend_delay;
   } power;
   
   struct {
      int cache_level; // DOME?
      int fps;
   } performance;
   
   struct {
      int main_gadget_size;
      int extra_gagdet_size;
      int style;
   } slipshelf;
   
   struct {
      struct {
	 int duration;
      } slipshelf, kbd, busywin, layout;
   } sliding;
   
   // FIXME: save/load these up minor version for this and init...
   struct {
      int         use_internal;
      const char *run_keyboard;
      const char *dict;
      double      fuzz_mul; // NEW
      int         ignore_auto_kbd; // NEW
      int         ignore_auto_type; // NEW
      int         ignore_auto_lang; // NEW
      int         ignore_hardware_keyboards; // NEW
      int         force_no_dict; // NEW
      const char *layout; // NEW
   } kbd;
};

EAPI int e_cfg_init(E_Module *m);
EAPI int e_cfg_shutdown(void);

EAPI int e_cfg_save(void);

EAPI void e_cfg_launcher(E_Container *con, const char *params);
EAPI void e_cfg_power(E_Container *con, const char *params);
EAPI void e_cfg_animation(E_Container *con, const char *params);
EAPI void e_cfg_slipshelf(E_Container *con, const char *params);
EAPI void e_cfg_thumbscroll(E_Container *con, const char *params);
EAPI void e_cfg_fps(E_Container *con, const char *params);
EAPI void e_cfg_gadgets(E_Container *con, const char *params);
EAPI void e_cfg_keyboard(E_Container *con, const char *params);
    
extern EAPI Illume_Cfg *illume_cfg;

#endif
