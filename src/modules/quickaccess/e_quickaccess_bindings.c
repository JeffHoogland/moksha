#include "e_mod_main.h"


void
e_qa_entry_bindings_cleanup(E_Quick_Access_Entry *entry)
{
   Eina_List *l, *ll;
   E_Config_Binding_Key *kbi;
   E_Config_Binding_Mouse *mbi;
   E_Config_Binding_Edge *ebi;
   E_Config_Binding_Wheel *wbi;
   E_Config_Binding_Acpi *abi;
   E_Config_Binding_Signal *sbi;

   EINA_LIST_FOREACH_SAFE(e_config->key_bindings, l, ll, kbi)
     {
        if ((kbi->action == _act_toggle) && (kbi->params == entry->id))
          {
             DBG("removed keybind for %s", entry->id);
             e_config->key_bindings = eina_list_remove_list(e_config->key_bindings, l);
             eina_stringshare_del(kbi->key);
             eina_stringshare_del(kbi->action);
             eina_stringshare_del(kbi->params);
             free(kbi);
          }
     }
   EINA_LIST_FOREACH_SAFE(e_config->mouse_bindings, l, ll, mbi)
     {
        if ((mbi->action == _act_toggle) && (mbi->params == entry->id))
          {
             DBG("removed mousebind for %s", entry->id);
             e_config->mouse_bindings = eina_list_remove_list(e_config->mouse_bindings, l);
             eina_stringshare_del(mbi->action);
             eina_stringshare_del(mbi->params);
             free(mbi);
          }
     }
   EINA_LIST_FOREACH_SAFE(e_config->edge_bindings, l, ll, ebi)
     {
        if ((ebi->action == _act_toggle) && (ebi->params == entry->id))
          {
             DBG("removed edgebind for %s", entry->id);
             e_config->edge_bindings = eina_list_remove_list(e_config->edge_bindings, l);
             eina_stringshare_del(ebi->action);
             eina_stringshare_del(ebi->params);
             free(ebi);
          }
     }
   EINA_LIST_FOREACH_SAFE(e_config->wheel_bindings, l, ll, wbi)
     {
        if ((wbi->action == _act_toggle) && (wbi->params == entry->id))
          {
             DBG("removed wheelbind for %s", entry->id);
             e_config->wheel_bindings = eina_list_remove_list(e_config->wheel_bindings, l);
             eina_stringshare_del(wbi->action);
             eina_stringshare_del(wbi->params);
             free(wbi);
          }
     }
   EINA_LIST_FOREACH_SAFE(e_config->acpi_bindings, l, ll, abi)
     {
        if ((abi->action == _act_toggle) && (abi->params == entry->id))
          {
             DBG("removed acpibind for %s", entry->id);
             e_config->acpi_bindings = eina_list_remove_list(e_config->acpi_bindings, l);
             eina_stringshare_del(abi->action);
             eina_stringshare_del(abi->params);
             free(abi);
          }
     }
   EINA_LIST_FOREACH_SAFE(e_config->signal_bindings, l, ll, sbi)
     {
        if ((sbi->action == _act_toggle) && (sbi->params == entry->id))
          {
             DBG("removed signalbind for %s", entry->id);
             e_config->signal_bindings = eina_list_remove_list(e_config->signal_bindings, l);
             eina_stringshare_del(sbi->action);
             eina_stringshare_del(sbi->params);
             free(sbi);
          }
     }
}

void
e_qa_entry_bindings_rename(E_Quick_Access_Entry *entry, const char *name)
{
   Eina_List *l;
   E_Config_Binding_Key *kbi;
   E_Config_Binding_Mouse *mbi;
   E_Config_Binding_Edge *ebi;
   E_Config_Binding_Wheel *wbi;
   E_Config_Binding_Acpi *abi;
   E_Config_Binding_Signal *sbi;

#define RENAME(TYPE, VAR) do {\
   EINA_LIST_FOREACH(e_config->TYPE##_bindings, l, VAR) \
     { \
        if ((VAR->action == _act_toggle) && (VAR->params == entry->id)) \
          { \
             DBG("removed %sbind for %s", #TYPE, entry->id); \
             eina_stringshare_replace(&VAR->params, name); \
          } \
     } \
   } while (0)
   RENAME(key, kbi);
   RENAME(mouse, mbi);
   RENAME(edge, ebi);
   RENAME(wheel, wbi);
   RENAME(acpi, abi);
   RENAME(signal, sbi);
   e_bindings_reset();
}
